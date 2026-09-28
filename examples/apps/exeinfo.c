/* exeinfo: dissect executable files -- ELF64, Mach-O (thin and fat), PE32+ --
 * and say what else a file is (a polyglot script, gzip, text).
 *
 *   unisacc -run exeinfo.c FILE...
 *
 * With no argument it dissects this host's own system executables (the
 * running process's image on Linux, a system binary on macOS/Windows).
 * Reads that polyglot container as readily as a plain ELF or Mach-O file.
 *
 * Every field is read through U()/UB(), which are bounds-checked: a field
 * that lies beyond the bytes examined reads as 0 and sets a flag that is
 * reported, instead of running off the buffer.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSZ (1 << 22)

static unsigned char B[BUFSZ];
static long LEN;            /* bytes in B */
static long FILESZ;         /* bytes in the file */
static int oob;

static unsigned long U(long off, int n)
{
    unsigned long v = 0;
    int i;
    if (off < 0 || off + n > LEN) { oob = 1; return 0; }
    for (i = n - 1; i >= 0; i--) v = (v << 8) | B[off + i];
    return v;
}

static unsigned long UB(long off, int n)
{
    unsigned long v = 0;
    int i;
    if (off < 0 || off + n > LEN) { oob = 1; return 0; }
    for (i = 0; i < n; i++) v = (v << 8) | B[off + i];
    return v;
}

static void name8(char *dst, long off, int n)
{
    int i;
    for (i = 0; i < n && off + i < LEN && B[off + i]; i++) dst[i] = (char)B[off + i];
    dst[i] = 0;
}

static void str_at(long off)
{
    int k;
    for (k = 0; k < 80 && off + k < LEN && B[off + k]; k++) putchar(B[off + k]);
}

static void rwx(unsigned long r, unsigned long w, unsigned long x)
{
    putchar(r ? 'r' : '-'); putchar(w ? 'w' : '-'); putchar(x ? 'x' : '-');
}

/* ---- ELF64 ---------------------------------------------------------- */

static const char *elf_machine(unsigned long m)
{
    switch (m) {
    case 3: return "x86";
    case 0x28: return "arm";
    case 0x3e: return "x86-64";
    case 0xb7: return "aarch64";
    case 0xf3: return "riscv";
    }
    return "other";
}

static const char *elf_ptype(unsigned long t)
{
    switch (t) {
    case 0: return "NULL";
    case 1: return "LOAD";
    case 2: return "DYNAMIC";
    case 3: return "INTERP";
    case 4: return "NOTE";
    case 6: return "PHDR";
    case 7: return "TLS";
    case 0x6474e550UL: return "GNU_EH_FRAME";
    case 0x6474e551UL: return "GNU_STACK";
    case 0x6474e552UL: return "GNU_RELRO";
    }
    return "other";
}

static void elf(void)
{
    unsigned long type, phoff, phentsize, phnum, i;
    int wx = 0;
    if (B[4] != 2 || B[5] != 1) {
        printf("format   ELF, class %d, data %d (only 64-bit little-endian is dissected)\n", B[4], B[5]);
        return;
    }
    type = U(16, 2);
    phoff = U(32, 8); phentsize = U(54, 2); phnum = U(56, 2);
    printf("format   ELF64 little-endian\n");
    printf("type     %s\n", type == 1 ? "relocatable" : type == 2 ? "executable" : type == 3 ? "shared/PIE" : "other");
    printf("machine  %s\n", elf_machine(U(18, 2)));
    printf("entry    0x%lx\n", U(24, 8));
    printf("segments %lu\n", phnum);
    for (i = 0; i < phnum && i < 32; i++) {
        long o = (long)(phoff + i * phentsize);
        unsigned long pt, fl;
        if (o + 56 > LEN) { printf("  (the rest lie beyond the bytes examined)\n"); break; }
        pt = U(o, 4); fl = U(o + 4, 4);
        printf("  %-12s ", elf_ptype(pt));
        rwx(fl & 4, fl & 2, fl & 1);
        printf("  offset 0x%lx  vaddr 0x%lx  filesz %lu  memsz %lu\n",
               U(o + 8, 8), U(o + 16, 8), U(o + 32, 8), U(o + 40, 8));
        if ((fl & 3) == 3) wx++;
        if (pt == 3) { printf("    interpreter "); str_at((long)U(o + 8, 8)); printf("\n"); }
    }
    if (wx) printf("warning  %d segment(s) writable and executable\n", wx);
}

