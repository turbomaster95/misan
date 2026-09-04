/* Misan UndefinedBehaviorSanitizer handlers
 * Full ABI implementation. All handlers report then abort (or return
 * for recoverable variants if MISAN_UBSAN_RECOVER is defined).
 */

#include "sanitizer/common.h"
#include "sanitizer/ubsan_interface.h"

static drt_spinlock_t ubsan_lock = MISAN_SPINLOCK_INIT;

static void ubsan_print_location(const __ubsan_source_location *loc) {
    if (loc && loc->filename) {
        drt_printf("%s:%u:%u", loc->filename, loc->line, loc->column);
    } else {
        drt_puts("<unknown>");
    }
}

static void ubsan_print_type(const __ubsan_type_descriptor *td) {
    if (td) {
        drt_puts(td->type_name);
    } else {
        drt_puts("<unknown type>");
    }
}

#define UBSAN_HANDLE(name, fmt, ...) do { \
    drt_spin_lock(&ubsan_lock); \
    drt_printf("\n==UBSAN: " name "==\n  Location: "); \
    ubsan_print_location(data->location); \
    drt_puts("\n"); \
    drt_printf(fmt, ##__VA_ARGS__); \
    drt_puts("\n"); \
    drt_stack_trace_t st; \
    drt_arch_capture_stack_trace(&st, 2); \
    drt_print_stack_trace(&st); \
    drt_spin_unlock(&ubsan_lock); \
    drt_arch_abort(); \
    __builtin_unreachable(); \
} while (0)

#define UBSAN_HANDLE_RECOVER(name, fmt, ...) do { \
    drt_spin_lock(&ubsan_lock); \
    drt_printf("\n==UBSAN: " name " (recoverable)==\n  Location: "); \
    ubsan_print_location(data->location); \
    drt_puts("\n"); \
    drt_printf fmt; \
    drt_puts("\n"); \
    drt_stack_trace_t st; \
    drt_arch_capture_stack_trace(&st, 2); \
    drt_print_stack_trace(&st); \
    drt_spin_unlock(&ubsan_lock); \
} while (0)

void __ubsan_handle_type_mismatch(void *data_raw, void *pointer) {
    __ubsan_type_mismatch_data *data = data_raw;
    UBSAN_HANDLE("type mismatch",
        "  Pointer: %p  Type: ", pointer);
    ubsan_print_type(data->type);
}

void __ubsan_handle_type_mismatch_abort(void *data_raw, void *pointer) {
    __ubsan_handle_type_mismatch(data_raw, pointer);
}

void __ubsan_handle_type_mismatch_v1(void *data_raw, void *pointer) {
    __ubsan_type_mismatch_data_v1 *data = data_raw;
    UBSAN_HANDLE("type mismatch v1",
        "  Pointer: %p  Alignment: %u  Type check kind: %u  Type: ",
        pointer, data->alignment, data->type_check_kind);
    ubsan_print_type(data->type);
}

void __ubsan_handle_type_mismatch_v1_abort(void *data_raw, void *pointer) {
    __ubsan_handle_type_mismatch_v1(data_raw, pointer);
}

void __ubsan_handle_function_type_mismatch(void *data_raw, void *function) {
    __ubsan_function_type_mismatch_data *data = data_raw;
    (void)function;

    drt_spin_lock(&ubsan_lock);
    drt_printf("\n==UBSAN: function type mismatch==\n  Location: ");
    ubsan_print_location(&data->location);
    drt_puts("\n  Type: ");
    ubsan_print_type(data->type);
    drt_puts("\n");

    drt_stack_trace_t st;
    drt_arch_capture_stack_trace(&st, 2);
    drt_print_stack_trace(&st);
    drt_spin_unlock(&ubsan_lock);
    drt_arch_abort();
    __builtin_unreachable();
}

void __ubsan_handle_function_type_mismatch_abort(void *data_raw, void *function) {
    __ubsan_handle_function_type_mismatch(data_raw, function);
}

void __ubsan_handle_add_overflow(void *data_raw, void *lhs, void *rhs) {
    __ubsan_overflow_data *data = data_raw;
    (void)lhs; (void)rhs;
    UBSAN_HANDLE("add overflow", "  Type: ");
    ubsan_print_type(data->type);
}

void __ubsan_handle_add_overflow_abort(void *data_raw, void *lhs, void *rhs) {
    __ubsan_handle_add_overflow(data_raw, lhs, rhs);
}

void __ubsan_handle_sub_overflow(void *data_raw, void *lhs, void *rhs) {
    __ubsan_overflow_data *data = data_raw;
    (void)lhs; (void)rhs;
    UBSAN_HANDLE("sub overflow", "  Type: ");
    ubsan_print_type(data->type);
}

void __ubsan_handle_sub_overflow_abort(void *data_raw, void *lhs, void *rhs) {
    __ubsan_handle_sub_overflow(data_raw, lhs, rhs);
}

void __ubsan_handle_mul_overflow(void *data_raw, void *lhs, void *rhs) {
    __ubsan_overflow_data *data = data_raw;
    (void)lhs; (void)rhs;
    UBSAN_HANDLE("mul overflow", "  Type: ");
    ubsan_print_type(data->type);
}

void __ubsan_handle_mul_overflow_abort(void *data_raw, void *lhs, void *rhs) {
    __ubsan_handle_mul_overflow(data_raw, lhs, rhs);
}

