/* Misan MemorySanitizer
 * Works w/ GCC/Clang -fsanitize=memory
 */

#ifndef MISAN_MSAN_INTERFACE_H
#define MISAN_MSAN_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void __msan_init(void);

void __msan_unpoison(void *addr, size_t size);
void __msan_poison(void *addr, size_t size);
void __msan_set_origin(void *addr, size_t size, uint32_t origin);

void __msan_check_mem_is_initialized(void *addr, size_t size);

void __msan_warning(void);
void __msan_warning_noreturn(void) __attribute__((noreturn));

int __msan_get_track_origins(void);

uint32_t __msan_chain_origin(uint32_t origin);

#ifdef __cplusplus
}
#endif

#endif /* MISAN_MSAN_INTERFACE_H */
