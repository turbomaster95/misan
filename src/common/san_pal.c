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
    (void)addr;
    (void)size;
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
