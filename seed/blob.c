/* seed/blob.c -- exec/c/asm/blob.py in C99 + POSIX (0.0.32 B5).
 *
 * Offline assembly/linking only: assemble exec/c/asm/<unit>_<arch>.S, link them
 * static with no imports, and cut the position-independent kernel blob out of the
 * Mach-O __TEXT segment.  The output is byte-identical to blob.py's:
 *     "UNIKERN1" u64 arch(1 arm64, 2 x86_64) u64 entry u64 slot u64 end, text[0:end]
 *
 * Usage: blob ROOT ARCH OUTPUT
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

static const char *const UNITS[] = {"transition", "arith", "buffer", "memory", "intern", "bytes",
                                    "format", "stack", "action", "run", "bridge"};
static const char *const IMPORTS[] = {"calloc", "realloc", "free", "memcpy", "memcmp", "memset",
                                      "strlen", "core_host_fetch", "core_host_panic"};
static char dir[256];

static void cleanup(void) {
    char cmd[400];
    if (dir[0]) { snprintf(cmd, sizeof cmd, "rm -rf '%s'", dir); if (system(cmd)) { } }
}
static void die(const char *why) { fprintf(stderr, "blob: %s\n", why); cleanup(); exit(1); }
static void run(const char *cmd) { if (system(cmd) != 0) { fprintf(stderr, "blob: %s\n", cmd); die("command failed"); } }
static uint32_t u32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
static uint64_t u64(const unsigned char *p) { return u32(p) | (uint64_t)u32(p + 4) << 32; }
static void put64(unsigned char *p, uint64_t v) { int i; for (i = 0; i < 8; i++) p[i] = (unsigned char)(v >> 8 * i); }

int main(int argc, char **argv) {
    const char *root, *arch, *out; char cmd[8192], path[512]; size_t i, k;
    unsigned char *raw; long n, got = 0; struct stat st; int fd;
    uint32_t ncmd, at, symoff = 0, count = 0, stroff = 0, strsize = 0; int havesym = 0, havetext = 0;
    uint64_t textva = 0, end = 0, entry = 0, slot = 0; int fe = 0, fs = 0;
    if (argc != 4 || (strcmp(argv[2], "arm64") && strcmp(argv[2], "x86_64"))) die("usage: blob ROOT arm64|x86_64 OUTPUT");
    root = argv[1]; arch = argv[2]; out = argv[3];
    strcpy(dir, "/tmp/unisa-kernel-XXXXXX");
    if (!mkdtemp(dir)) { dir[0] = 0; die("mkdtemp failed"); }
    for (i = 0; i < sizeof UNITS / sizeof *UNITS; i++) {
        int c = snprintf(cmd, sizeof cmd, "cc -arch %s", arch);
        if (strcmp(UNITS[i], "bridge"))
            for (k = 0; k < sizeof IMPORTS / sizeof *IMPORTS; k++)
                c += snprintf(cmd + c, sizeof cmd - c, " -D%s=bridge_%s", IMPORTS[k], IMPORTS[k]);
        snprintf(cmd + c, sizeof cmd - c, " -c '%s/exec/c/asm/%s_%s.S' -o '%s/%s.o'", root, UNITS[i], arch, dir, UNITS[i]);
        run(cmd);
    }
    {   int c = snprintf(cmd, sizeof cmd, "cc -arch %s -nostdlib -Wl,-static -Wl,-e,_kernel_entry -Wl,-no_uuid -Wl,-no_fixup_chains", arch);
        for (i = 0; i < sizeof UNITS / sizeof *UNITS; i++) c += snprintf(cmd + c, sizeof cmd - c, " '%s/%s.o'", dir, UNITS[i]);
        snprintf(cmd + c, sizeof cmd - c, " -o '%s/kernel'", dir);
        run(cmd);
    }
    snprintf(cmd, sizeof cmd, "nm -u '%s/kernel' > '%s/undef'", dir, dir); run(cmd);
    snprintf(path, sizeof path, "%s/undef", dir);
    if (stat(path, &st) || st.st_size != 0) die("unresolved kernel imports");
    snprintf(path, sizeof path, "%s/kernel", dir);
    if ((fd = open(path, O_RDONLY)) < 0 || fstat(fd, &st)) die("cannot read kernel");
    raw = malloc((size_t)st.st_size + 1); if (!raw) die("out of memory");
    while (got < st.st_size && (n = read(fd, raw + got, (size_t)(st.st_size - got))) > 0) got += n;
    close(fd);
    if (got != st.st_size || got < 32 || u32(raw) != 0xfeedfacfu) die("not Mach-O 64");
    ncmd = u32(raw + 16); at = 32;
    for (i = 0; i < ncmd; i++) {
        uint32_t c, size;
        if (at + 8 > (uint32_t)got) die("bad load command");
        c = u32(raw + at); size = u32(raw + at + 4);
        if (size < 8 || at + size > (uint32_t)got) die("bad load command");
        if (c == 0x19) {
            const char *name = (const char *)raw + at + 8;
            uint64_t va = u64(raw + at + 24), off = u64(raw + at + 40), fsz = u64(raw + at + 48);
            if (!strncmp(name, "__TEXT", 16)) {
                uint32_t ns, s;
                if (havetext || off != 0 || va % 16384) die("bad text base");
                havetext = 1; textva = va; ns = u32(raw + at + 64);
                for (s = 0; s < ns; s++) {
                    uint32_t pos = at + 72 + 80 * s;
                    uint64_t addr = u64(raw + pos + 32), length = u64(raw + pos + 40);
                    uint32_t sec_off = u32(raw + pos + 48);
                    if (addr - va != sec_off || sec_off + length > fsz) die("nonlinear text");
                    if (sec_off + length > end) end = sec_off + length;
                }
            } else if (strncmp(name, "__PAGEZERO", 16) && strncmp(name, "__LINKEDIT", 16)) die("unexpected mutable segment");
        } else if (c == 2) {
            symoff = u32(raw + at + 8); count = u32(raw + at + 12); stroff = u32(raw + at + 16); strsize = u32(raw + at + 20); havesym = 1;
        } else if (c == 0x22 || c == 0x80000022u) {
            /* no rebasing, binding or lazy imports may be needed at runtime */
            if (u32(raw + at + 12) || u32(raw + at + 20) || u32(raw + at + 28) || u32(raw + at + 36)) die("runtime relocation required");
        } else if (c == 0x80000034u) die("chained fixups unsupported");
        at += size;
    }
    if (!havetext || !havesym || end == 0) die("missing kernel text/symbols");
    for (i = 0; i < count; i++) {
        const unsigned char *e = raw + symoff + i * 16; uint32_t ix = u32(e); unsigned char typ = e[4];
        const char *name;
        if (ix >= strsize) die("bad string index");
        name = (const char *)raw + stroff + ix;
        if (!strcmp(name, "_kernel_entry") || !strcmp(name, "_kernel_service")) {
            int isentry = !strcmp(name, "_kernel_entry");
            if ((isentry ? fe : fs) || (typ & 0x0e) != 0x0e) die("bad exported binding");
            if (isentry) { fe = 1; entry = u64(e + 8) - textva; } else { fs = 1; slot = u64(e + 8) - textva; }
        }
    }
    if (!fe || !fs) die("missing kernel_entry or kernel_service");
    if (!(entry < end) || !(slot + 8 <= end) || slot % 8) die("bad entry or import slot");
    for (i = 0; i < 8; i++) if (raw[slot + i]) die("bad entry or import slot");
    {   unsigned char hdr[40]; FILE *o;
        memcpy(hdr, "UNIKERN1", 8); put64(hdr + 8, strcmp(arch, "arm64") ? 2 : 1);
        put64(hdr + 16, entry); put64(hdr + 24, slot); put64(hdr + 32, end);
        if (!(o = fopen(out, "wb")) || fwrite(hdr, 1, 40, o) != 40 || fwrite(raw, 1, (size_t)end, o) != end || fclose(o)) die("cannot write output");
        printf("kernel blob %s: %lu B, entry/slot/text (%lu, %lu, %lu)\n", arch, (unsigned long)(40 + end),
               (unsigned long)entry, (unsigned long)slot, (unsigned long)end);
    }
    cleanup();
    return 0;
}
