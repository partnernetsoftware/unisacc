#ifndef LIBUNISACC_H
#define LIBUNISACC_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* R10 development API. Compilation returns tape; host-native relocation
   and main execution are available. Callable exports, symbol injection and
   full multi-platform signatures remain under development. */
typedef struct us_context us_context;
us_context *us_new(const char *package_path);
void us_free(us_context *ctx);
int us_add_source(us_context *ctx, const char *name, const char *source);
int us_add_file(us_context *ctx, const char *path);
int us_add_tape(us_context *ctx, const void *bytes, size_t length);
int us_define(us_context *ctx, const char *definition);
int us_include_path(us_context *ctx, const char *path);
int us_compile(us_context *ctx, const char *target, int optimisation);
int us_relocate(us_context *ctx);
int us_run_main(us_context *ctx, int argc, const char *const *argv, int *status);
const char *us_error(const us_context *ctx);
const void *us_tape(const us_context *ctx, size_t *length);
#ifdef __cplusplus
}
#endif
#endif
