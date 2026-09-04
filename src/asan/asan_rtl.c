// Misan AddressSanitizer runtime

#include "sanitizer/common.h"
#include "sanitizer/asan_interface.h"

#define ASAN_SHADOW_SCALE    3
#define ASAN_SHADOW_GRAIN    (1ULL << ASAN_SHADOW_SCALE)   /* 8 bytes */
#define ASAN_SHADOW_MASK     (ASAN_SHADOW_GRAIN - 1)

#ifndef ASAN_SHADOW_BASE
#define ASAN_SHADOW_BASE     0xFFFF800000000000ULL
#endif

#define ASAN_SHADOW_UNPOISONED    0x00
#define ASAN_SHADOW_REDZONE       0xFA
#define ASAN_SHADOW_STACK_LEFT    0xF1
#define ASAN_SHADOW_STACK_MID     0xF2
#define ASAN_SHADOW_STACK_RIGHT   0xF3
#define ASAN_SHADOW_HEAP_LEFT     0xF5
#define ASAN_SHADOW_HEAP_RIGHT    0xF6
#define ASAN_SHADOW_GLOBAL_REDZONE 0xF9
#define ASAN_SHADOW_POISONED      0xFF

static volatile int asan_initialized = 0;
static drt_spinlock_t asan_lock = MISAN_SPINLOCK_INIT;

#define ASAN_FAKE_STACK_SIZE  (64 * 1024)
#define ASAN_FAKE_STACK_COUNT 16

typedef struct {
    unsigned char mem[ASAN_FAKE_STACK_SIZE];
    volatile int in_use;
    size_t alloc_size;
} asan_fake_stack_t;

static asan_fake_stack_t asan_fake_stacks[ASAN_FAKE_STACK_COUNT];

static inline uintptr_t asan_mem_to_shadow(uintptr_t addr) {
    return ASAN_SHADOW_BASE + (addr >> ASAN_SHADOW_SCALE);
}

static inline uintptr_t asan_shadow_to_mem(uintptr_t shadow) {
    return (shadow - ASAN_SHADOW_BASE) << ASAN_SHADOW_SCALE;
}

static inline unsigned char *asan_shadow_ptr(uintptr_t addr) {
    return (unsigned char *)asan_mem_to_shadow(addr);
}

static inline unsigned char asan_shadow_get(uintptr_t addr) {
    return *asan_shadow_ptr(addr);
}

static inline void asan_shadow_set(uintptr_t addr, unsigned char val) {
    *asan_shadow_ptr(addr) = val;
}

static void asan_poison_range(uintptr_t beg, size_t size, unsigned char val) {
    if (size == 0) return;
    uintptr_t end = beg + size;
    uintptr_t shadow_beg = asan_mem_to_shadow(beg);
    uintptr_t shadow_end = asan_mem_to_shadow(end - 1) + 1;

    for (uintptr_t s = shadow_beg; s < shadow_end; s++) {
        *(unsigned char *)s = val;
    }

    uintptr_t grain_beg = beg & ~ASAN_SHADOW_MASK;
    if (grain_beg != beg) {
        unsigned char partial = (unsigned char)((beg - grain_beg) & 0x7);
        *(unsigned char *)asan_mem_to_shadow(grain_beg) = partial;
    }
}

static void asan_unpoison_range(uintptr_t beg, size_t size) {
    if (size == 0) return;
    uintptr_t end = beg + size;
    uintptr_t shadow_beg = asan_mem_to_shadow(beg);
    uintptr_t shadow_end = asan_mem_to_shadow(end - 1) + 1;

    for (uintptr_t s = shadow_beg; s < shadow_end; s++) {
        *(unsigned char *)s = ASAN_SHADOW_UNPOISONED;
    }
}

static void asan_report_error(const char *kind, void *addr, size_t size,
                            unsigned char shadow_val) {
    drt_spin_lock(&asan_lock);
    drt_printf("\n==ASAN ERROR==\n");
    drt_printf("  %s of size %zu at %p\n", kind, size, addr);
    drt_printf("  Shadow byte value: 0x%02x\n", shadow_val);

    drt_stack_trace_t st;
    drt_arch_capture_stack_trace(&st, 2);
    drt_print_stack_trace(&st);

    drt_spin_unlock(&asan_lock);
    drt_arch_abort();
    __builtin_unreachable();
}

static void asan_check_access(void *addr, size_t size, int is_write) {
    if (!asan_initialized) return;
    uintptr_t a = (uintptr_t)addr;
    uintptr_t grain = a & ~ASAN_SHADOW_MASK;
    unsigned char shadow = asan_shadow_get(grain);

    if (shadow == ASAN_SHADOW_UNPOISONED) return;

    /* Partially addressable grain */
    if (shadow > 0 && shadow < ASAN_SHADOW_GRAIN) {
        size_t accessible = shadow;
        if ((a - grain) + size <= accessible) return;
    }

    asan_report_error(is_write ? "WRITE" : "READ", addr, size, shadow);
}

