/* ---- -run forwarding stubs (0.0.19 R19-10), shared by both drivers ----
   One C stub per host function: dlsym(RTLD_DEFAULT) once, then the uffi /
   libffi bridge with the argument kinds and widths of the prototype.  The
   reference front end (src/front_parse.c fwd_stub) and the product driver
   (from E3's USLFW1 side-car) both call fwd_emit, so the stub text is the
   same bytes on both routes.  Kinds: 0 integer, 1 pointer, 4 float, 8 double.
   Appends to fwdsrc; no libc. */
int fwdrun; char fwdsrc[131072]; int nfwdsrc;
/* The widest host call the target actually delivers.  0 means fwd_emit's own
   ceiling of six.  A win target sets four, and the reason is measured, not
   guessed: the Microsoft x64 convention has four integer register slots, and
   the generated hostcall sequence (the BK_HOST_WIN_X86 template in
   back_encode.c) does not build the shadow space or the stack argument area,
   so a fifth argument is dropped in silence.  The callee then returns a
   plausible value and the caller's out-parameter stays empty -- the most
   expensive kind of wrong, because the program keeps working and the data
   never arrives (examples/win/outparam.c measures it: a 4-argument
   GetDiskFreeSpaceExA fills all three of its out-parameters, a 5-argument
   SearchPathA returns the right length and writes nothing).  So a target
   that cannot deliver the arguments refuses the forward BY NAME instead of
   emitting a call that quietly loses them.  Set by the driver from the -b
   target; the product driver leaves it 0 and does not forward to a win target
   at all yet (archive/plans/v0.0.23.md item A1). */
int fwd_maxargs;
/* 0.0.31 H3: set by the reference front end when the program takes a function's address; every
   pointer argument then goes through __unisa_ccw_of (front_parse.c cca_emit), so a function the
   host calls back arrives as its C-ABI wrapper.  0 = the old bytes (and the product's). */
int fwd_ccw;
/* 0.0.28 N1' (glob's callback): a driver that knows a parameter's exact C declaration -- a function
   pointer, which `void *` does not match -- puts it here, whole and named a<k> ("int (*a2)(char *, int)").
   Read by fwd_emit and fwd_emit_var for parameter k, cleared after each stub; all 0 = the old bytes. */
char *fwd_ptype[64];
int fwd_s(char *s) { while (*s && nfwdsrc < 131000) { fwdsrc[nfwdsrc] = *s; nfwdsrc = nfwdsrc + 1; s = s + 1; } return 0; }
int fwd_n(char *s, int n) { int k; k = 0; while (k < n && nfwdsrc < 131000) { fwdsrc[nfwdsrc] = s[k]; nfwdsrc = nfwdsrc + 1; k = k + 1; } return 0; }
int fwd_d(int v) { char d[16]; int n; n = 0; if (v == 0) { d[0] = 48; n = 1; } while (v > 0) { d[n] = 48 + v % 10; v = v / 10; n = n + 1; } while (n > 0) { n = n - 1; fwdsrc[nfwdsrc] = d[n]; nfwdsrc = nfwdsrc + 1; } return 0; }
char *fwd_ctype(int kind, int w, int uns) {
    if (kind == 1) return w == 8 && uns ? "unsigned long" : "void *";
    if (kind == 8) return "double";
    if (kind == 4) return "float";
    if (w == 8) return uns ? "unsigned long" : "long";
    if (w == 2) return uns ? "unsigned short" : "short";
    if (w == 1) return uns ? "unsigned char" : "signed char";
    return uns ? "unsigned int" : "int";
}
int fwd_ukind(int kind, int w, int uns) {     /* UFFI_* from unisacc_ffi.h */
    if (kind == 1) return 5;
    if (kind == 8) return 6;
    if (kind == 4) return 7;
    if (w == 8) return uns ? 4 : 3;
    if (w == 2) return uns ? 11 : 10;
    if (w == 1) return uns ? 9 : 8;
    return uns ? 2 : 1;
}
int fwd_pdecl(int k, int kind, int w) {
    if (k < 64 && fwd_ptype[k]) { fwd_s(fwd_ptype[k]); return 0; }
    fwd_s(fwd_ctype(kind, w, 0)); fwd_s(" a"); fwd_d(k); return 0;
}
int fwd_pclear(void) { int k; k = 0; while (k < 64) { fwd_ptype[k] = 0; k = k + 1; } return 0; }
/* 0.0.30 H3: the stubs look a name up through one helper -- the process's global symbols first,
   then, on Linux, libm.so.6, where glibc keeps fenv.h's functions (macOS has them in libSystem) */
