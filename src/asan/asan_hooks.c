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
