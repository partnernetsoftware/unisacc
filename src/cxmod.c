/* ---- .cx modules, slice CX2a (research/cx2-module-design.md 1-4), shared by both drivers like src/fwdstub.c ----
   A .cx unit names the modules it uses with `#pragma unisa use NAME` (NAME = seg(.seg)*, seg = [a-z_][a-z0-9_]*,
   resolved beside the using file only, dots are directories; std and std.cx are the built-in prelude, other std.X
   are refused).  The driver takes the breadth-first closure of the command-line sources over `use` (each module once,
   appended after them), refuses cycles and a used module that defines main, and gives every .cx unit its forced
   includes: cx.h, the -include list, then the derived interface (CXIF v0) of each module in its use closure in
   dependency order -- the whole CXIF of a direct use, only the type part of a transitive one.
   CXIF v0 is text: one pass over the module source (splices joined, comments removed, strings respected) that splits
   file-scope items at a depth-0 ';' or at the '}' closing a function body, and exports the prototype of each
   non-static function definition and every typedef / struct-union-enum definition, whitespace collapsed.  When
   cxpack.tsv sits in the module file's directory and a row's path matches that file, only the comma-separated
   public names on that row are exported; a module with no such row still exports every non-static function.
   An exported
   object, a prototype the module does not define, a module macro inside an exported item, an export under #if and a
   reserved name prefix are refused by name.  Collisions (one function exported by two modules, a unit defining a
   name a module exports, one type name from two modules a unit can see) are refused before compilation.
   Each CXIF is written once, content-addressed, to $TMPDIR/ua-cxif-<sha16>.h; its bytes depend on the module
   bytes, this generator, and a matching cxpack.tsv row when one exists.  .c units are never scanned and their include list is exactly the -include list.
   The driver supplies cxm_rd (length, -1 ENOENT only, -2 too large, -3 other read/open/close error), cxm_wr (write a file) and cxm_say
   (bytes to fd 1 or 2).  Inspection: -cx-interface M.cx prints the CXIF, -cx-plan prints the units as TSV:
   index, compile path, forced includes. */
#include "../include/unisa_sha256.h"
int cxm_rd(char *path, char *buf, int cap);
int cxm_wr(char *path, char *buf, int n);
int cxm_say(int fd, char *s, int n);

#define CXM_MAXU 64
#define CXM_MAXUSE 32
#define CXM_PATHW 512
#define CXM_NAMEW 128
#define CXM_SRCMAX 1048576
#define CXM_IFMAX 262144
#define CXM_ITEMS 8192
#define CXM_SYMS 2048
#define CXM_SYMW 64
#define CXM_MACROS 1024

int cxm_wantplan; int cxm_wantif;
char cxm_src[CXM_SRCMAX + 1]; int cxm_nsrc;
char cxm_cmp[CXM_IFMAX + 1];
char cxm_out[CXM_IFMAX + 1]; int cxm_nout;
char cxm_msg[2048]; int cxm_nmsg;
int cxm_n; int cxm_on;
char *cxm_arg[CXM_MAXU];
char cxm_path[CXM_MAXU][CXM_PATHW];
char cxm_name[CXM_MAXU][CXM_NAMEW];
char cxm_stem[CXM_MAXU][CXM_NAMEW];
int cxm_iscx[CXM_MAXU]; int cxm_lib[CXM_MAXU];
int cxm_use[CXM_MAXU][CXM_MAXUSE]; int cxm_nuse[CXM_MAXU];
/* per use, the imported symbol and its alias (both empty = import the whole module) */
char cxm_usym[CXM_MAXU][CXM_MAXUSE][CXM_SYMW]; char cxm_ualias[CXM_MAXU][CXM_MAXUSE][CXM_SYMW];
char cxm_reach[CXM_MAXU][CXM_MAXU];
int cxm_topo[CXM_MAXU]; int cxm_ntopo; int cxm_color[CXM_MAXU]; int cxm_stk[CXM_MAXU]; int cxm_nstk;
char cxm_ift[CXM_IFMAX + 1]; int cxm_nift;
int cxm_tb[CXM_MAXU]; int cxm_te[CXM_MAXU]; int cxm_fb[CXM_MAXU]; int cxm_fe[CXM_MAXU];
char cxm_sha[CXM_MAXU][17];
char cxm_full[CXM_MAXU][CXM_PATHW]; char cxm_typ[CXM_MAXU][CXM_PATHW];
int cxm_inc[CXM_MAXU][CXM_MAXU]; int cxm_ninc[CXM_MAXU];
/* per-use alias projection: cxm_ren[u][k] is the renamed full CXIF path when use k has an alias, else empty */
char cxm_ren[CXM_MAXU][CXM_MAXUSE][CXM_PATHW];
/* renamed-CXIF path pool; cxm_inc[u][k] < 0 means cxm_renpool[-(x)-1] */
char cxm_renpool[CXM_MAXU * CXM_MAXUSE][CXM_PATHW]; int cxm_nren;
/* exported and defined names: kind 0 a module's function, 1 a module's type name (typedef, "struct T", enum
   constant), 2 a non-static function defined by a unit that is not a module */
char cxm_sym[CXM_SYMS][CXM_SYMW]; int cxm_symu[CXM_SYMS]; int cxm_syml[CXM_SYMS]; int cxm_symk[CXM_SYMS];
int cxm_symb[CXM_SYMS]; int cxm_syme[CXM_SYMS]; int cxm_nsym;
/* the file being scanned: its items and macros */
int cxm_ib[CXM_ITEMS]; int cxm_ie[CXM_ITEMS]; int cxm_ibody[CXM_ITEMS]; int cxm_iline[CXM_ITEMS]; int cxm_icond[CXM_ITEMS];
int cxm_ikind[CXM_ITEMS]; int cxm_nitem;
char cxm_mac[CXM_MACROS][CXM_SYMW]; int cxm_nmac;
int cxm_tokb; int cxm_tokl;

int cxm_len(char *s) { int n; n = 0; while (s[n]) n = n + 1; return n; }
int cxm_eq(char *a, char *b) { int k; k = 0; while (a[k] && a[k] == b[k]) k = k + 1; return a[k] == b[k]; }
int cxm_eqn(char *a, int n, char *b) { int k; k = 0; while (k < n && b[k] && a[k] == b[k]) k = k + 1; return k == n && b[k] == 0; }
int cxm_idch(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; }
int cxm_cpy(char *d, char *s, int cap) { int k; k = 0; while (s[k] && k < cap - 1) { d[k] = s[k]; k = k + 1; } d[k] = 0; return k; }
int cxm_cpyn(char *d, char *s, int n, int cap) { int k; k = 0; while (k < n && k < cap - 1) { d[k] = s[k]; k = k + 1; } d[k] = 0; return k; }
/* the message buffer: "cx: ..." lines to stderr */
int cxm_m(char *s) { while (*s && cxm_nmsg < 2000) { cxm_msg[cxm_nmsg] = *s; cxm_nmsg = cxm_nmsg + 1; s = s + 1; } return 0; }
int cxm_mn(char *s, int n) { int k; k = 0; while (k < n && cxm_nmsg < 2000) { cxm_msg[cxm_nmsg] = s[k]; cxm_nmsg = cxm_nmsg + 1; k = k + 1; } return 0; }
int cxm_md(int v) { char d[16]; int n; n = 0; if (v == 0) { d[0] = '0'; n = 1; } while (v > 0) { d[n] = '0' + v % 10; v = v / 10; n = n + 1; } while (n > 0) { n = n - 1; cxm_mn(d + n, 1); } return 0; }
int cxm_start(int u, int line) {         /* "cx: PATH:LINE: " (no location when u < 0) */
    cxm_nmsg = 0; cxm_m("cx: ");
    if (u >= 0) { cxm_m(cxm_arg[u]); cxm_m(":"); cxm_md(line); cxm_m(": "); }
    return 0;
}
int cxm_fail(void) { cxm_m("\n"); cxm_say(2, cxm_msg, cxm_nmsg); return 1; }
/* the out buffer: generated text */
int cxm_o(char *s) { while (*s && cxm_nout < CXM_IFMAX) { cxm_out[cxm_nout] = *s; cxm_nout = cxm_nout + 1; s = s + 1; } return 0; }
int cxm_on2(char *s, int n) { int k; k = 0; while (k < n && cxm_nout < CXM_IFMAX) { cxm_out[cxm_nout] = s[k]; cxm_nout = cxm_nout + 1; k = k + 1; } return 0; }
/* like cxm_on2 over s[b..e), but every identifier token equal to SYM is written as
   ALIAS (both NUL-terminated; ALIAS empty means drop the rename).  Whitespace is
   copied as-is, so the rename is token-exact and never touches a longer name. */
