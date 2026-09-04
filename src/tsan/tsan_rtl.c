/* Misan ThreadSanitizer runtime
 * Happens-before vector clock tracking for data races.
 */

#include "sanitizer/common.h"
#include "sanitizer/tsan_interface.h"

#define TSAN_MAX_THREADS 64
#define TSAN_MAX_MUTEXES 1024
#define TSAN_SHADOW_ENTRIES 65536

typedef struct {
    uint64_t clock[TSAN_MAX_THREADS];
} tsan_vc_t;

typedef struct {
    int tid;
    tsan_vc_t vc;
    int active;
} tsan_thread_t;

typedef struct {
    void *addr;
    tsan_vc_t vc;
    int tid;
    int is_write;
    int active;
} tsan_shadow_t;

typedef struct {
    void *addr;
    tsan_vc_t vc;
    int owner_tid;
    int active;
} tsan_mutex_t;

static tsan_thread_t tsan_threads[TSAN_MAX_THREADS];
static tsan_shadow_t tsan_shadow[TSAN_SHADOW_ENTRIES];
static tsan_mutex_t tsan_mutexes[TSAN_MAX_MUTEXES];
static volatile int tsan_thread_count = 0;
static __thread int tsan_current_tid = -1;
static drt_spinlock_t tsan_lock = MISAN_SPINLOCK_INIT;

static int tsan_get_tid(void) {
    if (tsan_current_tid < 0) {
        int new_tid = drt_atomic_add((int *)&tsan_thread_count, 1);
        if (new_tid >= TSAN_MAX_THREADS) {
            drt_die("TSan: too many threads");
        }
        tsan_current_tid = new_tid;
        tsan_threads[new_tid].tid = new_tid;
        tsan_threads[new_tid].active = 1;
        drt_memset(&tsan_threads[new_tid].vc, 0, sizeof(tsan_vc_t));
        tsan_threads[new_tid].vc.clock[new_tid] = 1;
    }
    return tsan_current_tid;
}

void __tsan_init(void) {
    tsan_get_tid();
}

void __tsan_func_entry(void *pc) {
    (void)pc;
    tsan_get_tid();
}

void __tsan_func_exit(void) {
    /* No-op for function exit in minimal implementation */
}

static void tsan_vc_inc(tsan_vc_t *vc, int tid) {
    vc->clock[tid]++;
}

static void tsan_vc_join(tsan_vc_t *dst, const tsan_vc_t *src) {
    for (int i = 0; i < TSAN_MAX_THREADS; i++) {
        if (src->clock[i] > dst->clock[i])
            dst->clock[i] = src->clock[i];
    }
}

static int tsan_vc_happens_before(const tsan_vc_t *a, const tsan_vc_t *b, int tid_a) {
    return a->clock[tid_a] <= b->clock[tid_a];
}

static inline uint32_t tsan_shadow_hash(void *addr) {
    uintptr_t p = (uintptr_t)addr;
    p ^= p >> 16;
    p ^= p >> 8;
    return (uint32_t)(p % TSAN_SHADOW_ENTRIES);
}

static tsan_shadow_t *tsan_shadow_get(void *addr, int alloc_if_missing) {
    uint32_t idx = tsan_shadow_hash(addr);
    uint32_t start = idx;
    do {
        if (tsan_shadow[idx].active && tsan_shadow[idx].addr == addr)
            return &tsan_shadow[idx];
        if (!tsan_shadow[idx].active && alloc_if_missing) {
            tsan_shadow[idx].addr = addr;
            tsan_shadow[idx].active = 1;
            return &tsan_shadow[idx];
        }
        idx = (idx + 1) % TSAN_SHADOW_ENTRIES;
    } while (idx != start);
    return 0;
}

static void tsan_report_race(void *addr, int is_write, int other_tid, int other_is_write) {
    drt_printf("\n==TSAN: DATA RACE==\n");
    drt_printf("  Address: %p\n", addr);
    drt_printf("  Current access: %s by T%d\n", is_write ? "WRITE" : "READ", tsan_current_tid);
    drt_printf("  Previous access: %s by T%d\n", other_is_write ? "WRITE" : "READ", other_tid);
    drt_stack_trace_t st;
    drt_arch_capture_stack_trace(&st, 2);
    drt_print_stack_trace(&st);
    drt_arch_abort();
}

static void tsan_access(void *addr, size_t size, int is_write) {
    (void)size;
    int tid = tsan_get_tid();

    drt_spin_lock(&tsan_lock);

    tsan_shadow_t *shadow = tsan_shadow_get(addr, 1);
    if (!shadow) {
        drt_spin_unlock(&tsan_lock);
        return;
    }

    if (shadow->active && shadow->addr == addr) {
        /* Check for race */
        if (shadow->tid != tid) {
            if (is_write || shadow->is_write) {
                /* Write-Write or Write-Read or Read-Write race */
                if (!tsan_vc_happens_before(&tsan_threads[shadow->tid].vc,
                                            &tsan_threads[tid].vc, shadow->tid) &&
                    !tsan_vc_happens_before(&tsan_threads[tid].vc,
                                            &tsan_threads[shadow->tid].vc, tid)) {
                    tsan_report_race(addr, is_write, shadow->tid, shadow->is_write);
                }
            }
        }
    }

    /* Update shadow */
    shadow->vc = tsan_threads[tid].vc;
    shadow->tid = tid;
    shadow->is_write = is_write;
    tsan_vc_inc(&tsan_threads[tid].vc, tid);

    drt_spin_unlock(&tsan_lock);
}

