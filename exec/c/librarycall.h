#ifndef UNISACC_LIBRARYCALL_H
#define UNISACC_LIBRARYCALL_H
/* Fixed integer/pointer host -> script ABI adapter, macOS arm64/x86_64.
   Raw labels are not native ABI pointers. Not FP/aggregate/variadic support.
   Caller supplies a private writable downward-growing stack; top is 16-aligned.
   Stack depth/guards and interruption/exit recovery belong to the host context. */
#include <stdint.h>
#include <stddef.h>
enum us_library_value_kind { US_LIBRARY_INTEGER=1, US_LIBRARY_POINTER=2 };
typedef struct us_library_signature {
    unsigned argument_count;
    unsigned result_kind;
    unsigned argument_kind[6];
    unsigned variadic;
} us_library_signature;
uint64_t us_library_bridge_raw(const void *entry,const uint64_t args[6],void *softstack_top);
static inline int us_library_signature_supported(const us_library_signature *s) {
    unsigned i;
    if (!s || s->variadic || s->argument_count>6 ||
        (s->result_kind!=US_LIBRARY_INTEGER && s->result_kind!=US_LIBRARY_POINTER)) return 0;
    for (i=0;i<s->argument_count;i++)
        if (s->argument_kind[i]!=US_LIBRARY_INTEGER && s->argument_kind[i]!=US_LIBRARY_POINTER) return 0;
    return 1;
}
static inline int us_library_call(const void *entry,const uint64_t args[6],
        void *softstack_top,const us_library_signature *signature,uint64_t *result) {
    if (!entry || !args || !softstack_top || !result ||
        ((uintptr_t)softstack_top & 15) || !us_library_signature_supported(signature)) return 0;
    *result=us_library_bridge_raw(entry,args,softstack_top);return 1;
}
#endif
