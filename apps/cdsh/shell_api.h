/*
 * shell_api.h — shell.c's public surface, so tools.c can call it without
 * `#include "shell.c"` (which would drag in a second `main`; see render_api.h).
 */
#ifndef CDSH_SHELL_API_H
#define CDSH_SHELL_API_H

#define SHELL_OUT_MAX 65536

typedef struct {
    int  ok; int err; int exited; int status; int signal; long bytes;
    char out[SHELL_OUT_MAX];
} shell_result;

shell_result shell_run(const char *command);
shell_result shell_run_in(const char *command, const char *cwd);
int          shell_run_selftest(void);

#endif