int fwd_prologue(void) {
    fwd_s("#include <unisacc_ffi.h>\n#include <unistd.h>\n#include <stdlib.h>\n");
    if (fwd_ccw) fwd_s("void *__unisa_ccw_of(void *);\n");
    fwd_s("static void *__unisa_fwdsym(const char *n) {\n    void *f; f = uffi_dlsym((void *)UFFI_RTLD_DEFAULT, n);\n");
    fwd_s("#ifdef __linux__\n    if (f == 0) { static void *m; if (m == 0) m = uffi_dlopen(\"libm.so.6\", 2); if (m) f = uffi_dlsym(m, n); }\n#endif\n");
    fwd_s("    return f;\n}\n");
    return 0;
}
int fwd_emit(char *nm, int nl, int np, int *kind, int *w, int *uns, int isvoid, int rkind, int rw, int runs) {
    int k; char *rt; int rk;
    rt = fwd_ctype(rkind, rw, runs); rk = fwd_ukind(rkind, rw, runs);
    if (rkind == 1) { rt = "void *"; rk = 5; }
    if (nfwdsrc == 0) fwd_prologue();
    {   /* R21-4a': integer and pointer signatures of at most six arguments call
           the host directly through __hostcall (no libffi: works on Linux too);
           floating-point ones keep the uffi/libffi bridge (macOS) */
        int intonly; intonly = np <= 6 && (isvoid || rkind != 4 && rkind != 8);
        k = 0; while (k < np) { if (kind[k] == 4 || kind[k] == 8) intonly = 0; k = k + 1; }
        if (intonly) {
            fwd_s(isvoid ? "void" : rt); fwd_s(" "); fwd_n(nm, nl); fwd_s("(");
            k = 0; while (k < np) { if (k) fwd_s(", "); fwd_pdecl(k, kind[k], w[k]); k = k + 1; }
            if (np == 0) fwd_s("void");
            fwd_s(") {\n    static void *fn; long v[10];\n");
            fwd_s("    if (fn == 0) fn = __unisa_fwdsym(\""); fwd_n(nm, nl); fwd_s("\");\n");
            fwd_s("    if (fn == 0) { write(2, \"unisacc: no host function "); fwd_n(nm, nl); fwd_s("\\n\", "); fwd_d(27 + nl); fwd_s("); exit(127); }\n");
            k = 0; while (k < 10) { fwd_s("    v["); fwd_d(k); fwd_s("] = "); if (k < np) { if (fwd_ccw && kind[k] == 1) { fwd_s("(long)__unisa_ccw_of((void *)a"); fwd_d(k); fwd_s(")"); } else { fwd_s("(long)a"); fwd_d(k); } } else fwd_s("0"); fwd_s(";\n"); k = k + 1; }
            if (isvoid) fwd_s("    __hostcall(fn, v);\n");
            else { fwd_s("    return ("); fwd_s(rt); fwd_s(")__hostcall(fn, v);\n"); }
            fwd_s("}\n");
            fwd_pclear();
            return 1;
        }
    }
    fwd_s(isvoid ? "void" : rt); fwd_s(" "); fwd_n(nm, nl); fwd_s("(");
    k = 0; while (k < np) { if (k) fwd_s(", "); fwd_pdecl(k, kind[k], w[k]); k = k + 1; }
    if (np == 0) fwd_s("void");
    fwd_s(") {\n    static void *fn; int kinds[33]; void *vals[33];");
    if (!isvoid) { fwd_s(" "); fwd_s(rt); fwd_s(" r;"); }
    fwd_s("\n    if (fn == 0) fn = __unisa_fwdsym(\""); fwd_n(nm, nl); fwd_s("\");\n");
    fwd_s("    if (fn == 0) { write(2, \"unisacc -run: no host function "); fwd_n(nm, nl); fwd_s("\\n\", "); fwd_d(32 + nl); fwd_s("); exit(127); }\n");
    k = 0; while (k < np) { if (fwd_ccw && kind[k] == 1) { fwd_s("    a"); fwd_d(k); fwd_s(" = __unisa_ccw_of((void *)a"); fwd_d(k); fwd_s(");\n"); } k = k + 1; }
    k = 0; while (k < np) { fwd_s("    kinds["); fwd_d(k); fwd_s("] = "); fwd_d(fwd_ukind(kind[k], w[k], 0)); fwd_s("; vals["); fwd_d(k); fwd_s("] = &a"); fwd_d(k); fwd_s(";\n"); k = k + 1; }
    fwd_s("    uffi_call(fn, "); fwd_d(isvoid ? 0 : rk); fwd_s(", kinds, vals, "); fwd_d(np); fwd_s(", 0 - 1, "); fwd_s(isvoid ? "0" : "&r"); fwd_s(");\n");
    if (!isvoid) fwd_s("    return r;\n");
    fwd_s("}\n");
    fwd_pclear();
    return 1;
}
/* 0.0.28 N1': a variadic host function whose fixed arguments are integers or pointers (at most
   five of them), such as curl_easy_setopt.  The stub takes four more long-sized arguments through
   va_arg and hands all of them on: the callee reads only what it expects.  macOS arm64 passes
   variadic arguments on the stack, so it goes through uffi_call with the fixed count; elsewhere
   integer variadic arguments sit where fixed ones would, and __hostcall delivers them.  Floating-
   point variadic arguments are not forwarded correctly by this stub (they would need their kinds). */