int cxm_on_rename(char *s, int b, int e, char *sym, char *alias) {
    int i;
    i = b;
    while (i < e) {
        if (cxm_idch(s[i] & 255)) {
            int j = i; while (j < e && cxm_idch(s[j] & 255)) j = j + 1;
            if (sym && sym[0] && alias && alias[0] && cxm_eqn(s + i, j - i, sym)) { cxm_o(alias); }
            else { cxm_on2(s + i, j - i); }
            i = j;
        } else { cxm_on2(s + i, 1); i = i + 1; }
    }
    return 0;
}

/* lexical normalisation: no "." or empty segments, "x/.." folded; no filesystem query */
int cxm_norm(char *in, char *out) {
    int i; int o; int seg[256]; int ns; int b; int l;
    i = 0; o = 0; ns = 0;
    if (in[0] == '/') { out[0] = '/'; o = 1; i = 1; }
    while (in[i]) {
        b = i; while (in[i] && in[i] != '/') i = i + 1;
        l = i - b;
        if (in[i] == '/') i = i + 1;
        if (l == 0) continue;
        if (l == 1 && in[b] == '.') continue;
        if (l == 2 && in[b] == '.' && in[b + 1] == '.' && ns > 0) {
            if (!(out[seg[ns - 1]] == '.' && out[seg[ns - 1] + 1] == '.' && (out[seg[ns - 1] + 2] == '/' || seg[ns - 1] + 2 == o))) {
                ns = ns - 1; o = seg[ns]; out[o] = 0; continue;
            }
        }
        if (ns > 0 || (o > 0 && out[o - 1] != '/')) { out[o] = '/'; o = o + 1; }
        if (o + l >= CXM_PATHW - 4 || ns >= 255) return 1;
        seg[ns] = o; ns = ns + 1;
        while (l > 0) { out[o] = in[b]; o = o + 1; b = b + 1; l = l - 1; }
    }
    if (o == 0) { out[0] = '.'; o = 1; }
    out[o] = 0;
    return 0;
}
int cxm_iscxpath(char *p) { int n; n = cxm_len(p); return n >= 3 && p[n - 3] == '.' && p[n - 2] == 'c' && p[n - 1] == 'x'; }
int cxm_stemof(char *p, char *out) {
    int n; int b; n = cxm_len(p); b = n;
    while (b > 0 && p[b - 1] != '/') b = b - 1;
    if (n - b >= 3 && p[n - 3] == '.' && p[n - 2] == 'c' && p[n - 1] == 'x') n = n - 3;
    return cxm_cpyn(out, p + b, n - b, CXM_NAMEW);
}
/* a module's compile path, kept for the whole process: the driver's unit cache keys on the path string, and a
   second plan in the same process (the forwarding restart) must hand out the same strings */
char cxm_pool[65536]; int cxm_npool;
char *cxm_keep(char *p) {
    int at; int n;
    at = 0;
    while (at < cxm_npool) { if (cxm_eq(cxm_pool + at, p)) return cxm_pool + at; at = at + cxm_len(cxm_pool + at) + 1; }
    n = cxm_len(p);
    if (cxm_npool + n + 1 > 65536) return 0;
    cxm_cpy(cxm_pool + cxm_npool, p, n + 1);
    cxm_npool = cxm_npool + n + 1;
    return cxm_pool + at;
}
int cxm_find(char *path) { int u; u = 0; if (cxm_on == 0) return 0 - 1; while (u < cxm_n) { if (cxm_eq(cxm_arg[u], path)) return u; u = u + 1; } return 0 - 1; }
int cxm_flag(char *a) {
    if (cxm_eq(a, "-cx-plan")) { cxm_wantplan = 1; return 1; }
    if (cxm_eq(a, "-cx-interface")) { cxm_wantif = 1; return 1; }
    return 0;
}

/* read unit u into cxm_src and clean it in place: each splice becomes \001 + its newline, comments become blanks
   (their newlines kept), a shebang line is blanked; strings and character constants are kept as they are */
int cxm_load(char *path, int u) {
    int n; int i; int q; char *s;
    n = cxm_rd(path, cxm_src, CXM_SRCMAX);
    if (n == 0 - 2) { cxm_start(u, 1); cxm_m("source too large for the module scan"); return cxm_fail(); }
    if (n < 0) return 0 - 1;
    cxm_nsrc = n; s = cxm_src; s[n] = 0;
    i = 0;
    if (n > 1 && s[0] == '#' && s[1] == '!') { while (i < n && s[i] != '\n') { s[i] = ' '; i = i + 1; } }
    while (i < n) {
        if (s[i] == '\\' && i + 1 < n && s[i + 1] == '\n') { s[i] = 1; i = i + 2; continue; }
        if (s[i] == '/' && i + 1 < n && s[i + 1] == '*') {
            s[i] = ' '; s[i + 1] = ' '; i = i + 2;
            while (i < n && !(s[i] == '*' && i + 1 < n && s[i + 1] == '/')) { if (s[i] != '\n') s[i] = ' '; i = i + 1; }
            if (i < n) { s[i] = ' '; s[i + 1] = ' '; i = i + 2; }
            continue;
        }
        if (s[i] == '/' && i + 1 < n && s[i + 1] == '/') { while (i < n && s[i] != '\n') { s[i] = ' '; i = i + 1; } continue; }
        if (s[i] == '"' || s[i] == 39) {
            q = s[i]; i = i + 1;
            while (i < n && s[i] != q && s[i] != '\n') { if (s[i] == '\\' && i + 1 < n) i = i + 1; i = i + 1; }
            if (i < n && s[i] == q) i = i + 1;
            continue;
        }
        i = i + 1;
    }
    return 0;
}
/* one token of s[i..e): an identifier or number, a string or character constant, or one punctuator; whitespace
   (and the splice mark) is skipped.  Sets cxm_tokb/cxm_tokl (0 at the end) and returns the index after it. */
int cxm_tok(char *s, int i, int e) {
    int q;
    while (i < e && (s[i] & 255) <= 32) i = i + 1;
    cxm_tokb = i; cxm_tokl = 0;
    if (i >= e) return e;
    if (cxm_idch(s[i] & 255)) { while (i < e && cxm_idch(s[i] & 255)) i = i + 1; cxm_tokl = i - cxm_tokb; return i; }
    if (s[i] == '"' || s[i] == 39) {
        q = s[i]; i = i + 1;
        while (i < e && s[i] != q) { if (s[i] == '\\') i = i + 1; i = i + 1; }
        if (i < e) i = i + 1;
        cxm_tokl = i - cxm_tokb; return i;
    }
    cxm_tokl = 1; return i + 1;
}
int cxm_is(char *s, char *w) { return cxm_eqn(s + cxm_tokb, cxm_tokl, w); }
int cxm_kw(char *t, int l) {
    char *k; int i; int b;
    k = "auto char const double enum extern float inline int long register restrict short signed static struct "
        "typedef union unsigned void volatile _Bool _Complex _Imaginary _Noreturn __inline __inline__ __restrict "
        "__restrict__ __const __volatile__ __signed__ __extension__ _Thread_local sizeof ";
    i = 0;
    while (k[i]) {
        b = i; while (k[i] != ' ') i = i + 1;
        if (i - b == l) { int j; j = 0; while (j < l && k[b + j] == t[j]) j = j + 1; if (j == l) return 1; }
        i = i + 1;
    }
    return 0;
}
/* skip a balanced group opened by the token just read (s[cxm_tokb] is the opener) */
int cxm_skipgrp(char *s, int i, int e) {
    int d; int o; int c;
    o = s[cxm_tokb]; c = o == '(' ? ')' : (o == '[' ? ']' : '}'); d = 1;
    while (i < e && d > 0) {
        i = cxm_tok(s, i, e); if (cxm_tokl == 0) break;
        if (cxm_tokl == 1 && s[cxm_tokb] == o) d = d + 1;
        if (cxm_tokl == 1 && s[cxm_tokb] == c) d = d - 1;
    }
    return i;
}
/* the declarator name of the declaration s[b..e): the first identifier that is not a keyword, a tag or a type
   name (an identifier followed by an identifier, '*' or "( *").  *isfn: the name is followed by '(' */
