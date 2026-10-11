/* seed/unisa_catalog.c -- C99 reference for unisa.catalog (K6-8), point-sample functions.
 *
 *   unisa-catalog OP OS ARCH
 *       prints ONE line of the derived target facts of (op, os, arch), in the order
 *         op os arch form symbol gate sysno nrreg arg0 arg1 arg2 arg3 arg4 arg5 ret argshape retconv winimp
 *       which is the subset of catalog.nine() + enc_form() + symbol() that the Python side prints.
 *       Mirrors unisa/catalog.py: enc_form, sysno, symbol, gate, argregs, nrreg, argshape, retconv,
 *       winimp; the SYSCALLS numbers and WinAPI names for the sampled ops; MNEMONIC for the sampled
 *       machine ops.  Ops outside the sampled set and an unknown op are refused by name (python:
 *       KeyError on SYSCALLS/MNEMONIC), nothing printed.
 *
 * Point-sample equivalence over the listed (op, os, arch) points only; not a product closure member;
 * keeps unisa/catalog.py; the full SYSCALLS/OPS/nine table is not exhaustively compared (K6-8b).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OSX_CLASS_BIT 0x02000000u

static void die(const char *why, const char *what) {
    fprintf(stderr, "unisa-catalog: %s %s\n", why, what ? what : "");
    exit(1);
}

typedef struct { const char *op; long lx, la, ox; const char *win; } Sys;   /* -1 = None */
static const Sys SYS[] = {
    {"exit", 60, 93, 1, "ExitProcess"}, {"read", 0, 63, 3, "ReadFile"},
    {"write", 1, 64, 4, "WriteFile"}, {"open", 2, 56, 5, "CreateFileW"},
    {"close", 3, 57, 6, "CloseHandle"}, {"getdents64", 217, 61, -1, NULL},
    {"mmap", 9, 222, 197, "VirtualAlloc"}, {"munmap", 11, 215, 73, "VirtualFree"},
    {"mprotect", 10, 226, 74, "VirtualProtect"}, {"getpid", 39, 172, 20, "GetCurrentProcessId"},
    {"clock_gettime", 228, 113, -1, "QueryPerformanceCounter"}, {"nanosleep", 35, 101, -1, "Sleep"},
    {"futex", 202, 98, 515, "WaitOnAddress"}, {"socket", 41, 198, 97, "WSASocketW"},
    {"connect", 42, 203, 98, "connect"}, {"bind", 49, 200, 104, "bind"},
    {"listen", 50, 201, 106, "listen"}, {"accept", 43, 202, 30, "accept"},
    {"clone", 56, 220, 360, "CreateThread"}, {"execve", 59, 221, 59, "CreateProcessW"},
    {"lseek", 8, 62, 199, "SetFilePointer"}, {"unlink", 87, 35, 10, "DeleteFileA"},
    {"rename", 82, 276, 128, "MoveFileExA"}, {"ioctl", 16, 29, 54, NULL},
    {"fstat", 5, 80, 339, NULL}, {"stat", 4, 79, 338, NULL}, {"lstat", 6, 79, 340, NULL},
    {"fcntl", 72, 25, 92, NULL}, {"mkdir", 83, 34, 136, NULL}, {"chmod", 90, 53, 15, NULL},
    {"poll", 7, -1, 230, NULL}, {"ppoll", 271, 73, -1, NULL}, {"getdirentries64", -1, -1, 344, NULL},
    {"ftruncate", 77, 46, 201, NULL}, {"gettimeofday", 96, 169, 116, NULL},
    {"syscall", 0, 0, 0, NULL},
};
#define NSYS ((int)(sizeof SYS / sizeof SYS[0]))

/* sampled machine ops: (op, x86 mnemonic, arm mnemonic) */
static const struct { const char *op, *x86, *arm; } MOP[] = {
    {"add64", "add", "add"}, {"syscall_gate", "syscall", "svc"}, {"fsqrt64", "sqrtsd", "fsqrt"},
};
#define NMOP ((int)(sizeof MOP / sizeof MOP[0]))

static int is_mop(const char *op) { int i; for (i = 0; i < NMOP; i++) if (!strcmp(MOP[i].op, op)) return 1; return 0; }
static const Sys *find_sys(const char *op) { int i; for (i = 0; i < NSYS; i++) if (!strcmp(SYS[i].op, op)) return &SYS[i]; return NULL; }

static const char *nr_reg(const char *os, const char *arch) {
    if (!strcmp(os, "lnx") && !strcmp(arch, "x86_64")) return "rax";
    if (!strcmp(os, "osx") && !strcmp(arch, "x86_64")) return "rax";
    if (!strcmp(os, "win") && !strcmp(arch, "x86_64")) return "rax";
    if (!strcmp(os, "lnx") && !strcmp(arch, "arm64")) return "x8";
    if (!strcmp(os, "osx") && !strcmp(arch, "arm64")) return "x16";
    if (!strcmp(os, "win") && !strcmp(arch, "arm64")) return "x16";
    return "none";
}

