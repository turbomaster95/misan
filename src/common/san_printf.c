/* Misan printf implementation
 * Supports %d %u %x %p %s %c %l %ll %z %%.
 */

#include "sanitizer/common.h"
#include <stdint.h>
#include <stdarg.h>

static void drt_putchar(char c) {
    char s[2] = {c, '\0'};
    drt_arch_print_string(s);
}

void drt_puts(const char *s) {
    drt_arch_print_string(s);
}

static void drt_putn(unsigned long long n, int base, int width, char pad, int upper) {
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char buf[32];
    int i = 0;
    do {
        buf[i++] = digits[n % base];
        n /= base;
    } while (n);
    while (i < width) drt_putchar(pad);
    while (i--) drt_putchar(buf[i]);
}

static void drt_putsn(long long n, int base, int width, char pad) {
    if (n < 0) {
        drt_putchar('-');
        n = -n;
        if (width > 0) width--;
    }
    drt_putn((unsigned long long)n, base, width, pad, 0);
}

void drt_vprintf(const char *fmt, va_list ap) {
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            drt_putchar(*p);
            continue;
        }
        p++;
        char pad = ' ';
        int width = 0;
        if (*p == '0') { pad = '0'; p++; }
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p++;
        }

        int longness = 0;
        if (*p == 'l') { longness = 1; p++; }
        if (*p == 'l') { longness = 2; p++; }
        if (*p == 'z') { longness = 3; p++; }

        switch (*p) {
            case 'd': {
                long long v;
                if (longness == 2) v = va_arg(ap, long long);
                else if (longness == 1) v = va_arg(ap, long);
                else if (longness == 3) v = va_arg(ap, ptrdiff_t);
                else v = va_arg(ap, int);
                drt_putsn(v, 10, width, pad);
                break;
            }
            case 'u': {
                unsigned long long v;
                if (longness == 2) v = va_arg(ap, unsigned long long);
                else if (longness == 1) v = va_arg(ap, unsigned long);
                else if (longness == 3) v = va_arg(ap, size_t);
                else v = va_arg(ap, unsigned int);
                drt_putn(v, 10, width, pad, 0);
                break;
            }
            case 'x': {
                unsigned long long v;
                if (longness == 2) v = va_arg(ap, unsigned long long);
                else if (longness == 1) v = va_arg(ap, unsigned long);
                else if (longness == 3) v = va_arg(ap, size_t);
                else v = va_arg(ap, unsigned int);
                drt_putn(v, 16, width, pad, 0);
                break;
            }
            case 'X': {
                unsigned long long v;
                if (longness == 2) v = va_arg(ap, unsigned long long);
                else if (longness == 1) v = va_arg(ap, unsigned long);
                else if (longness == 3) v = va_arg(ap, size_t);
                else v = va_arg(ap, unsigned int);
                drt_putn(v, 16, width, pad, 1);
                break;
            }
            case 'p': {
                drt_puts("0x");
                drt_putn((uintptr_t)va_arg(ap, void *), 16, sizeof(void*) * 2, '0', 0);
                break;
            }
            case 's': {
                const char *s = va_arg(ap, const char *);
                if (!s) s = "(null)";
                drt_puts(s);
                break;
            }
            case 'c': {
                int c = va_arg(ap, int);
                drt_putchar((char)c);
                break;
            }
            case '%':
                drt_putchar('%');
                break;
            default:
                drt_putchar('%');
                drt_putchar(*p);
                break;
        }
    }
}

void drt_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    drt_vprintf(fmt, ap);
    va_end(ap);
}

void drt_die(const char *fmt, ...) {
    drt_puts("\n[MISAN FATAL] ");
    va_list ap;
    va_start(ap, fmt);
    drt_vprintf(fmt, ap);
    va_end(ap);
    drt_puts("\n");
    drt_arch_abort();
    __builtin_unreachable();
}

void drt_print_stack_trace(const drt_stack_trace_t *st) {
    drt_puts("Stack trace:\n");
    for (size_t i = 0; i < st->count; i++) {
        drt_printf("  #%zu  0x%p\n", i, (void *)st->pcs[i]);
    }
}

static alignas(4096) unsigned char drt_bump_pool[256 * 1024];
static volatile size_t drt_bump_used = 0;

void *drt_internal_alloc(size_t size) {
    size_t align = sizeof(void *);
    size_t mask = align - 1;
    size_t old, newv;
    do {
        old = __atomic_load_n(&drt_bump_used, __ATOMIC_SEQ_CST);
        newv = (old + mask) & ~mask;
        newv += size;
        if (newv > sizeof(drt_bump_pool)) {
            drt_die("drt_internal_alloc: out of memory");
        }
    } while (!__atomic_compare_exchange_n(&drt_bump_used, &old, newv,
                                          false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
    return &drt_bump_pool[(newv - size)];
}

void drt_internal_free(void *ptr) {
    (void)ptr;
}

void drt_internal_alloc_reset(void) {
    __atomic_store_n(&drt_bump_used, 0, __ATOMIC_SEQ_CST);
}