int cxm_dname(char *s, int b, int e, int *nb, int *nl, int *isfn) {
    int i; int j; int tb; int tl;
    i = b;
    while (i < e) {
        i = cxm_tok(s, i, e); if (cxm_tokl == 0) return 0;
        tb = cxm_tokb; tl = cxm_tokl;
        if (tl == 1 && s[tb] == '=') return 0;
        if (tl == 1 && (s[tb] == '{' || s[tb] == '[')) { i = cxm_skipgrp(s, i, e); continue; }
        if (!cxm_idch(s[tb] & 255) || (s[tb] >= '0' && s[tb] <= '9')) continue;
        if (cxm_eqn(s + tb, tl, "struct") || cxm_eqn(s + tb, tl, "union") || cxm_eqn(s + tb, tl, "enum")) {
            j = cxm_tok(s, i, e); if (cxm_tokl > 0 && cxm_idch(s[cxm_tokb] & 255)) i = j;
            continue;
        }
        if (cxm_eqn(s + tb, tl, "__attribute__") || cxm_eqn(s + tb, tl, "__attribute") || cxm_eqn(s + tb, tl, "__asm__")
            || cxm_eqn(s + tb, tl, "__asm") || cxm_eqn(s + tb, tl, "asm") || cxm_eqn(s + tb, tl, "__declspec")
            || cxm_eqn(s + tb, tl, "_Alignas")) {
            j = cxm_tok(s, i, e); if (cxm_tokl == 1 && s[cxm_tokb] == '(') i = cxm_skipgrp(s, j, e);
            continue;
        }
        if (cxm_kw(s + tb, tl)) continue;
        j = cxm_tok(s, i, e);
        if (cxm_tokl > 0 && cxm_idch(s[cxm_tokb] & 255)) continue;
        if (cxm_tokl == 1 && s[cxm_tokb] == '*') continue;
        if (cxm_tokl == 1 && s[cxm_tokb] == '(') { cxm_tok(s, j, e); if (cxm_tokl == 1 && s[cxm_tokb] == '*') continue; cxm_tokb = j - 1; cxm_tokl = 1; }
        *nb = tb; *nl = tl; *isfn = cxm_tokl == 1 && s[cxm_tokb] == '(';
        return 1;
    }
    return 0;
}
/* keyword w at paren/brace depth 0 of s[b..e) (before an initializer) */
int cxm_haskw(char *s, int b, int e, char *w) {
    int i; int d;
    i = b; d = 0;
    while (i < e) {
        i = cxm_tok(s, i, e); if (cxm_tokl == 0) return 0;
        if (cxm_tokl == 1) {
            if (s[cxm_tokb] == '(' || s[cxm_tokb] == '[' || s[cxm_tokb] == '{') d = d + 1;
            if (s[cxm_tokb] == ')' || s[cxm_tokb] == ']' || s[cxm_tokb] == '}') d = d - 1;
            if (s[cxm_tokb] == '=' && d == 0) return 0;
        }
        if (d == 0 && cxm_is(s, w)) return 1;
    }
    return 0;
}
int cxm_firstis(char *s, int b, int e, char *w) {
    int i; i = b;
    while (i < e) {
        i = cxm_tok(s, i, e); if (cxm_tokl == 0) return 0;
        if (cxm_is(s, "const") || cxm_is(s, "volatile") || cxm_is(s, "__extension__")) continue;
        return cxm_is(s, w);
    }
    return 0;
}

/* the module name grammar: seg(.seg)*, seg = [a-z_][a-z0-9_]* */
int cxm_goodname(char *w, int n) {
    int i; int st;
    if (n <= 0 || n >= CXM_NAMEW) return 0;
    i = 0; st = 1;
    while (i < n) {
        if (w[i] == '.') { if (st) return 0; st = 1; i = i + 1; continue; }
        if (st) { if (!((w[i] >= 'a' && w[i] <= 'z') || w[i] == '_')) return 0; st = 0; }
        else { if (!((w[i] >= 'a' && w[i] <= 'z') || (w[i] >= '0' && w[i] <= '9') || w[i] == '_')) return 0; }
        i = i + 1;
    }
    return st == 0;
}
/* `#pragma unisa use NAME` (optionally `NAME::sym as alias`) in unit u at line */
int cxm_adduse(int u, char *w, int n, int line, char *sym, int sn, char *alias, int an) {
    char cand[CXM_PATHW]; char norm[CXM_PATHW]; int dl; int k; int v; int p;
    if (!cxm_goodname(w, n)) { cxm_start(u, line); cxm_m("bad module name \""); cxm_mn(w, n); cxm_m("\""); return cxm_fail(); }
    if (cxm_eqn(w, n, "std") || cxm_eqn(w, n, "std.cx")) return 0;        /* the prelude: always there */
    if (n > 4 && w[0] == 's' && w[1] == 't' && w[2] == 'd' && w[3] == '.') {
        cxm_start(u, line); cxm_m("unknown std module "); cxm_mn(w, n); return cxm_fail();
    }
    dl = cxm_len(cxm_path[u]);
    while (dl > 0 && cxm_path[u][dl - 1] != '/') dl = dl - 1;
    if (dl + n + 4 >= CXM_PATHW) { cxm_start(u, line); cxm_m("module path too long"); return cxm_fail(); }
    p = 0; k = 0;
    while (k < dl) { cand[p] = cxm_path[u][k]; p = p + 1; k = k + 1; }
    k = 0; while (k < n) { cand[p] = w[k] == '.' ? '/' : w[k]; p = p + 1; k = k + 1; }
    cand[p] = '.'; cand[p + 1] = 'c'; cand[p + 2] = 'x'; cand[p + 3] = 0;
    if (cxm_norm(cand, norm)) { cxm_start(u, line); cxm_m("module path too long"); return cxm_fail(); }
    v = 0; while (v < cxm_n && !cxm_eq(cxm_path[v], norm)) v = v + 1;
    if (v == cxm_n) {
        if (cxm_rd(norm, cxm_cmp, 1) == 0 - 1) {
            cxm_start(u, line); cxm_m("module "); cxm_mn(w, n); cxm_m(": no file "); cxm_m(norm); return cxm_fail();
        }
        if (cxm_n >= CXM_MAXU) { cxm_start(u, line); cxm_m("more than 64 units in one program"); return cxm_fail(); }
        cxm_cpy(cxm_path[v], norm, CXM_PATHW); cxm_arg[v] = cxm_keep(norm);
        if (cxm_arg[v] == 0) { cxm_start(u, line); cxm_m("too many module paths in one process"); return cxm_fail(); }
        cxm_cpyn(cxm_name[v], w, n, CXM_NAMEW); cxm_stemof(norm, cxm_stem[v]);
        cxm_iscx[v] = 1; cxm_lib[v] = 0; cxm_nuse[v] = 0;
        cxm_n = cxm_n + 1;
    }
    if (!cxm_lib[v]) cxm_cpyn(cxm_name[v], w, n, CXM_NAMEW);   /* a command-line unit used by name takes that name */
    cxm_lib[v] = 1;
    k = 0; while (k < cxm_nuse[u]) { if (cxm_use[u][k] == v) return 0; k = k + 1; }
    if (cxm_nuse[u] >= CXM_MAXUSE) { cxm_start(u, line); cxm_m("more than 32 uses in one unit"); return cxm_fail(); }
    cxm_use[u][cxm_nuse[u]] = v;
    cxm_cpyn(cxm_usym[u][cxm_nuse[u]], sym, sn, CXM_SYMW);
    cxm_cpyn(cxm_ualias[u][cxm_nuse[u]], alias, an, CXM_SYMW);
    cxm_nuse[u] = cxm_nuse[u] + 1;
    return 0;
}
/* a word on a directive line: the next run of non-blank bytes */
int cxm_word(char *s, int i, int e, int *wb) {
    while (i < e && (s[i] & 255) <= 32) i = i + 1;
    *wb = i;
    while (i < e && (s[i] & 255) > 32) i = i + 1;
    return i;
}
int cxm_id(char *s, int i, int e, int *wb) {
    while (i < e && (s[i] & 255) <= 32) i = i + 1;
    *wb = i;
    while (i < e && cxm_idch(s[i] & 255)) i = i + 1;
    return i;
}
/* the directive at s[i] ('#' at the start of a line), ending at the first newline not after a splice.  mode 0 handles
   `#pragma unisa`; every mode tracks #if nesting (*cond) and records #define names.  The line is blanked after. */
