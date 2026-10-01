/* ---- -run forwarding stubs (0.0.19 R19-10), shared by both drivers ----
   One C stub per host function: dlsym(RTLD_DEFAULT) once, then the uffi /
   libffi bridge with the argument kinds and widths of the prototype.  The
   reference front end (src/front_parse.c fwd_stub) and the product driver
   (from E3's USLFW1 side-car) both call fwd_emit, so the stub text is the
   same bytes on both routes.  Kinds: 0 integer, 1 pointer, 4 float, 8 double.
   Appends to fwdsrc; no libc. */
int fwdrun; char fwdsrc[131072]; int nfwdsrc;
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
int fwd_emit(char *nm, int nl, int np, int *kind, int *w, int *uns, int isvoid, int rkind, int rw, int runs) {
    int k; char *rt; int rk;
    rt = fwd_ctype(rkind, rw, runs); rk = fwd_ukind(rkind, rw, runs);
    if (rkind == 1) { rt = "void *"; rk = 5; }
    if (nfwdsrc == 0) fwd_s("#include <unisacc_ffi.h>\n#include <unistd.h>\n");
    fwd_s(isvoid ? "void" : rt); fwd_s(" "); fwd_n(nm, nl); fwd_s("(");
    k = 0; while (k < np) { if (k) fwd_s(", "); fwd_s(fwd_ctype(kind[k], w[k], 0)); fwd_s(" a"); fwd_d(k); k = k + 1; }
    if (np == 0) fwd_s("void");
    fwd_s(") {\n    static void *fn; int kinds[33]; void *vals[33];");
    if (!isvoid) { fwd_s(" "); fwd_s(rt); fwd_s(" r;"); }
    fwd_s("\n    if (fn == 0) fn = uffi_dlsym((void *)(0 - 2), \""); fwd_n(nm, nl); fwd_s("\");\n");
    fwd_s("    if (fn == 0) { write(2, \"unisacc -run: no host function "); fwd_n(nm, nl); fwd_s("\\n\", "); fwd_d(32 + nl); fwd_s("); _exit(127); }\n");
    k = 0; while (k < np) { fwd_s("    kinds["); fwd_d(k); fwd_s("] = "); fwd_d(fwd_ukind(kind[k], w[k], 0)); fwd_s("; vals["); fwd_d(k); fwd_s("] = &a"); fwd_d(k); fwd_s(";\n"); k = k + 1; }
    fwd_s("    uffi_call(fn, "); fwd_d(isvoid ? 0 : rk); fwd_s(", kinds, vals, "); fwd_d(np); fwd_s(", 0 - 1, "); fwd_s(isvoid ? "0" : "&r"); fwd_s(");\n");
    if (!isvoid) fwd_s("    return r;\n");
    fwd_s("}\n");
    return 1;
}
