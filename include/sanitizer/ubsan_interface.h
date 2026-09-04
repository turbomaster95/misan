/* Misan UndefinedBehaviorSanitizer
 * Works w/ GCC/Clang -fsanitize=undefined
 */

#ifndef MISAN_UBSAN_INTERFACE_H
#define MISAN_UBSAN_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *filename;
    uint32_t line;
    uint32_t column;
} __ubsan_source_location;

typedef struct {
    uint16_t type_kind;
    uint16_t type_info;
    char type_name[1];
} __ubsan_type_descriptor;

typedef struct {
    __ubsan_source_location *location;
    __ubsan_type_descriptor *type;
    uint8_t log_alignment;
    uint8_t type_check_kind;
} __ubsan_type_mismatch_data;

typedef struct {
    __ubsan_source_location *location;
    __ubsan_type_descriptor *type;
} __ubsan_overflow_data;

typedef struct {
    __ubsan_source_location *location;
    __ubsan_type_descriptor *left_type;
    __ubsan_type_descriptor *right_type;
} __ubsan_shift_out_of_bounds_data;

typedef struct {
    __ubsan_source_location *location;
    __ubsan_type_descriptor *array_type;
    __ubsan_type_descriptor *index_type;
} __ubsan_out_of_bounds_data;

typedef struct {
    __ubsan_source_location *location;
    __ubsan_type_descriptor *type;
} __ubsan_vla_bound_data;

typedef struct {
    __ubsan_source_location *location;
    __ubsan_type_descriptor *type;
    uint8_t kind;
} __ubsan_invalid_value_data;

typedef struct {
    __ubsan_source_location *location;
    __ubsan_type_descriptor *type;
    uint8_t alignment;
    uint8_t type_check_kind;
} __ubsan_type_mismatch_data_v1;

typedef struct {
    __ubsan_source_location *location;
    __ubsan_type_descriptor *from_type;
    __ubsan_type_descriptor *to_type;
    uint8_t kind;
} __ubsan_implicit_conversion_data;

typedef struct {
    __ubsan_source_location *location;
    __ubsan_source_location *attr_location;
    int arg_index;
} __ubsan_nonnull_arg_data;

typedef struct {
    __ubsan_source_location *location;
} __ubsan_pointer_overflow_data;

typedef struct {
    __ubsan_source_location location;
    __ubsan_type_descriptor *type;
} __ubsan_function_type_mismatch_data;

void __ubsan_handle_function_type_mismatch(void *data_raw, void *function);
void __ubsan_handle_function_type_mismatch_abort(void *data_raw, void *function);
void __ubsan_handle_type_mismatch(void *data, void *pointer);
void __ubsan_handle_type_mismatch_v1(void *data, void *pointer);
void __ubsan_handle_add_overflow(void *data, void *lhs, void *rhs);
void __ubsan_handle_sub_overflow(void *data, void *lhs, void *rhs);
void __ubsan_handle_mul_overflow(void *data, void *lhs, void *rhs);
void __ubsan_handle_negate_overflow(void *data, void *old_val);
void __ubsan_handle_divrem_overflow(void *data, void *lhs, void *rhs);
void __ubsan_handle_shift_out_of_bounds(void *data, void *lhs, void *rhs);
void __ubsan_handle_out_of_bounds(void *data, void *index);
void __ubsan_handle_vla_bound_not_positive(void *data, void *bound);
void __ubsan_handle_invalid_builtin(void *data);
void __ubsan_handle_invalid_value(void *data, void *value);
void __ubsan_handle_nonnull_arg(void *data);
void __ubsan_handle_implicit_conversion(void *data, void *from, void *to);
void __ubsan_handle_pointer_overflow(void *data, void *base, void *result);
void __ubsan_handle_builtin_unreachable(void *data);

void __ubsan_handle_type_mismatch_abort(void *data, void *pointer);
void __ubsan_handle_type_mismatch_v1_abort(void *data, void *pointer);
void __ubsan_handle_add_overflow_abort(void *data, void *lhs, void *rhs);
void __ubsan_handle_sub_overflow_abort(void *data, void *lhs, void *rhs);
void __ubsan_handle_mul_overflow_abort(void *data, void *lhs, void *rhs);
void __ubsan_handle_negate_overflow_abort(void *data, void *old_val);
void __ubsan_handle_divrem_overflow_abort(void *data, void *lhs, void *rhs);
void __ubsan_handle_shift_out_of_bounds_abort(void *data, void *lhs, void *rhs);
void __ubsan_handle_out_of_bounds_abort(void *data, void *index);
void __ubsan_handle_vla_bound_not_positive_abort(void *data, void *bound);
void __ubsan_handle_invalid_builtin_abort(void *data);
void __ubsan_handle_invalid_value_abort(void *data, void *value);
void __ubsan_handle_nonnull_arg_abort(void *data);
void __ubsan_handle_implicit_conversion_abort(void *data, void *from, void *to);
void __ubsan_handle_pointer_overflow_abort(void *data, void *base, void *result);
void __ubsan_handle_builtin_unreachable_abort(void *data);

#ifdef __cplusplus
}
#endif

#endif /* MISAN_UBSAN_INTERFACE_H */