int cxm_directive(int u, int mode, int i, int line, int *cond, int seen, int *endp) {
    char *s; int e; int a; int b; int wb; int we; int k;
    s = cxm_src; e = i;
    while (e < cxm_nsrc && !(s[e] == '\n' && (e == 0 || s[e - 1] != 1))) e = e + 1;
    *endp = e;
    b = cxm_id(s, i + 1, e, &a);
    if (cxm_eqn(s + a, b - a, "if") || cxm_eqn(s + a, b - a, "ifdef") || cxm_eqn(s + a, b - a, "ifndef")) *cond = *cond + 1;
    if (cxm_eqn(s + a, b - a, "endif") && *cond > 0) *cond = *cond - 1;
    if (cxm_eqn(s + a, b - a, "define")) {
        we = cxm_id(s, b, e, &wb);
        if (we > wb && cxm_nmac < CXM_MACROS) { cxm_cpyn(cxm_mac[cxm_nmac], s + wb, we - wb, CXM_SYMW); cxm_nmac = cxm_nmac + 1; }
    }
    if (cxm_eqn(s + a, b - a, "pragma")) {
        we = cxm_id(s, b, e, &wb);
        if (cxm_eqn(s + wb, we - wb, "unisa") && (we >= e || (s[we] & 255) <= 32) && mode == 0) {
            if (seen) { cxm_start(u, line); cxm_m("unisa pragma after first declaration"); return cxm_fail(); }
            if (*cond > 0) { cxm_start(u, line); cxm_m("unisa pragma inside conditional"); return cxm_fail(); }
            b = cxm_word(s, we, e, &a);
            if (cxm_eqn(s + a, b - a, "use") || cxm_eqn(s + a, b - a, "module")) {
                we = cxm_word(s, b, e, &wb);
                if (we == wb) { cxm_start(u, line); cxm_m("unisa "); cxm_mn(s + a, b - a); cxm_m(" takes a module name"); return cxm_fail(); }
                if (s[a] == 'u') {
                    /* NAME [::sym [as alias]] */
                    char *w = s + wb; int wn = we - wb; int nl = wn; int sb = 0; int sl = 0; int ab = 0; int al = 0; int q;
                    k = 0; while (k < wn && w[k] != ':') k = k + 1;
                    if (k < wn && w[k + 1] == ':') {
                        nl = k;
                        sb = k + 2; k = k + 2; sl = 0;
                        while (k < wn && cxm_idch(w[k] & 255)) { sl = sl + 1; k = k + 1; }
                        q = cxm_word(s, we, e, &i);
                        if (q > i && cxm_eqn(s + i, q - i, "as")) {
                            q = cxm_word(s, q, e, &ab);
                            if (q > ab) { al = q - ab; }
                        }
                    }
                    if (cxm_adduse(u, w, nl, line, w + sb, sl, (al ? s + ab : 0), al)) return 1;
                }
                else {
                    if (!cxm_eqn(s + wb, we - wb, cxm_stem[u])) {
                        cxm_start(u, line); cxm_m("module "); cxm_mn(s + wb, we - wb); cxm_m(" is not the file stem "); cxm_m(cxm_stem[u]); return cxm_fail();
                    }
                }
            } else { cxm_start(u, line); cxm_m("unknown unisa pragma "); cxm_mn(s + a, b - a); return cxm_fail(); }
        }
    }
    k = i; while (k < e) { if (s[k] != '\n') s[k] = ' '; k = k + 1; }
    return 0;
}
/* scan the loaded unit: directives (mode 0: the unisa pragmas), then the file-scope items (cxm_ib..) */
int cxm_scan(int u, int mode) {
    char *s; int n; int i; int line; int bol; int depth; int cond; int seen; int ib; int iline; int icond; int body; int eq; int last; int q; int e;
    s = cxm_src; n = cxm_nsrc; i = 0; line = 1; bol = 1; depth = 0; cond = 0; seen = 0;
    ib = 0 - 1; iline = 0; icond = 0; body = 0 - 1; eq = 0; last = 0;
    cxm_nitem = 0; cxm_nmac = 0;
    while (i < n) {
        if (s[i] == '\n') { line = line + 1; bol = 1; i = i + 1; continue; }
        if ((s[i] & 255) <= 32) { i = i + 1; continue; }
        if (bol && s[i] == '#') {
            if (cxm_directive(u, mode, i, line, &cond, seen, &e)) return 1;
            i = e; continue;
        }
        bol = 0; seen = 1;
        if (ib < 0) { ib = i; iline = line; icond = cond > 0; body = 0 - 1; eq = 0; }
        if (cond > 0) icond = 1;
        if (s[i] == '"' || s[i] == 39) {
            q = s[i]; i = i + 1;
            while (i < n && s[i] != q && s[i] != '\n') { if (s[i] == '\\') i = i + 1; i = i + 1; }
            if (i < n && s[i] == q) i = i + 1;
            last = q; continue;
        }
        if (cxm_idch(s[i] & 255)) { while (i < n && cxm_idch(s[i] & 255)) i = i + 1; last = 'a'; continue; }
        if (s[i] == '(' || s[i] == '[') depth = depth + 1;
        if (s[i] == ')' || s[i] == ']') { if (depth > 0) depth = depth - 1; }
        if (s[i] == '=' && depth == 0) eq = 1;
        if (s[i] == '{') { if (depth == 0 && eq == 0 && last == ')' && body < 0) body = i; depth = depth + 1; }
        if (s[i] == '}') {
            if (depth > 0) depth = depth - 1;
            if (depth == 0 && body >= 0) {
                if (cxm_nitem >= CXM_ITEMS) { cxm_start(u, line); cxm_m("too many file-scope items for the module scan"); return cxm_fail(); }
                cxm_ib[cxm_nitem] = ib; cxm_ie[cxm_nitem] = i + 1; cxm_ibody[cxm_nitem] = body; cxm_iline[cxm_nitem] = iline;
                cxm_icond[cxm_nitem] = icond; cxm_nitem = cxm_nitem + 1; ib = 0 - 1; i = i + 1; last = '}'; continue;
            }
        }
        if (s[i] == ';' && depth == 0) {
            if (cxm_nitem >= CXM_ITEMS) { cxm_start(u, line); cxm_m("too many file-scope items for the module scan"); return cxm_fail(); }
            cxm_ib[cxm_nitem] = ib; cxm_ie[cxm_nitem] = i + 1; cxm_ibody[cxm_nitem] = 0 - 1; cxm_iline[cxm_nitem] = iline;
            cxm_icond[cxm_nitem] = icond; cxm_nitem = cxm_nitem + 1; ib = 0 - 1;
        }
        last = s[i]; i = i + 1;
    }
    return 0;
}

