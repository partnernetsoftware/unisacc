#ifndef LIBUNISACC_H
#define LIBUNISACC_H
#include <stddef.h>
#include <stdint.h>
/* Define LIBUNISACC_STATIC when consuming a Windows static archive. */
#if defined(_WIN32) && !defined(LIBUNISACC_STATIC)
#ifdef LIBUNISACC_BUILD
#define US_API __declspec(dllexport)
#else
#define US_API __declspec(dllimport)
#endif
#elif defined(__GNUC__)
#define US_API __attribute__((visibility("default")))
#else
#define US_API
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* R10 development API: model compilation, host-native relocation, declared
   native bindings and scalar/plain-aggregate exports/imports, including typed
   variadic native imports with model-declared concrete callsites. Recursive
   callback signatures, unions/bitfields/wide floating layouts and full platform
   qualification remain under development. One caller per context;
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
US_API us_context *us_new(const char *package_path);
US_API void us_free(us_context *ctx);
US_API int us_add_source(us_context *ctx, const char *name, const char *source);
US_API int us_add_file(us_context *ctx, const char *path);
US_API int us_add_tape(us_context *ctx, const void *bytes, size_t length);
US_API int us_define(us_context *ctx, const char *definition);
US_API int us_include_path(us_context *ctx, const char *path);
/* Borrow address; caller owns its lifetime. Successful registration invalidates
   compiled code and previously returned exports; recompile before use. */
US_API int us_add_symbol(us_context *ctx, const char *name, void *address, const us_signature *signature);
/* Trusted ABI/object declarations; dlsym does not discover or validate types.
   Native lookup freezes candidates at compile: injection, process, then owned
   libraries in load order. The model selects and checks the winner. POSIX uses
   dlsym(RTLD_DEFAULT); Windows uses the first export in Toolhelp module order,
   excluding this context's owned modules. Process addresses are borrowed: the
   caller must keep their modules loaded while compiled exports remain live.
   Success invalidates code/exports; failure preserves the prior generation.
   Handles remain owned until us_free, after compiled code is destroyed. */
US_API int us_declare_import(us_context *ctx, const char *name, const us_signature *signature);
/* Typed fixed or variadic native declarations use the exact model USLSIG2 single-record
   protocol. The record name must equal name; bytes are deep-owned on success.
   Old us_signature/API layout is unchanged. Mutation follows the same generation
   invalidation rule. A variadic prototype describes its fixed prefix; the model records and promotes
   each concrete tail before the host builds its call plan (at least one fixed
   parameter, at most 1024 total parameters). This does not enable legacy
   us_signature variadics or native pointers to variadic script exports. Callback/union and
   wide floating layouts remain separate. */
US_API int us_add_symbol_typed(us_context *ctx, const char *name, void *address,
                              const void *signature, size_t length);
US_API int us_declare_import_typed(us_context *ctx, const char *name,
                                  const void *signature, size_t length);
US_API int us_load_library(us_context *ctx, const char *path);
US_API int us_compile(us_context *ctx, const char *target, int optimisation);
US_API int us_relocate(us_context *ctx);
US_API int us_run_main(us_context *ctx, int argc, const char *const *argv, int *status);
/* Pointers survive calls, but not recompile, relocate or free. */
US_API void *us_sym(us_context *ctx, const char *name);
/* Most recent native export invocation: 0 success, 1 error/explicit exit.
   Native pointer calls return a zero/NULL/void sentinel on failure. */
US_API int us_call_status(const us_context *ctx, int *exit_status);
US_API const char *us_error(const us_context *ctx);
US_API const void *us_tape(const us_context *ctx, size_t *length);
#ifdef __cplusplus
}
#endif
#endif