void __asan_poison_memory_region(void const volatile *addr, size_t size) {
    asan_poison_range((uintptr_t)addr, size, ASAN_SHADOW_POISONED);
}

void __asan_unpoison_memory_region(void const volatile *addr, size_t size) {
    asan_unpoison_range((uintptr_t)addr, size);
}

int __asan_address_is_poisoned(void const volatile *addr) {
    return asan_shadow_get((uintptr_t)addr) != ASAN_SHADOW_UNPOISONED;
}

void __asan_describe_address(void *addr) {
    unsigned char sv = asan_shadow_get((uintptr_t)addr);
    drt_printf("Address %p shadow: 0x%02x\n", addr, sv);
}

void __asan_init(void) {
    if (__atomic_exchange_n((int *)&asan_initialized, 1, __ATOMIC_SEQ_CST))
        return;

    drt_arch_map_shadow_memory(ASAN_SHADOW_BASE, 1ULL << 47);
}

void __asan_version_mismatch_check_v8(void) {
    /* Version 8 is what we implement. No-op. */
}

void __asan_register_globals(__asan_global *globals, size_t n) {
    if (!asan_initialized) __asan_init();
    for (size_t i = 0; i < n; i++) {
        __asan_global *g = &globals[i];
        /* Unpoison the global itself */
        asan_unpoison_range(g->beg, g->size);
        /* Poison redzones */
        asan_poison_range(g->beg + g->size,
                          g->size_with_redzone - g->size,
                          ASAN_SHADOW_GLOBAL_REDZONE);
    }
}

void __asan_unregister_globals(__asan_global *globals, size_t n) {
    for (size_t i = 0; i < n; i++) {
        __asan_global *g = &globals[i];
        asan_poison_range(g->beg, g->size_with_redzone, ASAN_SHADOW_POISONED);
    }
}

void __asan_before_dynamic_init(const char *module_name) {
    (void)module_name;
}

void __asan_after_dynamic_init(void) {
    /* no-op */
}

void __asan_handle_no_return(void) {
    for (int i = 0; i < ASAN_FAKE_STACK_COUNT; i++) {
        if (asan_fake_stacks[i].in_use) {
            asan_poison_range((uintptr_t)asan_fake_stacks[i].mem,
                              ASAN_FAKE_STACK_SIZE, ASAN_SHADOW_POISONED);
        }
    }
}

void *__asan_stack_malloc_0(size_t size) { (void)size; return 0; }
void *__asan_stack_malloc_1(size_t size) { (void)size; return 0; }
void *__asan_stack_malloc_2(size_t size) { (void)size; return 0; }
void *__asan_stack_malloc_3(size_t size) { (void)size; return 0; }
void *__asan_stack_malloc_4(size_t size) { (void)size; return 0; }
void *__asan_stack_malloc_5(size_t size) { (void)size; return 0; }

void __asan_stack_free_0(void *ptr, size_t size) { (void)ptr; (void)size; }
void __asan_stack_free_1(void *ptr, size_t size) { (void)ptr; (void)size; }
void __asan_stack_free_2(void *ptr, size_t size) { (void)ptr; (void)size; }
void __asan_stack_free_3(void *ptr, size_t size) { (void)ptr; (void)size; }
void __asan_stack_free_4(void *ptr, size_t size) { (void)ptr; (void)size; }
void __asan_stack_free_5(void *ptr, size_t size) { (void)ptr; (void)size; }

static void *asan_fake_stack_alloc(size_t size, int *idx_out) {
    for (int i = 0; i < ASAN_FAKE_STACK_COUNT; i++) {
        int expected = 0;
        if (__atomic_compare_exchange_n(&asan_fake_stacks[i].in_use,
                                        &expected, 1,
                                        false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
            asan_fake_stacks[i].alloc_size = size;
            asan_unpoison_range((uintptr_t)asan_fake_stacks[i].mem, size);
            *idx_out = i;
            return asan_fake_stacks[i].mem;
        }
    }
    *idx_out = -1;
    return 0; /* fallback: allocate on real stack */
}

static void asan_fake_stack_free(int idx, size_t size) {
    if (idx < 0 || idx >= ASAN_FAKE_STACK_COUNT) return;
    asan_poison_range((uintptr_t)asan_fake_stacks[idx].mem, size, ASAN_SHADOW_STACK_LEFT);
    __atomic_store_n(&asan_fake_stacks[idx].in_use, 0, __ATOMIC_SEQ_CST);
}
