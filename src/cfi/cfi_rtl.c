/* Misan Control Flow Integrity / Shadow Call Stack runtime
 * Type hash verification for indirect calls.
 */

#include "sanitizer/common.h"
#include "sanitizer/cfi_interface.h"

#define CFI_HASH_TABLE_BITS 12
#define CFI_HASH_TABLE_SIZE (1 << CFI_HASH_TABLE_BITS)
#define CFI_HASH_MASK (CFI_HASH_TABLE_SIZE - 1)

typedef struct cfi_entry {
    uint64_t type_hash;
    void *vtable;
    struct cfi_entry *next;
} cfi_entry_t;

static cfi_entry_t *cfi_hash_table[CFI_HASH_TABLE_SIZE];
static drt_spinlock_t cfi_lock = MISAN_SPINLOCK_INIT;


static inline uint32_t cfi_hash_uint64(uint64_t val) {
    uint32_t h = 2166136261U;
    for (int i = 0; i < 8; i++) {
        h ^= (val >> (i * 8)) & 0xFF;
        h *= 16777619U;
    }
    return h & CFI_HASH_MASK;
}

static cfi_entry_t *cfi_lookup(uint64_t type_hash, void *vtable) {
    uint32_t idx = cfi_hash_uint64(type_hash ^ (uintptr_t)vtable);
    cfi_entry_t *e = cfi_hash_table[idx];
    while (e) {
        if (e->type_hash == type_hash && e->vtable == vtable)
            return e;
        e = e->next;
    }
    return 0;
}

static void cfi_insert(uint64_t type_hash, void *vtable) {
    if (cfi_lookup(type_hash, vtable)) return;

    uint32_t idx = cfi_hash_uint64(type_hash ^ (uintptr_t)vtable);
    cfi_entry_t *e = drt_internal_alloc(sizeof(cfi_entry_t));
    e->type_hash = type_hash;
    e->vtable = vtable;
    e->next = cfi_hash_table[idx];
    cfi_hash_table[idx] = e;
}

void __cfi_slowpath(uint64_t call_site_type, void *addr) {
    void *vtable = *(void **)addr;
    if (!cfi_lookup(call_site_type, vtable)) {
        drt_printf("\n==CFI ERROR: type mismatch==\n");
        drt_printf("  Call site type hash: 0x%016llx\n", (unsigned long long)call_site_type);
        drt_printf("  Target address:      %p\n", addr);
        drt_printf("  Vtable:              %p\n", vtable);
        drt_stack_trace_t st;
        drt_arch_capture_stack_trace(&st, 2);
        drt_print_stack_trace(&st);
        __cfi_fail();
    }
}

void __cfi_slowpath_diag(uint64_t call_site_type, void *addr, void *diag_data) {
    (void)diag_data;
    __cfi_slowpath(call_site_type, addr);
}

void __cfi_fail(void) {
    drt_die("CFI check failed");
}

void __sanitizer_cfi_bad_type(void *diag_data, void *vtable, void *valid_vtable,
                               void *caller_pc, void *tag) {
    (void)diag_data;
    (void)valid_vtable;
    (void)tag;
    drt_printf("\n==CFI BAD TYPE==\n");
    drt_printf("  Vtable:    %p\n", vtable);
    drt_printf("  Caller:    %p\n", caller_pc);
    drt_stack_trace_t st;
    drt_arch_capture_stack_trace(&st, 2);
    drt_print_stack_trace(&st);
    drt_arch_abort();
    __builtin_unreachable();
}

void __sanitizer_cfi_init(void) {
    /* Register known valid vtables here if pre-population is needed */
}