int cxm_addsym(char *t, int l, int u, int line, int kind, int b, int e) {
    if (cxm_nsym >= CXM_SYMS) { cxm_start(u, line); cxm_m("too many names for the module check"); return cxm_fail(); }
    cxm_cpyn(cxm_sym[cxm_nsym], t, l, CXM_SYMW);
    cxm_symu[cxm_nsym] = u; cxm_syml[cxm_nsym] = line; cxm_symk[cxm_nsym] = kind; cxm_symb[cxm_nsym] = b; cxm_syme[cxm_nsym] = e;
    cxm_nsym = cxm_nsym + 1;
    return 0;
}
int cxm_ismac(char *t, int l) { int k; k = 0; while (k < cxm_nmac) { if (cxm_eqn(t, l, cxm_mac[k])) return 1; k = k + 1; } return 0; }
int cxm_reserved(char *t, int l) {
    if (l >= 3 && t[0] == 'c' && t[1] == 'x' && t[2] == '_') return 3;
    if (l >= 4 && t[0] == '_' && t[1] == 'c' && t[2] == 'x' && t[3] == '_') return 4;
    if (l >= 4 && t[0] == '_' && t[1] == '_' && t[2] == 'u' && t[3] == '_') return 4;
    return 0;
}
/* the export checks of one exported item s[b..e) named nm: #if, module macros, reserved prefix */
int cxm_exportok(int u, int k, int b, int e, char *nm, int nl) {
    char *s; int i; int r;
    s = cxm_src;
    if (cxm_icond[k]) {
        cxm_start(u, cxm_iline[k]); cxm_m("module "); cxm_m(cxm_name[u]); cxm_m(" exports "); cxm_mn(nm, nl);
        cxm_m(" under #if (conditional interface)"); return cxm_fail();
    }
    i = b;
    while (i < e) {
        i = cxm_tok(s, i, e); if (cxm_tokl == 0) break;
        if (cxm_idch(s[cxm_tokb] & 255) && cxm_ismac(s + cxm_tokb, cxm_tokl)) {
            cxm_start(u, cxm_iline[k]); cxm_m("exported "); cxm_mn(nm, nl); cxm_m(" uses macro "); cxm_mn(s + cxm_tokb, cxm_tokl);
            cxm_m("; make it an enum constant"); return cxm_fail();
        }
    }
    r = cxm_reserved(nm, nl);
    if (r) {
        cxm_start(u, cxm_iline[k]); cxm_m("module "); cxm_m(cxm_name[u]); cxm_m(" exports "); cxm_mn(nm, nl);
        cxm_m("; the "); cxm_mn(nm, r); cxm_m(" prefix is reserved for the prelude"); return cxm_fail();
    }
    return 0;
}
/* append s[b..e) to cxm_ift with blanks collapsed (string and character constants as they are) */
int cxm_emit(int b, int e) {
    char *s; int i; int sp; int q;
    s = cxm_src; i = b; sp = 0;
    while (i < e && (s[i] & 255) <= 32) i = i + 1;
    while (i < e) {
        if ((s[i] & 255) <= 32) { sp = 1; i = i + 1; continue; }
        if (cxm_nift >= CXM_IFMAX - 2) return 1;
        if (sp) { cxm_ift[cxm_nift] = ' '; cxm_nift = cxm_nift + 1; sp = 0; }
        if (s[i] == '"' || s[i] == 39) {
            q = s[i]; cxm_ift[cxm_nift] = s[i]; cxm_nift = cxm_nift + 1; i = i + 1;
            while (i < e && s[i] != q && cxm_nift < CXM_IFMAX - 2) {
                if (s[i] == '\\' && i + 1 < e) { cxm_ift[cxm_nift] = s[i]; cxm_nift = cxm_nift + 1; i = i + 1; }
                cxm_ift[cxm_nift] = s[i]; cxm_nift = cxm_nift + 1; i = i + 1;
            }
            if (i < e) { cxm_ift[cxm_nift] = s[i]; cxm_nift = cxm_nift + 1; i = i + 1; }
            continue;
        }
        cxm_ift[cxm_nift] = s[i]; cxm_nift = cxm_nift + 1; i = i + 1;
    }
    return 0;
}
/* the names a type item defines: its typedef name, "struct T" for a tag with a body, its enum constants */
int cxm_typenames(int u, int k, int b, int e, int istypedef, int tb, int te) {
    char *s; int i; int j; int nb; int nl; int isfn; char key[CXM_SYMW]; int kl; int d; int prev;
    s = cxm_src; i = b;
    while (i < e) {
        i = cxm_tok(s, i, e); if (cxm_tokl == 0) break;
        if (cxm_is(s, "struct") || cxm_is(s, "union") || cxm_is(s, "enum")) {
            kl = cxm_cpyn(key, s + cxm_tokb, cxm_tokl, CXM_SYMW - 2);
            j = cxm_tok(s, i, e);
            if (cxm_tokl > 0 && cxm_idch(s[cxm_tokb] & 255)) {
                int tb2; int tl2; tb2 = cxm_tokb; tl2 = cxm_tokl;
                cxm_tok(s, j, e);
                if (cxm_tokl == 1 && s[cxm_tokb] == '{') {
                    key[kl] = ' '; cxm_cpyn(key + kl + 1, s + tb2, tl2, CXM_SYMW - kl - 1);
                    if (cxm_exportok(u, k, b, b, key + kl + 1, tl2)) return 1;
                    if (cxm_addsym(key, cxm_len(key), u, cxm_iline[k], 1, tb, te)) return 1;
                }
            }
            if (key[0] == 'e') {           /* enum constants: identifiers at the start of the body or after a ',' */
                while (i < e) { i = cxm_tok(s, i, e); if (cxm_tokl == 0 || (cxm_tokl == 1 && s[cxm_tokb] == '{')) break; }
                d = 1; prev = 1;
                while (i < e && d > 0) {
                    i = cxm_tok(s, i, e); if (cxm_tokl == 0) break;
                    if (cxm_tokl == 1 && (s[cxm_tokb] == '{' || s[cxm_tokb] == '(' || s[cxm_tokb] == '[')) d = d + 1;
                    if (cxm_tokl == 1 && (s[cxm_tokb] == '}' || s[cxm_tokb] == ')' || s[cxm_tokb] == ']')) d = d - 1;
                    if (d == 1 && prev && cxm_idch(s[cxm_tokb] & 255)) {
                        if (cxm_exportok(u, k, b, b, s + cxm_tokb, cxm_tokl)) return 1;
                        if (cxm_addsym(s + cxm_tokb, cxm_tokl, u, cxm_iline[k], 1, tb, te)) return 1;
                    }
                    prev = d == 1 && cxm_tokl == 1 && s[cxm_tokb] == ',';
                }
            }
            break;
        }
        if (cxm_is(s, "typedef") || cxm_is(s, "const") || cxm_is(s, "volatile")) continue;
        break;
    }
    if (istypedef && cxm_dname(s, b, e, &nb, &nl, &isfn)) {
        if (cxm_exportok(u, k, b, b, s + nb, nl)) return 1;
        if (cxm_addsym(s + nb, nl, u, cxm_iline[k], 1, tb, te)) return 1;
    }
    return 0;
}
/* 1 if nm is on the public list of the cxpack.tsv row for this file.  0 if that row exists and omits nm.
   2 if there is no manifest, or no row whose path is this file. */