int fwd_emit_var(char *nm, int nl, int np, int *kind, int *w, int isvoid, int rkind, int rw, int runs) {
    int k; char *rt; int rk;
    rt = fwd_ctype(rkind, rw, runs); rk = fwd_ukind(rkind, rw, runs);
    if (rkind == 1) { rt = "void *"; rk = 5; }
    if (np < 1 || np > 5 || rkind == 4 || rkind == 8) return 0;
    k = 0; while (k < np) { if (kind[k] == 4 || kind[k] == 8) return 0; k = k + 1; }
    if (nfwdsrc == 0) fwd_prologue();
    fwd_s("#include <stdarg.h>\n");
    fwd_s(isvoid ? "void" : rt); fwd_s(" "); fwd_n(nm, nl); fwd_s("(");
    k = 0; while (k < np) { if (k) fwd_s(", "); fwd_pdecl(k, kind[k], w[k]); k = k + 1; }
    fwd_s(", ...) {\n    static void *fn; long x[4]; va_list ap;");
    if (!isvoid) { fwd_s(" "); fwd_s(rt); fwd_s(" r;"); }
    fwd_s("\n    if (fn == 0) fn = __unisa_fwdsym(\""); fwd_n(nm, nl); fwd_s("\");\n");
    fwd_s("    if (fn == 0) { write(2, \"unisacc: no host function "); fwd_n(nm, nl); fwd_s("\\n\", "); fwd_d(27 + nl); fwd_s("); exit(127); }\n");
    fwd_s("    va_start(ap, a"); fwd_d(np - 1); fwd_s("); x[0] = va_arg(ap, long); x[1] = va_arg(ap, long); x[2] = va_arg(ap, long); x[3] = va_arg(ap, long); va_end(ap);\n");
    fwd_s("#if defined(__APPLE__) && defined(__aarch64__)\n    { int kinds[9]; void *vals[9];\n");
    k = 0; while (k < np) { if (fwd_ccw && kind[k] == 1) { fwd_s("    a"); fwd_d(k); fwd_s(" = __unisa_ccw_of((void *)a"); fwd_d(k); fwd_s(");\n"); } k = k + 1; }
    k = 0; while (k < np) { fwd_s("    kinds["); fwd_d(k); fwd_s("] = "); fwd_d(fwd_ukind(kind[k], w[k], 0)); fwd_s("; vals["); fwd_d(k); fwd_s("] = &a"); fwd_d(k); fwd_s(";\n"); k = k + 1; }
    k = 0; while (k < 4) { fwd_s("    kinds["); fwd_d(np + k); fwd_s("] = 3; vals["); fwd_d(np + k); fwd_s("] = &x["); fwd_d(k); fwd_s("];\n"); k = k + 1; }
    fwd_s("    uffi_call(fn, "); fwd_d(isvoid ? 0 : rk); fwd_s(", kinds, vals, "); fwd_d(np + 4); fwd_s(", "); fwd_d(np); fwd_s(", "); fwd_s(isvoid ? "0" : "&r"); fwd_s("); }\n");
    fwd_s("#else\n    { long v[10];\n");
    k = 0; while (k < 10) { fwd_s("    v["); fwd_d(k); fwd_s("] = "); if (k < np) { if (fwd_ccw && kind[k] == 1) { fwd_s("(long)__unisa_ccw_of((void *)a"); fwd_d(k); fwd_s(")"); } else { fwd_s("(long)a"); fwd_d(k); } } else if (k < np + 4) { fwd_s("x["); fwd_d(k - np); fwd_s("]"); } else fwd_s("0"); fwd_s(";\n"); k = k + 1; }
    if (isvoid) fwd_s("    __hostcall(fn, v); }\n");
    else { fwd_s("    r = ("); fwd_s(rt); fwd_s(")__hostcall(fn, v); }\n"); }
    fwd_s("#endif\n");
    if (!isvoid) fwd_s("    return r;\n");
    fwd_s("}\n");
    fwd_pclear();
    return 1;
}
/* 0.0.28 N1': `-l NAME` loads that host library before main, so the functions a program
   declares from it are forwarded by name like libc's.  A constructor in the stub unit tries the
   usual names (macOS /usr/lib/libNAME.dylib, libNAME.dylib; Linux libNAME.so, then .so.0 .. .so.9)
   with RTLD_NOW | RTLD_GLOBAL, and names the library when none loads. */