/* ---- Mach-O --------------------------------------------------------- */

static const char *macho_cpu(unsigned long c)
{
    if (c == 0x0100000cUL) return "arm64";
    if (c == 0x01000007UL) return "x86_64";
    if (c == 7) return "i386";
    if (c == 12) return "arm";
    return "other";
}

static void macho(long base)
{
    unsigned long ncmds = U(base + 16, 4), filetype = U(base + 12, 4), i;
    long off = base + 32;
    printf("format   Mach-O 64-bit little-endian\n");
    printf("cpu      %s\n", macho_cpu(U(base + 4, 4)));
    printf("type     %s\n", filetype == 1 ? "object" : filetype == 2 ? "executable" : filetype == 6 ? "dylib" :
                            filetype == 8 ? "bundle" : "other");
    printf("commands %lu (%lu bytes)\n", ncmds, U(base + 20, 4));
    for (i = 0; i < ncmds && i < 64; i++) {
        unsigned long cmd = U(off, 4), size = U(off + 4, 4);
        char nm[17];
        if (size < 8 || off + (long)size > LEN) { printf("  (the rest lie beyond the bytes examined)\n"); break; }
        switch (cmd) {
        case 0x19:
            name8(nm, off + 8, 16);
            printf("  segment %-10s vm 0x%lx+0x%lx  file 0x%lx+0x%lx  ", nm, U(off + 24, 8), U(off + 32, 8),
                   U(off + 40, 8), U(off + 48, 8));
            rwx(U(off + 60, 4) & 1, U(off + 60, 4) & 2, U(off + 60, 4) & 4);
            printf("  sections %lu\n", U(off + 64, 4));
            break;
        case 0x80000028UL: printf("  entry offset 0x%lx\n", U(off + 8, 8)); break;
        case 0xc: printf("  dylib "); str_at(off + (long)U(off + 8, 4)); printf("\n"); break;
        case 0xe: printf("  dylinker "); str_at(off + (long)U(off + 8, 4)); printf("\n"); break;
        case 0x1d: printf("  code signature at 0x%lx, %lu bytes\n", U(off + 8, 4), U(off + 12, 4)); break;
        case 0x32: printf("  build version, platform %lu\n", U(off + 8, 4)); break;
        default: printf("  command 0x%lx, %lu bytes\n", cmd, size);
        }
        off += (long)size;
    }
}

static void fat(void)
{
    unsigned long n = UB(4, 4), i;
    printf("format   Mach-O universal (fat), %lu architectures\n", n);
    for (i = 0; i < n && i < 8; i++) {
        long o = 8 + 20 * (long)i;
        printf("  %-7s offset 0x%lx  size %lu\n", macho_cpu(UB(o, 4)), UB(o + 8, 4), UB(o + 12, 4));
    }
    printf("  (slices are not opened here; run exeinfo on an extracted slice)\n");
}

/* ---- PE32+ ---------------------------------------------------------- */

static void pe(long e)
{
    unsigned long machine = U(e + 4, 2), nsec = U(e + 6, 2), optsize = U(e + 20, 2), dll, i;
    long opt = e + 24, sec;
    if (U(e, 4) != 0x4550) { printf("format   DOS header without a PE signature\n"); return; }
    if (U(opt, 2) != 0x20b) { printf("format   PE, not PE32+ (only PE32+ is dissected)\n"); return; }
    dll = U(opt + 70, 2);
    printf("format   PE32+\n");
    printf("machine  %s\n", machine == 0x8664 ? "x86_64" : machine == 0xaa64 ? "arm64" : "other");
    printf("entry    rva 0x%lx, image base 0x%lx\n", U(opt + 16, 4), U(opt + 24, 8));
    printf("image    %lu bytes, subsystem %s\n", U(opt + 56, 4), U(opt + 68, 2) == 3 ? "console" : U(opt + 68, 2) == 2 ? "GUI" : "other");
    printf("timestamp %lu%s\n", U(e + 8, 4), U(e + 8, 4) == 0 ? " (zero: a reproducible build)" : "");
    printf("hardening %s%s%s\n", dll & 0x40 ? "ASLR " : "", dll & 0x100 ? "NX " : "", dll & 0x20 ? "high-entropy" : "");
    sec = opt + (long)optsize;
    printf("sections %lu\n", nsec);
    for (i = 0; i < nsec && i < 24; i++) {
        long o = sec + 40 * (long)i;
        unsigned long ch = U(o + 36, 4);
        char nm[9];
        if (o + 40 > LEN) { printf("  (the rest lie beyond the bytes examined)\n"); break; }
        name8(nm, o, 8);
        printf("  %-8s rva 0x%lx  vsize %lu  raw 0x%lx+%lu  ", nm, U(o + 12, 4), U(o + 8, 4), U(o + 20, 4), U(o + 16, 4));
        rwx(ch & 0x40000000UL, ch & 0x80000000UL, ch & 0x20000000UL);
        printf("\n");
    }
}

