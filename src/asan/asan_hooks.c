/* Misan AddressSanitizer instrumented-access hooks
 * Called by compiler-instrumented code on every memory access.
 */

#include "sanitizer/common.h"
#include "sanitizer/asan_interface.h"

extern volatile int asan_initialized;
extern void asan_check_access(void *addr, size_t size, int is_write);

void __asan_report_load1(void *addr)  { asan_check_access(addr, 1, 0); }
void __asan_report_load2(void *addr)  { asan_check_access(addr, 2, 0); }
void __asan_report_load4(void *addr)  { asan_check_access(addr, 4, 0); }
void __asan_report_load8(void *addr)  { asan_check_access(addr, 8, 0); }
void __asan_report_load16(void *addr) { asan_check_access(addr, 16, 0); }

void __asan_report_load_n(void *addr, size_t size) {
    asan_check_access(addr, size, 0);
}

void __asan_report_store1(void *addr)  { asan_check_access(addr, 1, 1); }
void __asan_report_store2(void *addr)  { asan_check_access(addr, 2, 1); }
void __asan_report_store4(void *addr)  { asan_check_access(addr, 4, 1); }
void __asan_report_store8(void *addr)  { asan_check_access(addr, 8, 1); }
void __asan_report_store16(void *addr) { asan_check_access(addr, 16, 1); }

void __asan_report_store_n(void *addr, size_t size) {
    asan_check_access(addr, size, 1);
}

void __asan_load1(void *addr)  { if (asan_initialized) asan_check_access(addr, 1, 0); }
void __asan_load2(void *addr)  { if (asan_initialized) asan_check_access(addr, 2, 0); }
void __asan_load4(void *addr)  { if (asan_initialized) asan_check_access(addr, 4, 0); }
void __asan_load8(void *addr)  { if (asan_initialized) asan_check_access(addr, 8, 0); }
void __asan_load16(void *addr) { if (asan_initialized) asan_check_access(addr, 16, 0); }

void __asan_store1(void *addr)  { if (asan_initialized) asan_check_access(addr, 1, 1); }
void __asan_store2(void *addr)  { if (asan_initialized) asan_check_access(addr, 2, 1); }
void __asan_store4(void *addr)  { if (asan_initialized) asan_check_access(addr, 4, 1); }
void __asan_store8(void *addr)  { if (asan_initialized) asan_check_access(addr, 8, 1); }
void __asan_store16(void *addr) { if (asan_initialized) asan_check_access(addr, 16, 1); }

void* __asan_memcpy(void *dest, const void *src, size_t n) {
    drt_memcpy(dest, src, n);
    return dest;
}

void* __asan_memset(void *s, int c, size_t n) {
    drt_memset(s, c, n);
    return s;
}

void __asan_register_elf_globals(void *flag, void *start, void *end) {}
void __asan_unregister_elf_globals(void *flag, void *start, void *end) {}

int __asan_option_detect_stack_use_after_return = 0;

void *__asan_stack_malloc_0(size_t size, void *addr) { (void)size; (void)addr; return NULL; }
void *__asan_stack_malloc_1(size_t size, void *addr) { (void)size; (void)addr; return NULL; }
void *__asan_stack_malloc_2(size_t size, void *addr) { (void)size; (void)addr; return NULL; }
void *__asan_stack_malloc_3(size_t size, void *addr) { (void)size; (void)addr; return NULL; }
void *__asan_stack_malloc_4(size_t size, void *addr) { (void)size; (void)addr; return NULL; }
void *__asan_stack_malloc_5(size_t size, void *addr) { (void)size; (void)addr; return NULL; }
void *__asan_stack_malloc_6(size_t size, void *addr) { (void)size; (void)addr; return NULL; }
void *__asan_stack_malloc_7(size_t size, void *addr) { (void)size; (void)addr; return NULL; }
void *__asan_stack_malloc_8(size_t size, void *addr) { (void)size; (void)addr; return NULL; }
void *__asan_stack_malloc_9(size_t size, void *addr) { (void)size; (void)addr; return NULL; }
void *__asan_stack_malloc_10(size_t size, void *addr) { (void)size; (void)addr; return NULL; }

void __asan_stack_free_0(void *ptr, size_t size, void *addr) { (void)ptr; (void)size; (void)addr; }
void __asan_stack_free_1(void *ptr, size_t size, void *addr) { (void)ptr; (void)size; (void)addr; }
void __asan_stack_free_2(void *ptr, size_t size, void *addr) { (void)ptr; (void)size; (void)addr; }
void __asan_stack_free_3(void *ptr, size_t size, void *addr) { (void)ptr; (void)size; (void)addr; }
void __asan_stack_free_4(void *ptr, size_t size, void *addr) { (void)ptr; (void)size; (void)addr; }
void __asan_stack_free_5(void *ptr, size_t size, void *addr) { (void)ptr; (void)size; (void)addr; }
void __asan_stack_free_6(void *ptr, size_t size, void *addr) { (void)ptr; (void)size; (void)addr; }
void __asan_stack_free_7(void *ptr, size_t size, void *addr) { (void)ptr; (void)size; (void)addr; }
void __asan_stack_free_8(void *ptr, size_t size, void *addr) { (void)ptr; (void)size; (void)addr; }
void __asan_stack_free_9(void *ptr, size_t size, void *addr) { (void)ptr; (void)size; (void)addr; }
void __asan_stack_free_10(void *ptr, size_t size, void *addr) { (void)ptr; (void)size; (void)addr; }
