/* Misan LeakSanitizer runtime
 * Allocation tracking with mark-and-sweep leak detection.
 */

#include "sanitizer/common.h"
#include "sanitizer/lsan_interface.h"

#define LSAN_MAX_ALLOCS 4096
#define LSAN_MAX_ROOTS  256

typedef enum {
    LSAN_ALLOC_ACTIVE,
    LSAN_ALLOC_FREED
} lsan_alloc_state_t;

typedef struct lsan_alloc {
    void *addr;
    size_t size;
    lsan_alloc_state_t state;
    int ignored;
    drt_stack_trace_t stack;
    uint64_t timestamp;
} lsan_alloc_t;

typedef struct {
    const void *addr;
    size_t size;
} lsan_root_t;

static lsan_alloc_t lsan_allocs[LSAN_MAX_ALLOCS];
static lsan_root_t lsan_roots[LSAN_MAX_ROOTS];
static volatile int lsan_alloc_count = 0;
static volatile int lsan_root_count = 0;
static volatile int lsan_disabled = 0;
static drt_spinlock_t lsan_lock = MISAN_SPINLOCK_INIT;

void __lsan_register_alloc(void *addr, size_t size) {
    if (lsan_disabled) return;
    drt_spin_lock(&lsan_lock);
    if (lsan_alloc_count >= LSAN_MAX_ALLOCS) {
        drt_spin_unlock(&lsan_lock);
        return;
    }
    int idx = lsan_alloc_count++;
    lsan_allocs[idx].addr = addr;
    lsan_allocs[idx].size = size;
    lsan_allocs[idx].state = LSAN_ALLOC_ACTIVE;
    lsan_allocs[idx].ignored = 0;
    lsan_allocs[idx].timestamp = drt_arch_get_time_ns();
    drt_arch_capture_stack_trace(&lsan_allocs[idx].stack, 2);
    drt_spin_unlock(&lsan_lock);
}

void __lsan_unregister_alloc(void *addr) {
    if (lsan_disabled) return;
    drt_spin_lock(&lsan_lock);
    for (int i = 0; i < lsan_alloc_count; i++) {
        if (lsan_allocs[i].addr == addr && lsan_allocs[i].state == LSAN_ALLOC_ACTIVE) {
            lsan_allocs[i].state = LSAN_ALLOC_FREED;
            drt_spin_unlock(&lsan_lock);
            return;
        }
    }
    drt_spin_unlock(&lsan_lock);
}

void __lsan_register_root_region(const void *p, size_t size) {
    drt_spin_lock(&lsan_lock);
    if (lsan_root_count < LSAN_MAX_ROOTS) {
        int idx = lsan_root_count++;
        lsan_roots[idx].addr = p;
        lsan_roots[idx].size = size;
    }
    drt_spin_unlock(&lsan_lock);
}

void __lsan_unregister_root_region(const void *p, size_t size) {
    (void)size;
    drt_spin_lock(&lsan_lock);
    for (int i = 0; i < lsan_root_count; i++) {
        if (lsan_roots[i].addr == p) {
            lsan_roots[i] = lsan_roots[--lsan_root_count];
            break;
        }
    }
    drt_spin_unlock(&lsan_lock);
}

void __lsan_ignore_object(const void *p) {
    drt_spin_lock(&lsan_lock);
    for (int i = 0; i < lsan_alloc_count; i++) {
        if (lsan_allocs[i].addr == p) {
            lsan_allocs[i].ignored = 1;
            break;
        }
    }
    drt_spin_unlock(&lsan_lock);
}

void __lsan_disable(void) {
    __atomic_store_n((int *)&lsan_disabled, 1, __ATOMIC_SEQ_CST);
}

void __lsan_enable(void) {
    __atomic_store_n((int *)&lsan_disabled, 0, __ATOMIC_SEQ_CST);
}

static int lsan_is_pointer_inside(const void *ptr, const void *base, size_t size) {
    uintptr_t p = (uintptr_t)ptr;
    uintptr_t b = (uintptr_t)base;
    return p >= b && p < b + size;
}

static int lsan_mark_reachable(lsan_alloc_t *alloc) {
    /* Check root regions */
    for (int r = 0; r < lsan_root_count; r++) {
        if (lsan_is_pointer_inside(alloc->addr, lsan_roots[r].addr, lsan_roots[r].size))
            return 1;
        /* Also check if root region contains a pointer to this alloc */
        const uintptr_t *scan = lsan_roots[r].addr;
        size_t words = lsan_roots[r].size / sizeof(void *);
        for (size_t w = 0; w < words; w++) {
            if (scan[w] == (uintptr_t)alloc->addr) return 1;
        }
    }

    /* Check other active allocations for pointers */
    for (int i = 0; i < lsan_alloc_count; i++) {
        if (lsan_allocs[i].state != LSAN_ALLOC_ACTIVE) continue;
        uintptr_t *scan = lsan_allocs[i].addr;
        size_t words = lsan_allocs[i].size / sizeof(void *);
        for (size_t w = 0; w < words; w++) {
            if (scan[w] == (uintptr_t)alloc->addr) return 1;
        }
    }

    return 0;
}

static int lsan_do_check_internal(int fatal) {
    int leaks_found = 0;

    drt_spin_lock(&lsan_lock);
    for (int i = 0; i < lsan_alloc_count; i++) {
        if (lsan_allocs[i].state != LSAN_ALLOC_ACTIVE) continue;
        if (lsan_allocs[i].ignored) continue;

        if (!lsan_mark_reachable(&lsan_allocs[i])) {
            leaks_found++;
            drt_printf("\n==LSAN: LEAK DETECTED==\n");
            drt_printf("  Address: %p  Size: %zu bytes\n",
                       lsan_allocs[i].addr, lsan_allocs[i].size);
            drt_print_stack_trace(&lsan_allocs[i].stack);
        }
    }
    drt_spin_unlock(&lsan_lock);

    if (leaks_found && fatal) {
        drt_die("LSan: %d leak(s) found\n", leaks_found);
    }

    return leaks_found;
}

int __lsan_do_leak_check(void) {
    return lsan_do_check_internal(1);
}

int __lsan_do_recoverable_leak_check(void) {
    return lsan_do_check_internal(0);
}