char *fwd_libs[16]; int nfwd_libs;
int fwd_emit_libs(void) {
    int k;
    if (nfwd_libs == 0) return 0;
    fwd_s("#include <string.h>\nstatic int __unisa_loadlib(char *n) {\n    char b[300]; int k; int d;\n");
    fwd_s("    if (strlen(n) > 200) return 0;\n#ifdef __APPLE__\n");
    fwd_s("    strcpy(b, \"/usr/lib/lib\"); strcat(b, n); strcat(b, \".dylib\"); if (uffi_dlopen(b, 2 | 8)) return 1;\n");
    fwd_s("    strcpy(b, \"lib\"); strcat(b, n); strcat(b, \".dylib\"); if (uffi_dlopen(b, 2 | 8)) return 1;\n#else\n");
    fwd_s("    strcpy(b, \"lib\"); strcat(b, n); strcat(b, \".so\"); if (uffi_dlopen(b, 2 | 0x100)) return 1;\n");
    fwd_s("    k = strlen(b); d = 0; while (d < 10) { b[k] = 46; b[k + 1] = 48 + d; b[k + 2] = 0; if (uffi_dlopen(b, 2 | 0x100)) return 1; d = d + 1; }\n#endif\n");
    fwd_s("    write(2, \"unisacc: cannot load host library -l\", 36); write(2, n, strlen(n)); write(2, \"\\n\", 1); exit(127);\n    return 0;\n}\n");
    fwd_s("__attribute__((constructor)) static void __unisa_libs(void) {\n");
    k = 0; while (k < nfwd_libs) { fwd_s("    __unisa_loadlib(\""); fwd_s(fwd_libs[k]); fwd_s("\");\n"); k = k + 1; }
    fwd_s("}\n");
    return 1;
}
