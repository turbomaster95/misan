/* Misan Platform Abstraction Layer
 * Default weak implementations. Override by linking to your OS
 * implementation before libdrt_*.a or by defining strong symbols.
 */

#include "sanitizer/common.h"

MISAN_WEAK void drt_arch_print_string(const char *str) {
    volatile unsigned char *uart = (volatile unsigned char *)0x3F8;
    while (*str) {
        *uart = *str++;
        for (volatile int i = 0; i < 100; i++) __asm__ volatile("" ::: "memory");
    }
}

MISAN_WEAK MISAN_NORETURN void drt_arch_abort(void) {
    __asm__ volatile (
        "cli\n"
        "1: hlt\n"
        "jmp 1b\n"
        ::: "memory"
    );
    __builtin_unreachable();
}

MISAN_WEAK int drt_arch_map_shadow_memory(uintptr_t addr, size_t size) {
    if (addr == 0) {
        addr = 0x7fff8000;
    }

    if (size == 0) {
        size = 0x100000000000ULL - addr;
    }

    extern void *mmap(void *addr, size_t length, int prot, int flags, int fd, long offset);

    #define MAP_FAILED ((void *) -1)

    #define MAP_PRIVATE    0x02
    #define MAP_FIXED      0x10
    #define MAP_ANON       0x20
    #define MAP_ANONYMOUS  MAP_ANON
    #define MAP_NORESERVE  0x4000
    #define MAP_FIXED_NOREPLACE 0x100000

    #define PROT_READ      1
    #define PROT_WRITE     2

    void *p = mmap((void *)addr, (size_t)size,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE,
                   -1, 0);

    if (p == MAP_FAILED) {
        p = mmap((void *)addr, (size_t)size,
                 PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED,
                 -1, 0);
    }

    if (p == MAP_FAILED) {
        return -1;
    }

    return 0;
}

MISAN_WEAK uint64_t drt_arch_get_time_ns(void) {
    unsigned lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

MISAN_WEAK void drt_arch_yield(void) {
    __asm__ volatile ("pause" ::: "memory");
}

MISAN_WEAK void drt_arch_capture_stack_trace(drt_stack_trace_t *out, size_t skip) {
    uintptr_t *rbp;
    __asm__ volatile ("mov %%rbp, %0" : "=r"(rbp));
    out->count = 0;
    while (rbp && out->count < MISAN_MAX_STACK_FRAMES) {
        if (skip) {
            skip--;
        } else {
            out->pcs[out->count++] = *(rbp + 1);
        }
        rbp = (uintptr_t *)*rbp;
    }
}
