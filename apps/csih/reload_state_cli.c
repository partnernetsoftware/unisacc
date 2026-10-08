/*
 * reload_state_cli.c — feed a JSON state file to the library validator.
 *
 * WHY: reload_state.c owns the question "is this v1 state valid?". This
 * tiny CLI owns only I/O: read a file, hand the bytes to the library,
 * print PASS/FAIL plus the reason. No JSON parsing lives here.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int reload_state_validate(const char *text, size_t len, char *why, size_t cap);

#define RS_CLI_MAX 131072

int main(int argc, char **argv) {
    if (argc != 3 || strcmp(argv[1], "check") != 0) {
        fprintf(stderr, "usage: %s check FILE\n", argv[0]);
        return 2;
    }

    const char *path = argv[2];
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "cannot open: %s\n", path);
        return 2;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "cannot seek: %s\n", path);
        fclose(f);
        return 2;
    }
    long n = ftell(f);
    if (n < 0) {
        fprintf(stderr, "cannot size: %s\n", path);
        fclose(f);
        return 2;
    }
    if (n > RS_CLI_MAX) {
        fprintf(stderr, "too large (>%d)\n", RS_CLI_MAX);
        fclose(f);
        return 2;
    }
    rewind(f);

    size_t len = (size_t)n;
    char *buf = malloc(len + 1);
    if (!buf) {
        fprintf(stderr, "out of memory\n");
        fclose(f);
        return 2;
    }
    size_t got = fread(buf, 1, len, f);
    if (got != len || ferror(f)) {
        fprintf(stderr, "read error: %s\n", path);
        free(buf);
        fclose(f);
        return 2;
    }
    fclose(f);
    buf[len] = '\0';

    char why[256];
    int ok = reload_state_validate(buf, len, why, sizeof why);
    printf("%s: %s\n", ok ? "PASS" : "FAIL", why);

    free(buf);
    return ok ? 0 : 1;
}