/* ---- the rest ------------------------------------------------------- */

static void analyze(void)
{
    long i, n = LEN < 65536 ? LEN : 65536, zero = 0, print = 0, gz = 0;
    oob = 0;
    printf("size     %ld bytes\n", FILESZ);
    for (i = 0; i < n; i++) {
        if (B[i] == 0) zero++;
        if ((B[i] >= 32 && B[i] < 127) || B[i] == '\n' || B[i] == '\t') print++;
    }
    if (n > 0) printf("content  %ld%% zero bytes, %ld%% printable (first %ld bytes)\n", zero * 100 / n, print * 100 / n, n);
    if (LEN >= 4 && B[0] == 0x7f && B[1] == 'E' && B[2] == 'L' && B[3] == 'F') elf();
    else if (LEN >= 32 && U(0, 4) == 0xfeedfacfUL) macho(0);
    else if (LEN >= 8 && UB(0, 4) == 0xcafebabeUL && UB(4, 4) < 20) fat();
    else if (LEN >= 64 && B[0] == 'M' && B[1] == 'Z') {
        if (memcmp(B, "MZqFpD", 6) == 0) {
            long nl = 0;
            while (nl < LEN && B[nl] != '\n') nl++;
            printf("polyglot DOS/PE header that is also a POSIX shell script (its first line is %ld bytes)\n", nl);
        }
        pe((long)U(0x3c, 4));
        for (i = 0; i + 3 < LEN; i++)
            if (B[i] == 0x1f && B[i + 1] == 0x8b && B[i + 2] == 8) {
                if (gz < 8) printf("gzip stream at offset %ld\n", i);
                gz++;
            }
        if (gz > 8) printf("... %ld gzip streams in all\n", gz);
    } else if (LEN >= 2 && B[0] == '#' && B[1] == '!') {
        long nl = 0;
        printf("format   script, interpreter line: ");
        while (nl < LEN && B[nl] != '\n' && nl < 80) putchar(B[nl++]);
        printf("\n");
    } else if (LEN >= 3 && B[0] == 0x1f && B[1] == 0x8b && B[2] == 8) {
        printf("format   gzip stream\n");
    } else {
        printf("format   %s\n", n > 0 && print * 100 / n > 95 ? "text" : "unknown binary");
    }
    if (oob) printf("note     some fields lie beyond the %ld bytes examined and read as 0\n", LEN);
}

static int dissect(const char *path)
{
    FILE *f = fopen(path, "rb");
    char tmp[4096];
    long n;
    if (f == 0) return 0;
    LEN = (long)fread(B, 1, BUFSZ, f);
    FILESZ = LEN;
    while ((n = (long)fread(tmp, 1, sizeof tmp, f)) > 0) FILESZ += n;
    fclose(f);
    printf("== %s\n", path);
    analyze();
    printf("\n");
    return 1;
}

int main(int argc, char **argv)
{
    /* In -run mode argv[0] names the source, not an image, so the no-argument
     * default reads real executables that exist on every host of its kind. */
    static const char *host[] = {
#if defined(_WIN32)
        "C:\\Windows\\System32\\cmd.exe", "C:\\Windows\\System32\\kernel32.dll",
#elif defined(__APPLE__)
        "/bin/ls", "/usr/lib/dyld",
#else
        "/proc/self/exe", "/bin/ls",
#endif
        0 };
    int a, found = 0;
    if (argc <= 1) {
        for (a = 0; host[a]; a++) found += dissect(host[a]);
        if (!found) { fprintf(stderr, "exeinfo: no system executable readable; pass a path\n"); return 1; }
        return 0;
    }
    for (a = 1; a < argc; a++) {
        if (!dissect(argv[a])) { fprintf(stderr, "exeinfo: cannot open %s\n", argv[a]); return 1; }
    }
    return 0;
}
