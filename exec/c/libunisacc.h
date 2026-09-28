#ifndef LIBUNISACC_H
#define LIBUNISACC_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* R10 development API: model compilation, host-native relocation and callable
   fixed integer/pointer/void exports. Symbol injection, wider ABIs and full
   platform qualification remain under development. One caller per context;
   different contexts may execute concurrently. */
typedef struct us_context us_context;
/* ABI facts, not parser-local IDs. kind: 0 void, 1 integer, 2 data pointer,
   3 float, 4 function pointer, 5 aggregate, 6 unknown. base/shape are opaque. */
typedef struct us_type_descriptor {
    uint64_t depth, base, shape, kind, width, uns;
} us_type_descriptor;
typedef struct us_signature {
    unsigned kind; /* 0 function, 1 data */
    us_type_descriptor result;
    const us_type_descriptor *args;
    size_t count;
    unsigned variadic;
    uint64_t extent; /* data object bytes, not pointee bytes */
    unsigned writable;
} us_signature;
us_context *us_new(const char *package_path);
void us_free(us_context *ctx);
int us_add_source(us_context *ctx, const char *name, const char *source);
int us_add_file(us_context *ctx, const char *path);
int us_add_tape(us_context *ctx, const void *bytes, size_t length);
int us_define(us_context *ctx, const char *definition);
int us_include_path(us_context *ctx, const char *path);
/* Borrow address; caller owns its lifetime. Successful registration invalidates
   compiled code and previously returned exports; recompile before use. */
int us_add_symbol(us_context *ctx, const char *name, void *address, const us_signature *signature);
int us_compile(us_context *ctx, const char *target, int optimisation);
int us_relocate(us_context *ctx);
int us_run_main(us_context *ctx, int argc, const char *const *argv, int *status);
/* Pointers survive calls, but not recompile, relocate or free. */
void *us_sym(us_context *ctx, const char *name);
/* Most recent native export invocation: 0 success, 1 error/explicit exit.
   Native pointer calls return a zero/NULL/void sentinel on failure. */
int us_call_status(const us_context *ctx, int *exit_status);
const char *us_error(const us_context *ctx);
const void *us_tape(const us_context *ctx, size_t *length);
#ifdef __cplusplus
}
#endif
#endif
