/* Misan AddressSanitizer
 * Works w/ GCC/Clang -fsanitize=address
 */

#ifndef MISAN_ASAN_INTERFACE_H
#define MISAN_ASAN_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void __asan_poison_memory_region(void const volatile *addr, size_t size);
void __asan_unpoison_memory_region(void const volatile *addr, size_t size);

int __asan_address_is_poisoned(void const volatile *addr);
void __asan_describe_address(void *addr);

typedef struct {
    uintptr_t beg;
    uintptr_t size;
    uintptr_t size_with_redzone;
    const char *name;
    const char *module_name;
    uintptr_t has_dynamic_init;
    void *source_location;
    uintptr_t odr_indicator;
} __asan_global;

void __asan_register_globals(__asan_global *globals, size_t n);
void __asan_unregister_globals(__asan_global *globals, size_t n);

void __asan_init(void);
void __asan_version_mismatch_check_v8(void);

void *__asan_stack_malloc_0(size_t size);
void *__asan_stack_malloc_1(size_t size);
void *__asan_stack_malloc_2(size_t size);
void *__asan_stack_malloc_3(size_t size);
void *__asan_stack_malloc_4(size_t size);
void *__asan_stack_malloc_5(size_t size);

void __asan_stack_free_0(void *ptr, size_t size);
void __asan_stack_free_1(void *ptr, size_t size);
void __asan_stack_free_2(void *ptr, size_t size);
void __asan_stack_free_3(void *ptr, size_t size);
void __asan_stack_free_4(void *ptr, size_t size);
void __asan_stack_free_5(void *ptr, size_t size);

void __asan_before_dynamic_init(const char *module_name);
void __asan_after_dynamic_init(void);
void __asan_handle_no_return(void);

void __asan_report_load1(void *addr);
void __asan_report_load2(void *addr);
void __asan_report_load4(void *addr);
void __asan_report_load8(void *addr);
void __asan_report_load16(void *addr);
void __asan_report_load_n(void *addr, size_t size);

void __asan_report_store1(void *addr);
void __asan_report_store2(void *addr);
void __asan_report_store4(void *addr);
void __asan_report_store8(void *addr);
void __asan_report_store16(void *addr);
void __asan_report_store_n(void *addr, size_t size);

void __asan_load1(void *addr);
void __asan_load2(void *addr);
void __asan_load4(void *addr);
void __asan_load8(void *addr);
void __asan_load16(void *addr);

void __asan_store1(void *addr);
void __asan_store2(void *addr);
void __asan_store4(void *addr);
void __asan_store8(void *addr);
void __asan_store16(void *addr);

#define ASAN_SHADOW_SCALE 3
#define ASAN_SHADOW_OFFSET 0x00007fff8000ULL

#ifdef __cplusplus
}
#endif

#endif /* MISAN_ASAN_INTERFACE_H */