int cxm_listed(int u, char *nm, int nl) {
    char man[CXM_PATHW]; char rel[CXM_PATHW];
    static char pack[65536];
    int dl; int k; int n; int i; int hit;
    char *s;
    dl = cxm_len(cxm_path[u]);
    while (dl > 0 && cxm_path[u][dl - 1] != '/') dl = dl - 1;
    if (dl + 12 >= CXM_PATHW) { cxm_start(u, 1); cxm_m("cxpack.tsv path too long"); cxm_fail(); return -1; }
    k = 0; while (k < dl) { man[k] = cxm_path[u][k]; k = k + 1; }
    if (cxm_cpy(man + dl, "cxpack.tsv", CXM_PATHW - dl) < 10) return 2;
    n = cxm_rd(man, pack, 65535);
    if (n == -1) return 2;
    if (n < 0) { cxm_start(u, 1); cxm_m(n == -2 ? "cxpack.tsv too large" : "cannot read cxpack.tsv"); cxm_fail(); return -1; }
    pack[n] = 0;
    k = 0;
    while (dl + k < cxm_len(cxm_path[u]) && k < CXM_PATHW - 1) { rel[k] = cxm_path[u][dl + k]; k = k + 1; }
    rel[k] = 0;
    s = pack; i = 0; hit = 0;
    while (i < n) {
        int line; int t1; int t2; int rl;
        line = i;
        while (i < n && s[i] != '\n') i = i + 1;
        if (line < i && s[line] != '#') {
            t1 = line; while (t1 < i && s[t1] != '\t') t1 = t1 + 1;
            t2 = t1 + 1; while (t2 < i && s[t2] != '\t') t2 = t2 + 1;
            rl = cxm_len(rel);
            if (t1 < i && t2 < i && t2 - (t1 + 1) == rl) {
                int q; q = 0;
                while (q < rl && s[t1 + 1 + q] == rel[q]) q = q + 1;
                if (q == rl) {
                    int p; int end;
                    hit = 1; p = t2 + 1; end = i;
                    if (end > p && s[end - 1] == '\r') end = end - 1;
                    while (p < end) {
                        int b; int e;
                        b = p; while (p < end && s[p] != ',') p = p + 1; e = p;
                        while (b < e && s[b] == ' ') b = b + 1;
                        while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\r')) e = e - 1;
                        if (e - b == nl) {
                            int q2; q2 = 0;
                            while (q2 < nl && s[b + q2] == nm[q2]) q2 = q2 + 1;
                            if (q2 == nl) return 1;
                        }
                        if (p < end && s[p] == ',') p = p + 1;
                    }
                }
            }
        }
        if (i < n && s[i] == '\n') i = i + 1;
    }
    if (hit) return 0;
    return 2;
}
/* project the loaded unit u.  lib: a used module -- classify every item, refuse by name, and write its interface
   text into cxm_ift; otherwise only record its non-static function definitions (collision check) */
int cxm_project(int u, int lib) {
    char *s; int k; int b; int e; int nb; int nl; int isfn; int j; int found; int ntypes; int tb;
    s = cxm_src;
    k = 0;
    while (k < cxm_nitem) {                          /* 0 private or nothing, 1 exported type, 2 exported function */
        b = cxm_ib[k]; e = cxm_ie[k]; cxm_ikind[k] = 0;
        cxm_tok(s, b, e);
        if (cxm_tokl == 0 || (cxm_tokl == 1 && s[cxm_tokb] == ';') || cxm_is(s, "_Static_assert")) { k = k + 1; continue; }
        if (cxm_haskw(s, b, cxm_ibody[k] >= 0 ? cxm_ibody[k] : e, "typedef")) cxm_ikind[k] = 1;
        else if (cxm_haskw(s, b, cxm_ibody[k] >= 0 ? cxm_ibody[k] : e, "static")) cxm_ikind[k] = 0;
        else if (cxm_ibody[k] >= 0) cxm_ikind[k] = 2;
        else if ((cxm_firstis(s, b, e, "struct") || cxm_firstis(s, b, e, "union") || cxm_firstis(s, b, e, "enum"))
                 && !cxm_dname(s, b, e, &nb, &nl, &isfn)) cxm_ikind[k] = 1;
        else cxm_ikind[k] = 3;                       /* a declaration: checked below */
        k = k + 1;
    }
    if (!lib) {
        k = 0;
        while (k < cxm_nitem) {
            if (cxm_ikind[k] == 2 && cxm_dname(s, cxm_ib[k], cxm_ibody[k], &nb, &nl, &isfn))
                if (cxm_addsym(s + nb, nl, u, cxm_iline[k], 2, 0, 0)) return 1;
            k = k + 1;
        }
        return 0;
    }
    k = 0;
    while (k < cxm_nitem) {
        b = cxm_ib[k]; e = cxm_ie[k];
        if (cxm_ikind[k] == 2) {
            if (!cxm_dname(s, b, cxm_ibody[k], &nb, &nl, &isfn)) { cxm_ikind[k] = 0; k = k + 1; continue; }
            if (cxm_eqn(s + nb, nl, "main")) { cxm_start(0 - 1, 0); cxm_m("module "); cxm_m(cxm_name[u]); cxm_m(" defines main (a used module is a library)"); return cxm_fail(); }
            if (cxm_exportok(u, k, b, cxm_ibody[k], s + nb, nl)) return 1;
        }
        if (cxm_ikind[k] == 3) {
            if (!cxm_dname(s, b, e, &nb, &nl, &isfn)) { cxm_ikind[k] = 0; k = k + 1; continue; }
            if (isfn) {
                found = 0; j = 0;
                while (j < cxm_nitem && !found) {
                    int nb2; int nl2; int f2;
                    if (cxm_ibody[j] >= 0 && cxm_dname(s, cxm_ib[j], cxm_ibody[j], &nb2, &nl2, &f2) && nl2 == nl) {
                        int q; q = 0; while (q < nl && s[nb + q] == s[nb2 + q]) q = q + 1; if (q == nl) found = 1;
                    }
                    j = j + 1;
                }
                if (!found) {
                    cxm_start(u, cxm_iline[k]); cxm_m("module "); cxm_m(cxm_name[u]); cxm_m(" declares "); cxm_mn(s + nb, nl);
                    cxm_m(" but does not define it (a module exports its own definitions)"); return cxm_fail();
                }
            } else if (!cxm_haskw(s, b, e, "extern")) {
                cxm_start(u, cxm_iline[k]); cxm_m("module "); cxm_m(cxm_name[u]); cxm_m(" exports object "); cxm_mn(s + nb, nl);
                cxm_m("; make it static and export a function"); return cxm_fail();
            }
            cxm_ikind[k] = 0;
        }
        k = k + 1;
    }
    cxm_tb[u] = cxm_nift; ntypes = 0;
    k = 0;
    while (k < cxm_nitem) {
        if (cxm_ikind[k] == 1) {
            int td; td = cxm_haskw(s, cxm_ib[k], cxm_ie[k], "typedef");
            if (td && cxm_dname(s, cxm_ib[k], cxm_ie[k], &nb, &nl, &isfn)) j = 0;
            else {                                   /* the tag, or the enum keyword of an anonymous enum */
                j = cxm_tok(s, cxm_ib[k], cxm_ie[k]);
                while (cxm_tokl > 0 && (cxm_is(s, "const") || cxm_is(s, "volatile") || cxm_is(s, "typedef"))) j = cxm_tok(s, j, cxm_ie[k]);
                nb = cxm_tokb; nl = cxm_tokl;
                cxm_tok(s, j, cxm_ie[k]); if (cxm_tokl > 0 && cxm_idch(s[cxm_tokb] & 255)) { nb = cxm_tokb; nl = cxm_tokl; }
            }
            if (cxm_exportok(u, k, cxm_ib[k], cxm_ie[k], s + nb, nl)) return 1;
            tb = cxm_nift;
            if (cxm_emit(cxm_ib[k], cxm_ie[k])) { cxm_start(u, cxm_iline[k]); cxm_m("interface too large"); return cxm_fail(); }
            if (cxm_typenames(u, k, cxm_ib[k], cxm_ie[k], td, tb, cxm_nift)) return 1;
            cxm_ift[cxm_nift] = '\n'; cxm_nift = cxm_nift + 1; ntypes = ntypes + 1;
        }
        k = k + 1;
    }
    cxm_te[u] = cxm_nift; cxm_fb[u] = cxm_nift;
    k = 0;
    while (k < cxm_nitem) {
        if (cxm_ikind[k] == 2) {
            int pub;
            cxm_dname(s, cxm_ib[k], cxm_ibody[k], &nb, &nl, &isfn);
            pub = cxm_listed(u, s + nb, nl);
            if (pub < 0) return 1;
            if (pub == 0) { k = k + 1; continue; }
            if (cxm_addsym(s + nb, nl, u, cxm_iline[k], 0, 0, 0)) return 1;
            if (cxm_emit(cxm_ib[k], cxm_ibody[k]) || cxm_nift >= CXM_IFMAX - 2) { cxm_start(u, cxm_iline[k]); cxm_m("interface too large"); return cxm_fail(); }
            cxm_ift[cxm_nift] = ';'; cxm_ift[cxm_nift + 1] = '\n'; cxm_nift = cxm_nift + 2;
        }
        k = k + 1;
    }
    cxm_fe[u] = cxm_nift;
    return 0;
}
/* the CXIF text of module u into cxm_out: full (types, then prototypes) or the type part only.
   When SYM is non-empty, every prototype identifier equal to SYM is written as ALIAS (the
   symbol-level import rename: `use m::sym as alias`).  Types are never renamed. */
