/* Misan ThreadSanitizer
 * Works w/ GCC/Clang -fsanitize=thread
 */

#ifndef MISAN_TSAN_INTERFACE_H
#define MISAN_TSAN_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void __tsan_init(void);
void __tsan_func_entry(void *pc);
void __tsan_func_exit(void);

void __tsan_read1(void *addr);
void __tsan_read2(void *addr);
void __tsan_read4(void *addr);
void __tsan_read8(void *addr);
void __tsan_read16(void *addr);

void __tsan_write1(void *addr);
void __tsan_write2(void *addr);
void __tsan_write4(void *addr);
void __tsan_write8(void *addr);
void __tsan_write16(void *addr);

void __tsan_read_range(void *addr, void *end);
void __tsan_write_range(void *addr, void *end);

int8_t  __tsan_atomic8_load(const void *addr, int mo);
int16_t __tsan_atomic16_load(const void *addr, int mo);
int32_t __tsan_atomic32_load(const void *addr, int mo);
int64_t __tsan_atomic64_load(const void *addr, int mo);

void __tsan_atomic8_store(void *addr, int8_t val, int mo);
void __tsan_atomic16_store(void *addr, int16_t val, int mo);
void __tsan_atomic32_store(void *addr, int32_t val, int mo);
void __tsan_atomic64_store(void *addr, int64_t val, int mo);

void __tsan_mutex_create(void *addr);
void __tsan_mutex_destroy(void *addr);
void __tsan_mutex_pre_lock(void *addr, int rw);
void __tsan_mutex_post_lock(void *addr, int rw);
void __tsan_mutex_pre_unlock(void *addr, int rw);
void __tsan_mutex_post_unlock(void *addr, int rw);

void __tsan_vptr_read(void *addr);
void __tsan_vptr_update(void *addr, void *new_val);

#ifdef __cplusplus
}
#endif

#endif /* MISAN_TSAN_INTERFACE_H */
