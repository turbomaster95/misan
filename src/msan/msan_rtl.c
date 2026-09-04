/* Misan MemorySanitizer runtime
 * Shadow memory tracking for uninitialized reads.
 */

#include "sanitizer/common.h"
#include "sanitizer/msan_interface.h"

#define MSAN_SHADOW_SCALE 0
#define MSAN_ORIGIN_ENABLED 1

#define MSAN_SHADOW_BASE  0xFFFF700000000000ULL
#define MSAN_ORIGIN_BASE  0xFFFF600000000000ULL

#define MSAN_INITIALIZED   0x00
#define MSAN_UNINITIALIZED 0xFF

#define MSAN_MAX_ORIGINS 65536
#define MSAN_ORIGIN_MASK 0xFFFF

typedef struct {
    uint32_t id;
    drt_stack_trace_t stack;
    uint64_t timestamp;
    int active;
} msan_origin_t;

static msan_origin_t msan_origins[MSAN_MAX_ORIGINS];
static volatile uint32_t msan_origin_counter = 1;
static volatile int msan_initialized = 0;
static volatile int msan_track_origins = 1;
static drt_spinlock_t msan_lock = MISAN_SPINLOCK_INIT;

static inline unsigned char *msan_shadow_ptr(void *addr) {
    return (unsigned char *)(MSAN_SHADOW_BASE + (uintptr_t)addr);
}

static inline uint32_t *msan_origin_ptr(void *addr) {
    return (uint32_t *)(MSAN_ORIGIN_BASE + ((uintptr_t)addr * sizeof(uint32_t)));
}

void __msan_init(void) {
    if (__atomic_exchange_n((int *)&msan_initialized, 1, __ATOMIC_SEQ_CST))
        return;

    drt_arch_map_shadow_memory(MSAN_SHADOW_BASE, 1ULL << 47);
    drt_arch_map_shadow_memory(MSAN_ORIGIN_BASE, 1ULL << 47);
}

void __msan_unpoison(void *addr, size_t size) {
    if (!addr || !size) return;
    unsigned char *shadow = msan_shadow_ptr(addr);
    for (size_t i = 0; i < size; i++) {
        shadow[i] = MSAN_INITIALIZED;
    }
    if (msan_track_origins) {
        uint32_t *origin = msan_origin_ptr(addr);
        for (size_t i = 0; i < size; i++) {
            origin[i] = 0;
        }
    }
}

void __msan_poison(void *addr, size_t size) {
    if (!addr || !size) return;
    unsigned char *shadow = msan_shadow_ptr(addr);
    for (size_t i = 0; i < size; i++) {
        shadow[i] = MSAN_UNINITIALIZED;
    }
}

void __msan_set_origin(void *addr, size_t size, uint32_t origin_id) {
    if (!msan_track_origins || !addr || !size) return;
    uint32_t *origin = msan_origin_ptr(addr);
    for (size_t i = 0; i < size; i++) {
        origin[i] = origin_id;
    }
}

static uint32_t msan_alloc_origin(void) {
    uint32_t id = drt_atomic_add((int *)&msan_origin_counter, 1);
    if (id >= MSAN_MAX_ORIGINS) {
        /* Wrap around, reuse old origins */
        id = 1;
        drt_atomic_store((int *)&msan_origin_counter, 2);
    }
    msan_origins[id].id = id;
    msan_origins[id].active = 1;
    msan_origins[id].timestamp = drt_arch_get_time_ns();
    drt_arch_capture_stack_trace(&msan_origins[id].stack, 2);
    return id;
}

uint32_t __msan_chain_origin(uint32_t origin) {
    if (!origin) return 0;
    uint32_t new_id = msan_alloc_origin();
    drt_spin_lock(&msan_lock);
    msan_origins[new_id].stack = msan_origins[origin].stack;
    drt_spin_unlock(&msan_lock);
    return new_id;
}

int __msan_get_track_origins(void) {
    return msan_track_origins;
}

static void msan_report_uninit(void *addr, size_t size) {
    drt_spin_lock(&msan_lock);
    drt_printf("\n==MSAN: USE OF UNINITIALIZED MEMORY==\n");
    drt_printf("  Address: %p  Size: %zu\n", addr, size);

    if (msan_track_origins) {
        uint32_t *origin = msan_origin_ptr(addr);
        uint32_t oid = origin[0];
        if (oid && oid < MSAN_MAX_ORIGINS && msan_origins[oid].active) {
            drt_printf("  Origin: #%u  Timestamp: %llu\n",
                       oid, (unsigned long long)msan_origins[oid].timestamp);
            drt_puts("  Origin stack:\n");
            drt_print_stack_trace(&msan_origins[oid].stack);
        }
    }

    drt_stack_trace_t st;
    drt_arch_capture_stack_trace(&st, 2);
    drt_puts("  Access stack:\n");
    drt_print_stack_trace(&st);

    drt_spin_unlock(&msan_lock);
}

void __msan_check_mem_is_initialized(void *addr, size_t size) {
    if (!addr || !size) return;
    unsigned char *shadow = msan_shadow_ptr(addr);
    for (size_t i = 0; i < size; i++) {
        if (shadow[i] != MSAN_INITIALIZED) {
            msan_report_uninit((char *)addr + i, size - i);
            drt_arch_abort();
            __builtin_unreachable();
        }
    }
}

void __msan_warning(void) {
    drt_stack_trace_t st;
    drt_arch_capture_stack_trace(&st, 1);
    drt_printf("\n==MSAN WARNING==\n");
    drt_print_stack_trace(&st);
}

void __msan_warning_noreturn(void) {
    __msan_warning();
    drt_arch_abort();
    __builtin_unreachable();
}