void __ubsan_handle_negate_overflow(void *data_raw, void *old_val) {
    __ubsan_overflow_data *data = data_raw;
    (void)old_val;
    UBSAN_HANDLE("negate overflow", "  Type: ");
    ubsan_print_type(data->type);
}

void __ubsan_handle_negate_overflow_abort(void *data_raw, void *old_val) {
    __ubsan_handle_negate_overflow(data_raw, old_val);
}

void __ubsan_handle_divrem_overflow(void *data_raw, void *lhs, void *rhs) {
    __ubsan_overflow_data *data = data_raw;
    (void)lhs; (void)rhs;
    UBSAN_HANDLE("divrem overflow", "  Type: ");
    ubsan_print_type(data->type);
}

void __ubsan_handle_divrem_overflow_abort(void *data_raw, void *lhs, void *rhs) {
    __ubsan_handle_divrem_overflow(data_raw, lhs, rhs);
}

void __ubsan_handle_shift_out_of_bounds(void *data_raw, void *lhs, void *rhs) {
    __ubsan_shift_out_of_bounds_data *data = data_raw;
    (void)lhs; (void)rhs;
    UBSAN_HANDLE("shift out of bounds",
        "  Left type: ");
    ubsan_print_type(data->left_type);
    drt_puts("  Right type: ");
    ubsan_print_type(data->right_type);
}

void __ubsan_handle_shift_out_of_bounds_abort(void *data_raw, void *lhs, void *rhs) {
    __ubsan_handle_shift_out_of_bounds(data_raw, lhs, rhs);
}

void __ubsan_handle_out_of_bounds(void *data_raw, void *index) {
    __ubsan_out_of_bounds_data *data = data_raw;
    (void)index;
    UBSAN_HANDLE("out of bounds",
        "  Array type: ");
    ubsan_print_type(data->array_type);
    drt_puts("  Index type: ");
    ubsan_print_type(data->index_type);
}

void __ubsan_handle_out_of_bounds_abort(void *data_raw, void *index) {
    __ubsan_handle_out_of_bounds(data_raw, index);
}

void __ubsan_handle_vla_bound_not_positive(void *data_raw, void *bound) {
    __ubsan_vla_bound_data *data = data_raw;
    (void)bound;
    UBSAN_HANDLE("VLA bound not positive", "  Type: ");
    ubsan_print_type(data->type);
}

void __ubsan_handle_vla_bound_not_positive_abort(void *data_raw, void *bound) {
    __ubsan_handle_vla_bound_not_positive(data_raw, bound);
}

void __ubsan_handle_invalid_value(void *data_raw, void *value) {
    __ubsan_invalid_value_data *data = data_raw;
    (void)value;
    UBSAN_HANDLE("invalid value", "  Type: ");
    ubsan_print_type(data->type);
}

void __ubsan_handle_invalid_value_abort(void *data_raw, void *value) {
    __ubsan_handle_invalid_value(data_raw, value);
}

void __ubsan_handle_invalid_builtin(void *data_raw) {
    __ubsan_invalid_value_data *data = data_raw;
    UBSAN_HANDLE("invalid builtin", "  Kind: %u  Type: ", data->kind);
    ubsan_print_type(data->type);
}

void __ubsan_handle_invalid_builtin_abort(void *data_raw) {
    __ubsan_handle_invalid_builtin(data_raw);
}

void __ubsan_handle_nonnull_arg(void *data_raw) {
    __ubsan_nonnull_arg_data *data = data_raw;
    UBSAN_HANDLE("non-null arg",
        "  Arg index: %d  Attribute location: ", data->arg_index);
    ubsan_print_location(data->attr_location);
}

void __ubsan_handle_nonnull_arg_abort(void *data_raw) {
    __ubsan_handle_nonnull_arg(data_raw);
}

void __ubsan_handle_implicit_conversion(void *data_raw, void *from, void *to) {
    __ubsan_implicit_conversion_data *data = data_raw;
    (void)from; (void)to;
    UBSAN_HANDLE("implicit conversion",
        "  Kind: %u  From type: ", data->kind);
    ubsan_print_type(data->from_type);
    drt_puts("  To type: ");
    ubsan_print_type(data->to_type);
}

void __ubsan_handle_implicit_conversion_abort(void *data_raw, void *from, void *to) {
    __ubsan_handle_implicit_conversion(data_raw, from, to);
}

void __ubsan_handle_pointer_overflow(void *data_raw, void *base, void *result) {
    __ubsan_pointer_overflow_data *data = data_raw;
    UBSAN_HANDLE("pointer overflow",
        "  Base: %p  Result: %p", base, result);
}

void __ubsan_handle_pointer_overflow_abort(void *data_raw, void *base, void *result) {
    __ubsan_handle_pointer_overflow(data_raw, base, result);
}

void __ubsan_handle_builtin_unreachable(void *data_raw) {
    __ubsan_source_location *loc = data_raw;
    drt_spin_lock(&ubsan_lock);
    drt_printf("\n==UBSAN: builtin unreachable==\n  Location: ");
    ubsan_print_location(loc);
    drt_puts("\n");
    drt_stack_trace_t st;
    drt_arch_capture_stack_trace(&st, 2);
    drt_print_stack_trace(&st);
    drt_spin_unlock(&ubsan_lock);
    drt_arch_abort();
    __builtin_unreachable();
}

void __ubsan_handle_builtin_unreachable_abort(void *data_raw) {
    __ubsan_handle_builtin_unreachable(data_raw);
}
