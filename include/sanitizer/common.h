// Misan Common — Freestanding sanitizer runtime infrastructure

#ifndef MISAN_COMMON_H
#define MISAN_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#define MISAN_NORETURN    __attribute__((noreturn))
#define MISAN_NOINLINE    __attribute__((noinline))
#define MISAN_WEAK        __attribute__((weak))
#define MISAN_HIDDEN      __attribute__((visibility("hidden")))
#define MISAN_EXPORT      __attribute__((visibility("default")))
#define MISAN_ALIGNED(x)  __attribute__((aligned(x)))
#define MISAN_SECTION(x)  __attribute__((section(x)))
#define MISAN_CONSTRUCTOR __attribute__((constructor))
#define MISAN_DESTRUCTOR  __attribute__((destructor))

static inline int drt_atomic_add(int *ptr, int val) {
    return __atomic_add_fetch(ptr, val, __ATOMIC_SEQ_CST);
}

static inline int drt_atomic_sub(int *ptr, int val) {
    return __atomic_sub_fetch(ptr, val, __ATOMIC_SEQ_CST);
}

static inline int drt_atomic_load(const int *ptr) {
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static inline void drt_atomic_store(int *ptr, int val) {
    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);
}

static inline bool drt_atomic_cmpxchg(uintptr_t *ptr, uintptr_t expected, uintptr_t desired) {
    return __atomic_compare_exchange_n(ptr, &expected, desired, false,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

typedef struct {
    volatile int lock;
} drt_spinlock_t;

#define MISAN_SPINLOCK_INIT {0}

static inline void drt_spin_lock(drt_spinlock_t *sl) {
    while (__atomic_test_and_set(&sl->lock, __ATOMIC_SEQ_CST))
        __asm__ volatile("pause" ::: "memory");
}

static inline void drt_spin_unlock(drt_spinlock_t *sl) {
    __atomic_clear(&sl->lock, __ATOMIC_SEQ_CST);
}

MISAN_WEAK void drt_arch_print_string(const char *str);

MISAN_WEAK MISAN_NORETURN void drt_arch_abort(void);

MISAN_WEAK int drt_arch_map_shadow_memory(uintptr_t addr, size_t size);

MISAN_WEAK uint64_t drt_arch_get_time_ns(void);

MISAN_WEAK void drt_arch_yield(void);

void drt_vprintf(const char *fmt, __builtin_va_list ap);
void drt_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void drt_puts(const char *s);

MISAN_NORETURN void drt_die(const char *fmt, ...);

static inline void drt_memset(void *dst, int c, size_t n) {
    unsigned char *d = dst;
    while (n--) *d++ = (unsigned char)c;
}

static inline void drt_memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
}

static inline int drt_memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *pa = a, *pb = b;
    while (n--) {
        if (*pa != *pb) return (int)*pa - (int)*pb;
        pa++; pb++;
    }
    return 0;
}

static inline size_t drt_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static inline int drt_strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

#define MISAN_MAX_STACK_FRAMES 32

typedef struct {
    uintptr_t pcs[MISAN_MAX_STACK_FRAMES];
    size_t count;
} drt_stack_trace_t;

MISAN_WEAK void drt_arch_capture_stack_trace(drt_stack_trace_t *out, size_t skip);

void drt_print_stack_trace(const drt_stack_trace_t *st);

void *drt_internal_alloc(size_t size);
void drt_internal_free(void *ptr);
void drt_internal_alloc_reset(void);

#endif /* MISAN_COMMON_H */