int cxm_text(int u, int full, char *sym, char *alias) {
    char g[CXM_NAMEW]; int k;
    cxm_nout = 0;
    k = cxm_cpy(g, cxm_stem[u], CXM_NAMEW);
    while (k > 0) { k = k - 1; if (!cxm_idch(g[k] & 255)) g[k] = '_'; }
    cxm_o("/* CXIF v0 module "); cxm_o(cxm_stem[u]); if (!full) cxm_o(" types");
    if (sym && sym[0]) { cxm_o(" "); cxm_o(sym); cxm_o(" as "); cxm_o(alias); }
    cxm_o(" src-sha256 "); cxm_o(cxm_sha[u]); cxm_o(" -- generated, do not edit */\n");
    cxm_o("#ifndef CXIF_"); cxm_o(g); cxm_o("_"); cxm_o(cxm_sha[u]); cxm_o("\n");
    cxm_o("#define CXIF_"); cxm_o(g); cxm_o("_"); cxm_o(cxm_sha[u]); cxm_o("\n");
    cxm_on2(cxm_ift + cxm_tb[u], cxm_te[u] - cxm_tb[u]);
    if (full) cxm_on_rename(cxm_ift, cxm_fb[u], cxm_fe[u], sym ? sym : "", alias ? alias : "");
    cxm_o("#endif\n");
    return cxm_nout >= CXM_IFMAX;
}
/* write cxm_out to $TMPDIR/ua-cxif-<sha16>.h unless that file already holds these bytes */
int cxm_put(char *path) {
    char hex[65]; char *td; int p; int k; int n;
    unisa_sha256_hex(cxm_out, (size_t)cxm_nout, hex);
    td = getenv("TMPDIR"); if (td == 0 || td[0] == 0) td = getenv("TEMP");   /* TEMP: Windows, as the forwarding unit */
    if (td == 0 || td[0] == 0) td = "/tmp";
    p = cxm_cpy(path, td, CXM_PATHW - 40);
    k = cxm_cpy(path + p, "/ua-cxif-", 12); p = p + k;
    k = 0; while (k < 16) { path[p] = hex[k]; p = p + 1; k = k + 1; }
    path[p] = '.'; path[p + 1] = 'h'; path[p + 2] = 0;
    n = cxm_rd(path, cxm_cmp, CXM_IFMAX);
    if (n == cxm_nout) { k = 0; while (k < n && cxm_cmp[k] == cxm_out[k]) k = k + 1; if (k == n) return 0; }
    if (cxm_wr(path, cxm_out, cxm_nout)) { cxm_start(0 - 1, 0); cxm_m("cannot write the interface "); cxm_m(path); return cxm_fail(); }
    return 0;
}
int cxm_srcsha(int u) {
    char hex[65];
    unisa_sha256_hex(cxm_src, (size_t)cxm_nsrc, hex);
    cxm_cpyn(cxm_sha[u], hex, 16, 17);
    return 0;
}
/* depth-first over `use`: a back edge is a cycle (named), the post-order is the dependency order */
int cxm_dfs(int u) {
    int k; int v; int j;
    cxm_color[u] = 1; cxm_stk[cxm_nstk] = u; cxm_nstk = cxm_nstk + 1;
    k = 0;
    while (k < cxm_nuse[u]) {
        v = cxm_use[u][k];
        if (cxm_color[v] == 1) {
            cxm_start(0 - 1, 0); cxm_m("module cycle: ");
            j = 0; while (cxm_stk[j] != v) j = j + 1;
            while (j < cxm_nstk) { cxm_m(cxm_name[cxm_stk[j]]); cxm_m(" -> "); j = j + 1; }
            cxm_m(cxm_name[v]);
            return cxm_fail();
        }
        if (cxm_color[v] == 0) { if (cxm_dfs(v)) return 1; }
        k = k + 1;
    }
    cxm_nstk = cxm_nstk - 1; cxm_color[u] = 2;
    cxm_topo[cxm_ntopo] = u; cxm_ntopo = cxm_ntopo + 1;
    return 0;
}
int cxm_samebytes(int a, int b) {
    int n; int k;
    n = cxm_syme[a] - cxm_symb[a];
    if (n != cxm_syme[b] - cxm_symb[b]) return 0;
    k = 0; while (k < n && cxm_ift[cxm_symb[a] + k] == cxm_ift[cxm_symb[b] + k]) k = k + 1;
    return k == n;
}
/* 1 when unit w uses module v importing only SYM under an alias (so SYM is renamed away and does not clash) */
int cxm_aliased(int w, int v, char *sym) {
    int k;
    k = 0;
    while (k < cxm_nuse[w]) {
        if (cxm_use[w][k] == v && cxm_usym[w][k][0] && cxm_eq(cxm_usym[w][k], sym) && cxm_ualias[w][k][0]) return 1;
        k = k + 1;
    }
    return 0;
}
/* 1 when unit w can see module v's exports (direct use or through its use closure) */
int cxm_sees(int w, int v) {
    return w == v || cxm_reach[w][v];
}
int cxm_collisions(void) {
    int a; int b; int w; int seen;
    a = 0;
    while (a < cxm_nsym) {
        b = a + 1;
        while (b < cxm_nsym) {
            if (cxm_symu[a] != cxm_symu[b] && cxm_eq(cxm_sym[a], cxm_sym[b])) {
                if (cxm_symk[a] == 0 && cxm_symk[b] == 0) {
                    w = 0; seen = 0;
                    while (w < cxm_n && !seen) {
                        if (cxm_sees(w, cxm_symu[a]) && cxm_sees(w, cxm_symu[b])) {
                            if (!cxm_aliased(w, cxm_symu[a], cxm_sym[a]) && !cxm_aliased(w, cxm_symu[b], cxm_sym[b])) seen = 1;
                        }
                        w = w + 1;
                    }
                    if (!seen) { b = b + 1; continue; }
                    cxm_start(0 - 1, 0); cxm_m(cxm_sym[a]); cxm_m(" exported by both ");
                    cxm_m(cxm_name[cxm_symu[a]]); cxm_m(" ("); cxm_m(cxm_arg[cxm_symu[a]]); cxm_m(":"); cxm_md(cxm_syml[a]); cxm_m(") and ");
                    cxm_m(cxm_name[cxm_symu[b]]); cxm_m(" ("); cxm_m(cxm_arg[cxm_symu[b]]); cxm_m(":"); cxm_md(cxm_syml[b]); cxm_m(")");
                    return cxm_fail();
                }
                if ((cxm_symk[a] == 0 && cxm_symk[b] == 2) || (cxm_symk[a] == 2 && cxm_symk[b] == 0)) {
                    int d; int x; d = cxm_symk[a] == 2 ? a : b; x = cxm_symk[a] == 2 ? b : a;
                    cxm_start(cxm_symu[d], cxm_syml[d]); cxm_m(cxm_sym[d]); cxm_m(" is exported by "); cxm_m(cxm_name[cxm_symu[x]]);
                    return cxm_fail();
                }
                if (cxm_symk[a] == 1 && cxm_symk[b] == 1) {
                    w = 0; seen = 0;
                    while (w < cxm_n && !seen) {
                        if ((w == cxm_symu[a] || cxm_reach[w][cxm_symu[a]]) && (w == cxm_symu[b] || cxm_reach[w][cxm_symu[b]])) seen = 1;
                        w = w + 1;
                    }
                    if (seen) {
                        cxm_start(0 - 1, 0); cxm_m("type "); cxm_m(cxm_sym[a]); cxm_m(" from "); cxm_m(cxm_name[cxm_symu[a]]); cxm_m(" and ");
                        cxm_m(cxm_name[cxm_symu[b]]); cxm_m(cxm_samebytes(a, b) ? " is defined twice" : " differ");
                        return cxm_fail();
                    }
                }
            }
            b = b + 1;
        }
        a = a + 1;
    }
    return 0;
}
/* the program's units: paths[0..n) as given (the command line), then every module their `use` closure reaches.
   0 with cxm_n units (cxm_arg[u] is the path to compile, the given string for a command-line unit), 1 refused. */
