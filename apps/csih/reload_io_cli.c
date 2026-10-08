/* reload_io_cli.c - reload save CLI entry */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int reload_io_save(const char *path, const char *text, size_t len,
                   char *why, size_t cap);

int main(int argc, char **argv) {
    char why[512];
    why[0] = '\0';
    long sz;
    size_t len;
    char *buf;
    FILE *fp;
    int rc;

    if (argc != 4 || strcmp(argv[1], "save") != 0) {
        fprintf(stderr, "usage: %s save DEST JSON_FILE\n", argv[0]);
        return 2;
    }

    const char *dest = argv[2];
    const char *file = argv[3];

    fp = fopen(file, "rb");
    if (!fp) {
        fprintf(stderr, "read error: cannot open %s\n", file);
        return 2;
    }
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return 2; }
    sz = ftell(fp);
    if (sz < 0 || sz > 131072) { fclose(fp); return 2; }
    if (fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); return 2; }

    len = (size_t)sz;
    buf = malloc(len + 1);
    if (!buf) { fclose(fp); return 2; }

    if (len > 0 && fread(buf, 1, len, fp) != len) {
        free(buf); fclose(fp);
        fprintf(stderr, "read error: short read on %s\n", file);
        return 2;
    }
    buf[len] = '\0';
    fclose(fp);

    rc = reload_io_save(dest, buf, len, why, sizeof(why));
    free(buf);

    printf("rc=%d why=%s\n", rc, why);
    if (rc == 0) return 0;
    if (rc == -2) return 3;
    return 1;
}