static const char *winimp_of(const char *op) {
    static const struct { const char *op, *imp; } W[] = {
        {"exit", "ExitProcess"}, {"write", "WriteFile"}, {"read", "ReadFile"}, {"mmap", "VirtualAlloc"},
        {"mprotect", "VirtualProtect"}, {"munmap", "VirtualFree"}, {"close", "CloseHandle"},
        {"open", "CreateFileA"}, {"lseek", "SetFilePointer"}, {"unlink", "DeleteFileA"}, {"rename", "MoveFileExA"},
    };
    size_t i;
    for (i = 0; i < sizeof W / sizeof W[0]; i++) if (!strcmp(W[i].op, op)) return W[i].imp;
    return "none";
}
static const char *retconv_of(const char *op) {
    static const struct { const char *op, *c; } R[] = {
        {"write", "wcount"}, {"read", "wcount"}, {"mprotect", "bool_inv"}, {"munmap", "bool_inv"},
        {"unlink", "bool_neg"}, {"rename", "bool_neg"}, {"lseek", "dword_sx"},
    };
    size_t i;
    for (i = 0; i < sizeof R / sizeof R[0]; i++) if (!strcmp(R[i].op, op)) return R[i].c;
    return "none";
}
static const char *argshape_of(const char *op, const char *os, const char *arch) {
    if (!strcmp(os, "lnx") && !strcmp(arch, "arm64")) {
        if (!strcmp(op, "open") || !strcmp(op, "stat") || !strcmp(op, "lstat") || !strcmp(op, "mkdir") || !strcmp(op, "chmod")) return "atfd_1";
        if (!strcmp(op, "unlink")) return "atfd_1_zero";
        if (!strcmp(op, "rename")) return "atfd_2_zero5";
    }
    if (!strcmp(os, "lnx") && !strcmp(op, "ppoll")) return "zero4";
    return "plain";
}

int main(int argc, char **argv) {
    const char *op, *os, *arch;
    int x86, lnx, osx, win, mop;
    char sysbuf[32];
    const char *form, *symbol, *gate, *sysno;
    const char *a[7];
    const Sys *s;
    if (argc == 2 && !strcmp(argv[1], "consts")) {
        /* OS, ARCH and TARGETS = "%s/%s" for os in OS for arch in ARCH (catalog.py) */
        static const char *OSN[] = {"lnx", "osx", "win"};
        static const char *ARN[] = {"x86_64", "arm64"};
        int i, j;
        printf("OS");  for (i = 0; i < 3; i++) printf(" %s", OSN[i]); printf("\n");
        printf("ARCH"); for (j = 0; j < 2; j++) printf(" %s", ARN[j]); printf("\n");
        printf("TARGETS");
        for (i = 0; i < 3; i++) for (j = 0; j < 2; j++) printf(" %s/%s", OSN[i], ARN[j]);
        printf("\n");
        return 0;
    }
    if (argc != 4) { fprintf(stderr, "usage: unisa-catalog OP OS ARCH | consts\n"); return 2; }
    op = argv[1]; os = argv[2]; arch = argv[3];
    if (strcmp(os, "lnx") && strcmp(os, "osx") && strcmp(os, "win")) die("unknown OS", os);
    if (strcmp(arch, "x86_64") && strcmp(arch, "arm64")) die("unknown ARCH", arch);
    x86 = !strcmp(arch, "x86_64"); lnx = !strcmp(os, "lnx"); osx = !strcmp(os, "osx"); win = !strcmp(os, "win");
    mop = is_mop(op);
    s = find_sys(op);
    if (!mop && !s) die("unknown op (python: KeyError)", op);

    /* enc_form */
    if (win && !mop) form = "winapi";
    else if (!x86 && !mop) form = "svc";
    else if (!mop && x86 && (lnx || osx)) form = "syscall";
    else form = x86 ? "x86" : "arm";

    /* symbol */
    if (mop) { int i; symbol = NULL; for (i = 0; i < NMOP; i++) if (!strcmp(MOP[i].op, op)) symbol = x86 ? MOP[i].x86 : MOP[i].arm; }
    else symbol = op;

    /* gate */
    if (mop && strcmp(op, "syscall_gate")) gate = "none";
    else if (win) gate = "winapi";
    else if (!x86) gate = osx ? "svc80" : "svc0";
    else gate = "syscall";

    /* sysno */
    if (mop || win) sysno = "none";
    else {
        long v;
        if (lnx) v = x86 ? s->lx : s->la;
        else v = s->ox;
        if (v < 0) sysno = "none";
        else if (lnx) { snprintf(sysbuf, sizeof sysbuf, "%ld", v); sysno = sysbuf; }
        else { snprintf(sysbuf, sizeof sysbuf, "0x%x", (unsigned)(OSX_CLASS_BIT | (unsigned long)v)); sysno = sysbuf; }
    }

    /* argregs */
    if (mop) { int k; for (k = 0; k < 7; k++) a[k] = "none"; }
    else if (win) {
        if (x86) { a[0]="rcx"; a[1]="rdx"; a[2]="r8"; a[3]="r9"; a[4]="none"; a[5]="none"; a[6]="rax"; }
        else { a[0]="x0"; a[1]="x1"; a[2]="x2"; a[3]="x3"; a[4]="none"; a[5]="none"; a[6]="x0"; }
    } else if (x86) { a[0]="rdi"; a[1]="rsi"; a[2]="rdx"; a[3]="r10"; a[4]="r8"; a[5]="r9"; a[6]="rax"; }
    else { a[0]="x0"; a[1]="x1"; a[2]="x2"; a[3]="x3"; a[4]="x4"; a[5]="x5"; a[6]="x0"; }

    /* nrreg: the register carries the number only for an op that has one */
    {
        const char *nr = strcmp(sysno, "none") ? nr_reg(os, arch) : "none";
        printf("%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n",
               op, os, arch, form, symbol, gate, sysno, nr,
               a[0], a[1], a[2], a[3], a[4], a[5], a[6],
               argshape_of(op, os, arch), win ? retconv_of(op) : "none", win ? winimp_of(op) : "none");
    }
    return 0;
}