int cxm_plan(char **paths, int n) {
    int u; int k; int t; int v; int r; int direct;
    cxm_n = 0; cxm_on = 1; cxm_nsym = 0; cxm_nift = 0; cxm_ntopo = 0; cxm_nstk = 0; cxm_nren = 0;
    u = 0;
    while (u < n) {
        if (u >= CXM_MAXU) { cxm_start(0 - 1, 0); cxm_m("more than 64 units in one program"); cxm_on = 0; return cxm_fail(); }
        cxm_arg[u] = paths[u];
        if (cxm_norm(paths[u], cxm_path[u])) cxm_cpy(cxm_path[u], paths[u], CXM_PATHW);
        cxm_stemof(paths[u], cxm_stem[u]); cxm_cpy(cxm_name[u], cxm_stem[u], CXM_NAMEW);
        cxm_iscx[u] = cxm_iscxpath(paths[u]); cxm_lib[u] = 0; cxm_nuse[u] = 0;
        u = u + 1;
    }
    cxm_n = n;
    u = 0;
    while (u < cxm_n) {                              /* breadth-first: the list grows while it is walked */
        cxm_full[u][0] = 0; cxm_typ[u][0] = 0; cxm_ninc[u] = 0; cxm_color[u] = 0;
        k = 0; while (k < CXM_MAXU) { cxm_reach[u][k] = 0; k = k + 1; }
        if (cxm_iscx[u]) {
            r = cxm_load(cxm_arg[u], u);
            if (r > 0) { cxm_on = 0; return 1; }
            if (r == 0 && cxm_scan(u, 0)) { cxm_on = 0; return 1; }
        }
        u = u + 1;
    }
    u = 0;
    while (u < cxm_n) { if (cxm_color[u] == 0 && cxm_dfs(u)) { cxm_on = 0; return 1; } u = u + 1; }
    k = 1;
    while (k) {                                      /* reach = the transitive closure of use */
        k = 0; u = 0;
        while (u < cxm_n) {
            t = 0;
            while (t < cxm_nuse[u]) {
                v = cxm_use[u][t];
                if (!cxm_reach[u][v]) { cxm_reach[u][v] = 1; k = 1; }
                r = 0; while (r < cxm_n) { if (cxm_reach[v][r] && !cxm_reach[u][r]) { cxm_reach[u][r] = 1; k = 1; } r = r + 1; }
                t = t + 1;
            }
            u = u + 1;
        }
    }
    r = 0; u = 0; while (u < cxm_n) { if (cxm_lib[u]) r = 1; u = u + 1; }
    if (r == 0) return 0;                            /* no module: every list stays as it was */
    u = 0;
    while (u < cxm_n) {
        if (cxm_iscx[u]) {
            r = cxm_load(cxm_arg[u], u);
            if (r > 0) { cxm_on = 0; return 1; }
            if (r < 0) { u = u + 1; continue; }      /* a missing command-line source: the driver names it */
            cxm_srcsha(u);
            if (cxm_scan(u, 1) || cxm_project(u, cxm_lib[u])) { cxm_on = 0; return 1; }
        }
        u = u + 1;
    }
    if (cxm_collisions()) { cxm_on = 0; return 1; }
    u = 0;
    while (u < cxm_n) {
        if (cxm_lib[u]) {
            if (cxm_text(u, 1, 0, 0) || cxm_put(cxm_full[u])) { cxm_on = 0; return 1; }
            if (cxm_te[u] > cxm_tb[u] && (cxm_text(u, 0, 0, 0) || cxm_put(cxm_typ[u]))) { cxm_on = 0; return 1; }
        }
        u = u + 1;
    }
    u = 0;
    while (u < cxm_n) {                              /* each unit: its use closure, dependencies first */
        if (cxm_iscx[u]) {
            t = 0;
            while (t < cxm_ntopo) {
                v = cxm_topo[t];
                if (v != u && cxm_reach[u][v]) {
                    int uk; direct = 0; uk = 0;
                    k = 0; while (k < cxm_nuse[u]) { if (cxm_use[u][k] == v) { direct = 1; uk = k; } k = k + 1; }
                    if (direct) {
                        if (cxm_usym[u][uk][0] && cxm_ualias[u][uk][0]) {
                            if (cxm_text(v, 1, cxm_usym[u][uk], cxm_ualias[u][uk]) || cxm_put(cxm_renpool[cxm_nren])) { cxm_on = 0; return 1; }
                            cxm_cpy(cxm_ren[u][uk], cxm_renpool[cxm_nren], CXM_PATHW);
                            cxm_inc[u][cxm_ninc[u]] = 0 - (cxm_nren + 1); cxm_nren = cxm_nren + 1; cxm_ninc[u] = cxm_ninc[u] + 1;
                        } else { cxm_inc[u][cxm_ninc[u]] = 2 * v + 1; cxm_ninc[u] = cxm_ninc[u] + 1; }
                    }
                    else if (cxm_typ[v][0]) { cxm_inc[u][cxm_ninc[u]] = 2 * v; cxm_ninc[u] = cxm_ninc[u] + 1; }
                }
                t = t + 1;
            }
        }
        u = u + 1;
    }
    return 0;
}
/* the k-th interface file unit u is compiled with (after cx.h and the -include list) */
char *cxm_incpath(int u, int k) { int x; x = cxm_inc[u][k]; if (x < 0) return cxm_renpool[0 - x - 1]; return (x & 1) ? cxm_full[x / 2] : cxm_typ[x / 2]; }
int cxm_nincof(char *path) { int u; u = cxm_find(path); return u >= 0 ? cxm_ninc[u] : 0; }
/* -cx-plan: index, path, forced includes (cx.h for a .cx unit, the -include list, the interfaces), one unit a line */
int cxm_planprint(char **opt, int nopt) {
    int u; int k;
    u = 0;
    while (u < cxm_n) {
        cxm_nout = 0;
        cxm_nmsg = 0; cxm_md(u); cxm_on2(cxm_msg, cxm_nmsg);
        cxm_o("\t"); cxm_o(cxm_arg[u]);
        if (cxm_iscx[u]) cxm_o("\tcx.h");
        k = 0; while (k < nopt) { cxm_o("\t"); cxm_o(opt[k]); k = k + 1; }
        k = 0; while (k < cxm_ninc[u]) { cxm_o("\t"); cxm_o(cxm_incpath(u, k)); k = k + 1; }
        cxm_o("\n");
        cxm_say(1, cxm_out, cxm_nout);
        u = u + 1;
    }
    return 0;
}
/* -cx-interface M.cx: the CXIF of one module, printed */
int cxm_interface(char *path) {
    int r;
    cxm_on = 0; cxm_n = 1; cxm_nsym = 0; cxm_nift = 0;
    cxm_arg[0] = path;
    if (cxm_norm(path, cxm_path[0])) cxm_cpy(cxm_path[0], path, CXM_PATHW);
    cxm_stemof(path, cxm_stem[0]); cxm_cpy(cxm_name[0], cxm_stem[0], CXM_NAMEW);
    r = cxm_load(path, 0);
    if (r < 0) { cxm_start(0 - 1, 0); cxm_m("cannot read "); cxm_m(path); return cxm_fail(); }
    if (r > 0) return 1;
    cxm_srcsha(0);
    if (cxm_scan(0, 1) || cxm_project(0, 1) || cxm_text(0, 1, 0, 0)) return 1;
    cxm_say(1, cxm_out, cxm_nout);
    return 0;
}