void __tsan_read1(void *addr)  { tsan_access(addr, 1, 0); }
void __tsan_read2(void *addr)  { tsan_access(addr, 2, 0); }
void __tsan_read4(void *addr)  { tsan_access(addr, 4, 0); }
void __tsan_read8(void *addr)  { tsan_access(addr, 8, 0); }
void __tsan_read16(void *addr) { tsan_access(addr, 16, 0); }

void __tsan_write1(void *addr)  { tsan_access(addr, 1, 1); }
void __tsan_write2(void *addr)  { tsan_access(addr, 2, 1); }
void __tsan_write4(void *addr)  { tsan_access(addr, 4, 1); }
void __tsan_write8(void *addr)  { tsan_access(addr, 8, 1); }
void __tsan_write16(void *addr) { tsan_access(addr, 16, 1); }

void __tsan_read_range(void *addr, void *end) {
    uintptr_t a = (uintptr_t)addr;
    uintptr_t e = (uintptr_t)end;
    while (a < e) {
        tsan_access((void *)a, 1, 0);
        a++;
    }
}

void __tsan_write_range(void *addr, void *end) {
    uintptr_t a = (uintptr_t)addr;
    uintptr_t e = (uintptr_t)end;
    while (a < e) {
        tsan_access((void *)a, 1, 1);
        a++;
    }
}

int8_t __tsan_atomic8_load(const void *addr, int mo) {
    (void)mo;
    tsan_access((void *)addr, 1, 0);
    return *(const volatile int8_t *)addr;
}

int16_t __tsan_atomic16_load(const void *addr, int mo) {
    (void)mo;
    tsan_access((void *)addr, 2, 0);
    return *(const volatile int16_t *)addr;
}

int32_t __tsan_atomic32_load(const void *addr, int mo) {
    (void)mo;
    tsan_access((void *)addr, 4, 0);
    return *(const volatile int32_t *)addr;
}

int64_t __tsan_atomic64_load(const void *addr, int mo) {
    (void)mo;
    tsan_access((void *)addr, 8, 0);
    return *(const volatile int64_t *)addr;
}

void __tsan_atomic8_store(void *addr, int8_t val, int mo) {
    (void)mo;
    tsan_access(addr, 1, 1);
    *(volatile int8_t *)addr = val;
}

void __tsan_atomic16_store(void *addr, int16_t val, int mo) {
    (void)mo;
    tsan_access(addr, 2, 1);
    *(volatile int16_t *)addr = val;
}

void __tsan_atomic32_store(void *addr, int32_t val, int mo) {
    (void)mo;
    tsan_access(addr, 4, 1);
    *(volatile int32_t *)addr = val;
}

void __tsan_atomic64_store(void *addr, int64_t val, int mo) {
    (void)mo;
    tsan_access(addr, 8, 1);
    *(volatile int64_t *)addr = val;
}

static tsan_mutex_t *tsan_mutex_find(void *addr, int alloc) {
    uint32_t idx = ((uintptr_t)addr / sizeof(void *)) % TSAN_MAX_MUTEXES;
    uint32_t start = idx;
    do {
        if (tsan_mutexes[idx].active && tsan_mutexes[idx].addr == addr)
            return &tsan_mutexes[idx];
        if (!tsan_mutexes[idx].active && alloc) {
            tsan_mutexes[idx].addr = addr;
            tsan_mutexes[idx].active = 1;
            drt_memset(&tsan_mutexes[idx].vc, 0, sizeof(tsan_vc_t));
            return &tsan_mutexes[idx];
        }
        idx = (idx + 1) % TSAN_MAX_MUTEXES;
    } while (idx != start);
    return 0;
}

void __tsan_mutex_create(void *addr) {
    drt_spin_lock(&tsan_lock);
    tsan_mutex_t *m = tsan_mutex_find(addr, 1);
    if (m) {
        m->owner_tid = -1;
        drt_memset(&m->vc, 0, sizeof(tsan_vc_t));
    }
    drt_spin_unlock(&tsan_lock);
}

void __tsan_mutex_destroy(void *addr) {
    drt_spin_lock(&tsan_lock);
    tsan_mutex_t *m = tsan_mutex_find(addr, 0);
    if (m) m->active = 0;
    drt_spin_unlock(&tsan_lock);
}

void __tsan_mutex_pre_lock(void *addr, int rw) {
    (void)rw;
    tsan_access(addr, sizeof(void *), 1);
}

void __tsan_mutex_post_lock(void *addr, int rw) {
    (void)rw;
    int tid = tsan_get_tid();
    drt_spin_lock(&tsan_lock);
    tsan_mutex_t *m = tsan_mutex_find(addr, 0);
    if (m) {
        /* Acquire: join mutex VC into thread VC */
        tsan_vc_join(&tsan_threads[tid].vc, &m->vc);
        m->owner_tid = tid;
    }
    drt_spin_unlock(&tsan_lock);
}

void __tsan_mutex_pre_unlock(void *addr, int rw) {
    (void)rw;
    tsan_access(addr, sizeof(void *), 1);
}

void __tsan_mutex_post_unlock(void *addr, int rw) {
    (void)rw;
    int tid = tsan_get_tid();
    drt_spin_lock(&tsan_lock);
    tsan_mutex_t *m = tsan_mutex_find(addr, 0);
    if (m) {
        /* Release: copy thread VC into mutex VC */
        m->vc = tsan_threads[tid].vc;
        tsan_vc_inc(&m->vc, tid);
    }
    drt_spin_unlock(&tsan_lock);
}

void __tsan_vptr_read(void *addr) {
    tsan_access(addr, sizeof(void *), 0);
}

void __tsan_vptr_update(void *addr, void *new_val) {
    (void)new_val;
    tsan_access(addr, sizeof(void *), 1);
}
