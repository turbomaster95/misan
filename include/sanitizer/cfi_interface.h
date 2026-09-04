/* Misan Control Flow Integrity / Shadow Call Stack
 * Works w/ GCC/Clang -fsanitize=cfi, -fsanitize=shadow-call-stack
 */

#ifndef MISAN_CFI_INTERFACE_H
#define MISAN_CFI_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void __cfi_slowpath(uint64_t call_site_type, void *addr);
void __cfi_slowpath_diag(uint64_t call_site_type, void *addr, void *diag_data);

void __cfi_fail(void);

void __sanitizer_cfi_bad_type(void *diag_data, void *vtable, void *valid_vtable,
                              void *caller_pc, void *tag);

void __sanitizer_cfi_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MISAN_CFI_INTERFACE_H */
