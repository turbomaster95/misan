/* Misan LeakSanitizer public interface
 * Works w/ GCC/Clang -fsanitize=leak
 */

#ifndef MISAN_LSAN_INTERFACE_H
#define MISAN_LSAN_INTERFACE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void __lsan_register_root_region(const void *p, size_t size);
void __lsan_unregister_root_region(const void *p, size_t size);

void __lsan_ignore_object(const void *p);

void __lsan_disable(void);
void __lsan_enable(void);

int __lsan_do_leak_check(void);              /* 0 = no leaks, 1 = leaks found */
int __lsan_do_recoverable_leak_check(void);  /* non-fatal variant */

#ifdef __cplusplus
}
#endif

#endif /* MISAN_LSAN_INTERFACE_H */
