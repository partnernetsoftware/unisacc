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

#define CXM_MAXU 128
#define CXM_MAXUSE 48
#define CXM_MAXDECL 48
#define CXM_MAXEXP 128
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
int cxm_use[CXM_MAXU][CXM_MAXDECL]; int cxm_nuse[CXM_MAXU];
/* per use, the imported symbol and its alias (both empty = import the whole module) */
char cxm_usym[CXM_MAXU][CXM_MAXDECL][CXM_SYMW]; char cxm_ualias[CXM_MAXU][CXM_MAXDECL][CXM_SYMW];
char cxm_reach[CXM_MAXU][CXM_MAXU];
int cxm_topo[CXM_MAXU]; int cxm_ntopo; int cxm_color[CXM_MAXU]; int cxm_stk[CXM_MAXU]; int cxm_nstk;
char cxm_ift[CXM_IFMAX + 1]; int cxm_nift;
int cxm_tb[CXM_MAXU]; int cxm_te[CXM_MAXU]; int cxm_fb[CXM_MAXU]; int cxm_fe[CXM_MAXU];
char cxm_sha[CXM_MAXU][17];
char cxm_full[CXM_MAXU][CXM_PATHW]; char cxm_typ[CXM_MAXU][CXM_PATHW];
int cxm_inc[CXM_MAXU][CXM_MAXU]; int cxm_ninc[CXM_MAXU];
/* renamed-CXIF path pool; cxm_inc[u][k] < 0 means cxm_renpool[-(x)-1] */
char cxm_renpool[CXM_MAXU * CXM_MAXDECL][CXM_PATHW]; int cxm_nren;
/* body-symbol rename: cxm_bodysrc[v] is a rewritten source of module v ("#define sym alias" head),
   empty = compile the original source.  One rename per module (the common case). */
char cxm_bodysrc[CXM_MAXU][CXM_PATHW];
/* `#pragma unisa export NAME[,NAME...]` of the unit being scanned (reset by every cxm_scan): the public list
   of its own non-static functions.  cxm_modseen: the unit's `#pragma unisa module` line was already read. */
char cxm_exp[CXM_MAXEXP][CXM_SYMW]; int cxm_nexp; int cxm_modseen;
/* `#pragma unisa owns NAME [NAME...]` of the unit being scanned (mode 0, reset by every cxm_scan): functions that
   return an owned block (prd_cx.md 11.7); cxm_ownl is the pragma line.  A unit without owns never reads these. */
char cxm_own[CXM_MAXEXP][CXM_SYMW]; int cxm_ownl[CXM_MAXEXP]; int cxm_nown;
/* `#pragma unisa safe` (cxm_safe: its line) and `#pragma unisa unsafe NAME...` of the scanned unit (plans/v0.0.77.md
   P5); cxf_rel: one-line `#define NAME(x) free(x)` releaser macros seen by the scan (blanked lines keep no text) */
int cxm_safe; char cxm_uns[CXM_MAXEXP][CXM_SYMW]; int cxm_unsl[CXM_MAXEXP]; int cxm_nuns;
#define CXF_R 32
static char cxf_rel[CXF_R][CXM_SYMW]; static int cxf_nrel;
/* exported and defined names: kind 0 a module's function, 1 a module's type name (typedef, "struct T", enum
   constant), 2 a non-static function defined by a unit that is not a module, 3 a module's non-static function
   that is not public (no prototype in the interface; only names the owner of a refused call) */
char cxm_sym[CXM_SYMS][CXM_SYMW]; int cxm_symu[CXM_SYMS]; int cxm_syml[CXM_SYMS]; int cxm_symk[CXM_SYMS];
int cxm_symb[CXM_SYMS]; int cxm_syme[CXM_SYMS]; int cxm_nsym;
/* the file being scanned: its items and macros */
int cxm_ib[CXM_ITEMS]; int cxm_ie[CXM_ITEMS]; int cxm_ibody[CXM_ITEMS]; int cxm_iline[CXM_ITEMS]; int cxm_icond[CXM_ITEMS];
int cxm_ikind[CXM_ITEMS]; int cxm_nitem;
char cxm_mac[CXM_MACROS][CXM_SYMW]; int cxm_nmac;
/* -funit: cxm_expand keeps only the command-line units. Their direct `use`
   still yields a CXIF declaration; the used modules' bodies stay out of the
   object, so their functions are undefined references for the link. */
int cxm_funit; int cxm_rootn;
/* function declarators this unit itself writes (a prototype or a definition). A call is visible when the
   name is here or exported by a module this unit directly uses. */
#define CXM_LOCS 4096
char cxm_locn[CXM_LOCS][CXM_SYMW]; int cxm_locu[CXM_LOCS]; int cxm_nloc;
int cxm_badcalls;
int cxm_tokb; int cxm_tokl;

int cxm_len(char *s) { int n; n = 0; while (s[n]) n = n + 1; return n; }
int cxm_eq(char *a, char *b) { int k; k = 0; while (a[k] && a[k] == b[k]) k = k + 1; return a[k] == b[k]; }
int cxm_eqsn(char *a, char *b, int n) { int k = 0; while (a[k]) k++; if (k != n) return 0; for (k = 0; k < n; k++) if (a[k] != b[k]) return 0; return 1; }
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
/* decimal v into the out buffer (cxm_md writes the message buffer, not generated text) */
int cxm_od(int v) { char d[16]; int n; n = 0; if (v == 0) { d[0] = '0'; n = 1; } while (v > 0) { d[n] = '0' + v % 10; v = v / 10; n = n + 1; } while (n > 0) { n = n - 1; cxm_on2(d + n, 1); } return 0; }
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
int cxm_find(char *path) { int u; u = 0; if (cxm_on == 0) return 0 - 1; while (u < cxm_n) { if (cxm_eq(cxm_arg[u], path) || (cxm_bodysrc[u][0] && cxm_eq(cxm_bodysrc[u], path))) return u; u = u + 1; } return 0 - 1; }
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
    /* Match E2 P0S: only a complete UTF-8 BOM at physical source start. */
    if (n >= 3 && (s[0] & 255) == 239 && (s[1] & 255) == 187 && (s[2] & 255) == 191) { s[0] = ' '; s[1] = ' '; s[2] = ' '; i = 3; }
    if (i + 1 < n && s[i] == '#' && s[i + 1] == '!') { while (i < n && s[i] != '\n') { s[i] = ' '; i = i + 1; } }
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
/* Same file when the two normalised texts match. sy_path has no device number
   and this unit must not pull sys.fs in (-funit would copy it and the link
   would report sy_fstat twice), so both sides are compared as text: every
   path in cxm_path is already lexically normalised by cxm_norm, and the
   caller hands absolute paths. */
int cxm_samefile(char *a, char *b) {
#ifdef CXM_HOST_REALPATH
    char ra[512], rb[512];
    extern char *realpath(const char *, char *);
#endif
    if (cxm_eq(a, b)) return 1;
#ifdef CXM_HOST_REALPATH
    if (realpath(a, ra) && realpath(b, rb) && cxm_eq(ra, rb)) return 1;
#endif
    return 0;
}
/* Two lexical entries of one physical module resolve their `use` names
   beside their own directories.  Equal resolved targets may merge; any
   differing target keeps the ambiguity and is refused by the caller. */
int cxm_deps_same(char *a, char *b) {
    char da[CXM_PATHW], db[CXM_PATHW], cand[CXM_PATHW], norm[CXM_PATHW];
    int n, k, la, lb, p, q;
    n = cxm_rd(a, cxm_cmp, CXM_IFMAX);
    if (n < 0) return 0;
    la = cxm_len(a); while (la > 0 && a[la - 1] != '/') la = la - 1;
    lb = cxm_len(b); while (lb > 0 && b[lb - 1] != '/') lb = lb - 1;
    if (la >= CXM_PATHW || lb >= CXM_PATHW) return 0;
    k = 0; while (k < la) { da[k] = a[k]; k = k + 1; } da[la] = 0;
    k = 0; while (k < lb) { db[k] = b[k]; k = k + 1; } db[lb] = 0;
    k = 0;
    while (k < n) {
        int start = k, end, at;
        while (k < n && cxm_cmp[k] != '\n') k = k + 1;
        end = k; if (k < n) k = k + 1;
        at = start; while (at < end && (cxm_cmp[at] == ' ' || cxm_cmp[at] == '\t')) at = at + 1;
        if (at >= end || cxm_cmp[at] != '#') continue;
        at = at + 1; while (at < end && (cxm_cmp[at] == ' ' || cxm_cmp[at] == '\t')) at = at + 1;
        if (at + 6 > end || cxm_cmp[at] != 'p' || cxm_cmp[at + 1] != 'r' || cxm_cmp[at + 2] != 'a' ||
            cxm_cmp[at + 3] != 'g' || cxm_cmp[at + 4] != 'm' || cxm_cmp[at + 5] != 'a') continue;
        at = at + 6; while (at < end && (cxm_cmp[at] == ' ' || cxm_cmp[at] == '\t')) at = at + 1;
        if (at + 5 > end || cxm_cmp[at] != 'u' || cxm_cmp[at + 1] != 'n' || cxm_cmp[at + 2] != 'i' ||
            cxm_cmp[at + 3] != 's' || cxm_cmp[at + 4] != 'a') continue;
        at = at + 5; while (at < end && (cxm_cmp[at] == ' ' || cxm_cmp[at] == '\t')) at = at + 1;
        if (at + 3 > end || cxm_cmp[at] != 'u' || cxm_cmp[at + 1] != 's' || cxm_cmp[at + 2] != 'e') continue;
        at = at + 3; while (at < end && (cxm_cmp[at] == ' ' || cxm_cmp[at] == '\t')) at = at + 1;
        { int mb = at; while (at < end && (cxm_cmp[at] == '.' || cxm_idch(cxm_cmp[at] & 255))) at = at + 1;
          if (at == mb) return 0;
          p = 0; q = 0; while (p < la) cand[q++] = da[p++];
          p = mb; while (p < at) { cand[q++] = cxm_cmp[p] == '.' ? '/' : cxm_cmp[p]; p++; }
          cand[q++] = '.'; cand[q++] = 'c'; cand[q++] = 'x'; cand[q] = 0;
          if (cxm_norm(cand, norm)) return 0;
          p = 0; q = 0; while (p < lb) cand[q++] = db[p++];
          p = mb; while (p < at) { cand[q++] = cxm_cmp[p] == '.' ? '/' : cxm_cmp[p]; p++; }
          cand[q++] = '.'; cand[q++] = 'c'; cand[q++] = 'x'; cand[q] = 0;
          if (!cxm_samefile(norm, cand)) return 0;
        }
    }
    return 1;
}
/* Package root (spec CX-U-7).  A use NAME with no file beside the using unit is looked up through the
   nearest ancestor directory of that unit holding cxroot.tsv: "CXROOT1\n", then rows
   "R<TAB>PREFIX<TAB>DIR\n" ('#' lines and empty lines are comments).  PREFIX is a module name; DIR is a
   directory relative to the cxroot.tsv directory: not empty, no leading or trailing '/', no empty, "."
   or ".." segment, no tab.  The longest PREFIX equal to NAME's leading dot segments maps NAME to
   DIR/<the remaining segments, '.' as '/'>.cx.  The nearest cxroot.tsv decides alone: a NAME none of its
   rows covers is not looked up further up.  A malformed file, a malformed row and a PREFIX given twice
   are refused by name; rows are never skipped.
   cxm_rootrow and cxm_rootfind return 0 with out set, 1 when no root or no row covers NAME, and -1 after
   the refusal was reported. */
int cxm_rootbad(int u, int line, char *file, char *why) {
    cxm_start(u, line); cxm_m(file); cxm_m(": "); cxm_m(why); cxm_fail(); return 0 - 1;
}
int cxm_rootrow(int u, int line, char *w, int n, char *dir, int dl, int len, char *file, char *out) {
    char cand[CXM_PATHW]; char *s; int i; int ls; int le; int pb; int pe; int db; int k; int p;
    int bl; int bdb; int bde;
    s = cxm_cmp; bl = 0 - 1; bdb = 0; bde = 0;
    if (len < 8 || !cxm_eqn(s, 8, "CXROOT1\n")) return cxm_rootbad(u, line, file, "first line is not CXROOT1");
    i = 8;
    while (i < len) {
        ls = i; while (i < len && s[i] != '\n') i = i + 1;
        le = i; if (i < len) i = i + 1; else return cxm_rootbad(u, line, file, "last row has no newline");
        if (le == ls || s[ls] == '#') continue;
        if (le - ls < 5 || s[ls] != 'R' || s[ls + 1] != '\t') return cxm_rootbad(u, line, file, "row is not R<TAB>PREFIX<TAB>DIR");
        pb = ls + 2; pe = pb; while (pe < le && s[pe] != '\t') pe = pe + 1;
        if (pe >= le) return cxm_rootbad(u, line, file, "row is not R<TAB>PREFIX<TAB>DIR");
        if (!cxm_goodname(s + pb, pe - pb)) return cxm_rootbad(u, line, file, "bad PREFIX");
        db = pe + 1;
        if (db >= le || s[db] == '/' || s[le - 1] == '/') return cxm_rootbad(u, line, file, "bad DIR");
        k = db;
        while (k < le) {                                   /* every DIR segment: not empty, not "." or "..", no tab */
            p = k; while (p < le && s[p] != '/') { if (s[p] == '\t' || (s[p] & 255) < 32) return cxm_rootbad(u, line, file, "bad DIR"); p = p + 1; }
            if (p == k || (p - k == 1 && s[k] == '.') || (p - k == 2 && s[k] == '.' && s[k + 1] == '.')) return cxm_rootbad(u, line, file, "bad DIR");
            k = p + 1;
        }
        if (pe - pb < n && w[pe - pb] == '.') {
            k = 0; while (k < pe - pb && w[k] == s[pb + k]) k = k + 1;
            if (k == pe - pb) {
                if (k == bl) return cxm_rootbad(u, line, file, "PREFIX given twice");
                if (k > bl) { bl = k; bdb = db; bde = le; }
            }
        }
    }
    if (bl < 0) return 1;
    if (dl + (bde - bdb) + 1 + (n - bl) + 4 >= CXM_PATHW) return cxm_rootbad(u, line, file, "module path too long");
    p = 0; k = 0; while (k < dl) { cand[p] = dir[k]; p = p + 1; k = k + 1; }
    k = bdb; while (k < bde) { cand[p] = s[k]; p = p + 1; k = k + 1; }
    k = bl; while (k < n) { cand[p] = w[k] == '.' ? '/' : w[k]; p = p + 1; k = k + 1; }
    cand[p] = '.'; cand[p + 1] = 'c'; cand[p + 2] = 'x'; cand[p + 3] = 0;
    if (cxm_norm(cand, out)) return cxm_rootbad(u, line, file, "module path too long");
    return 0;
}
int cxm_rootfind(int u, int line, char *w, int n, char *out) {
    char dir[CXM_PATHW]; char file[CXM_PATHW]; int dl; int k; int r; int depth;
    dl = cxm_len(cxm_path[u]);
    while (dl > 0 && cxm_path[u][dl - 1] != '/') dl = dl - 1;
    k = 0; while (k < dl) { dir[k] = cxm_path[u][k]; k = k + 1; }
    dir[dl] = 0; depth = 0;
    while (depth < 64) {                                   /* a relative unit path walks up with "../" */
        if (dl + 11 >= CXM_PATHW) return 1;
        k = 0; while (k < dl) { file[k] = dir[k]; k = k + 1; }
        cxm_cpy(file + dl, "cxroot.tsv", CXM_PATHW - dl);
        r = cxm_rd(file, cxm_cmp, CXM_IFMAX);
        if (r >= 0) return cxm_rootrow(u, line, w, n, dir, dl, r, file, out);
        if (r == 0 - 2) return cxm_rootbad(u, line, file, "too large");
        if (r != 0 - 1) return cxm_rootbad(u, line, file, "read error");
        if (dl == 1 && dir[0] == '/') return 1;            /* "/" searched */
        if (dir[0] != '/' && (dl == 0 || (dl >= 3 && dir[dl - 3] == '.' && dir[dl - 2] == '.' && (dl == 3 || dir[dl - 4] == '/')))) {
            dir[dl] = '.'; dir[dl + 1] = '.'; dir[dl + 2] = '/'; dl = dl + 3;
        } else {
            dl = dl - 1; while (dl > 0 && dir[dl - 1] != '/') dl = dl - 1;
        }
        dir[dl] = 0; depth = depth + 1;
    }
    return 1;
}
/* `#pragma unisa use NAME` in unit u at line: resolve beside the using file and add the module */
/* `#pragma unisa use NAME` (optionally `NAME::sym as alias`) in unit u at line */
int cxm_adduse(int u, char *w, int n, int line, char *sym, int sn, char *alias, int an) {
    char cand[CXM_PATHW]; char norm[CXM_PATHW]; int dl; int k; int v; int p;
    if (!cxm_goodname(w, n)) { cxm_start(u, line); cxm_m("bad module name \""); cxm_mn(w, n); cxm_m("\""); return cxm_fail(); }
    if (cxm_eqn(w, n, "std") || cxm_eqn(w, n, "std.cx")) return 0;        /* the prelude: always there */
    if (n > 4 && w[0] == 's' && w[1] == 't' && w[2] == 'd' && w[3] == '.') {
        cxm_start(u, line); cxm_m("unknown std module "); cxm_mn(w, n); return cxm_fail();
    }
    v = 0; while (v < cxm_n && !(cxm_name[v][0] && cxm_eqn(w, n, cxm_name[v]))) v = v + 1; /* cxm_name[v][0] */
    if (v == cxm_n) { /* cxm_name[v][0] */
    dl = cxm_len(cxm_path[u]);
    while (dl > 0 && cxm_path[u][dl - 1] != '/') dl = dl - 1;
    if (dl + n + 4 >= CXM_PATHW) { cxm_start(u, line); cxm_m("module path too long"); return cxm_fail(); }
    p = 0; k = 0;
    while (k < dl) { cand[p] = cxm_path[u][k]; p = p + 1; k = k + 1; }
    k = 0; while (k < n) { cand[p] = w[k] == '.' ? '/' : w[k]; p = p + 1; k = k + 1; }
    cand[p] = '.'; cand[p + 1] = 'c'; cand[p + 2] = 'x'; cand[p + 3] = 0;
    if (cxm_norm(cand, norm)) { cxm_start(u, line); cxm_m("module path too long"); return cxm_fail(); }
    v = 0; while (v < cxm_n && !cxm_samefile(cxm_path[v], norm)) v = v + 1;
    if (v == cxm_n && cxm_rd(norm, cxm_cmp, 1) == 0 - 1) {     /* nothing beside the unit: the package root */
        char rnorm[CXM_PATHW]; int rr;
        rr = cxm_rootfind(u, line, w, n, rnorm);
        if (rr < 0) return 1;
        if (rr == 0) { cxm_cpy(norm, rnorm, CXM_PATHW); v = 0; while (v < cxm_n && !cxm_samefile(cxm_path[v], norm)) v = v + 1; }
    }
    if (v < cxm_n && !cxm_eq(cxm_path[v], norm) && !cxm_deps_same(cxm_path[v], norm)) {
        cxm_start(u, line); cxm_m("module "); cxm_mn(w, n); cxm_m(" has two lexical entries: ");
        cxm_m(cxm_path[v]); cxm_m(" and "); cxm_m(norm); cxm_m(" with use dependencies"); return cxm_fail();
    }
    if (v == cxm_n) {
        if (cxm_rd(norm, cxm_cmp, 1) == 0 - 1) {
            cxm_start(u, line); cxm_m("module "); cxm_mn(w, n); cxm_m(": no file "); cxm_m(norm); return cxm_fail();
        }
        if (cxm_n >= CXM_MAXU) { cxm_start(u, line); cxm_m("more than "); cxm_md(CXM_MAXU); cxm_m(" units in one program"); return cxm_fail(); }
        cxm_cpy(cxm_path[v], norm, CXM_PATHW); cxm_arg[v] = cxm_keep(norm);
        if (cxm_arg[v] == 0) { cxm_start(u, line); cxm_m("too many module paths in one process"); return cxm_fail(); }
        cxm_cpyn(cxm_name[v], w, n, CXM_NAMEW); cxm_stemof(norm, cxm_stem[v]);
        cxm_iscx[v] = 1; cxm_lib[v] = 0; cxm_nuse[v] = 0;
        cxm_n = cxm_n + 1;
    }
    } /* cxm_name[v][0] reuse */
    if (!cxm_lib[v]) cxm_cpyn(cxm_name[v], w, n, CXM_NAMEW);   /* a command-line unit used by name takes that name */
    cxm_lib[v] = 1;
    k = 0;
    while (k < cxm_nuse[u]) {
        if (cxm_use[u][k] == v && cxm_eqsn(cxm_usym[u][k], sym, sn) &&
            cxm_eqsn(cxm_ualias[u][k], alias, an)) return 0;
        k = k + 1;
    }
    { int distinct = 0, z = 0;
      while (z < cxm_nuse[u]) { int q = 0, seen = 0; while (q < z) { if (cxm_use[u][q] == cxm_use[u][z]) { seen = 1; break; } q++; } if (!seen) distinct++; z++; }
      if (v == cxm_n || distinct < CXM_MAXUSE) { }
      else if (distinct >= CXM_MAXUSE) { cxm_start(u, line); cxm_m("more than 48 uses in one unit"); return cxm_fail(); }
    }
    if (cxm_nuse[u] >= CXM_MAXDECL) { cxm_start(u, line); cxm_m("too many use declarations in one unit"); return cxm_fail(); }
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
/* the operand of `#pragma unisa export` at s[i..e): NAME[,NAME...], identifiers only, into cxm_exp */
int cxm_exportline(int u, int line, int i, int e) {
    char *s; int b; int k; int q;
    s = cxm_src;
    while (i < e && (s[i] & 255) <= 32) i = i + 1;
    if (i >= e) { cxm_start(u, line); cxm_m("unisa export takes a name"); return cxm_fail(); }
    if (!cxm_modseen) { cxm_start(u, line); cxm_m("export outside module"); return cxm_fail(); }
    while (1) {
        while (i < e && (s[i] & 255) <= 32) i = i + 1;
        b = i; while (i < e && cxm_idch(s[i] & 255)) i = i + 1;
        q = i; while (q < e && (s[q] & 255) <= 32) q = q + 1;
        if (i == b || i - b >= CXM_SYMW || (q < e && s[q] != ',')) {
            if (i > b && i - b < CXM_SYMW) b = q;    /* a good name followed by something else: name that */
            q = b; while (q < e && (s[q] & 255) > 32) q = q + 1;
            cxm_start(u, line); cxm_m("bad export name \""); cxm_mn(s + b, q - b); cxm_m("\""); return cxm_fail();
        }
        k = 0;
        while (k < cxm_nexp) {
            if (cxm_eqn(s + b, i - b, cxm_exp[k])) {
                cxm_start(u, line); cxm_m("module "); cxm_m(cxm_stem[u]); cxm_m(" exports "); cxm_mn(s + b, i - b); cxm_m(" twice"); return cxm_fail();
            }
            k = k + 1;
        }
        if (cxm_nexp >= CXM_MAXEXP) {
            cxm_start(u, line); cxm_m("module "); cxm_m(cxm_stem[u]); cxm_m(" has more than "); cxm_md(CXM_MAXEXP); cxm_m(" exports"); return cxm_fail();
        }
        cxm_cpyn(cxm_exp[cxm_nexp], s + b, i - b, CXM_SYMW); cxm_nexp = cxm_nexp + 1;
        if (q >= e) return 0;
        i = q + 1;
    }
}
int cxm_exported(char *nm, int nl) { int k; k = 0; while (k < cxm_nexp) { if (cxm_eqn(nm, nl, cxm_exp[k])) return 1; k = k + 1; } return 0; }
/* ---- `#pragma unisa safe` (plans/v0.0.77.md 2.3, P5): use after free (E2) and double free (E2b) by a structured,
   intraprocedural walk of each function body of a safe unit.  The rules are those of tools/own-flow.cx (the oracle,
   tests/cases/own-flow/expected.tsv): subjects are local names (parameters included) handed alone to free, mm_free
   or a one-line releaser macro; a name is freed after its release until it is assigned as a whole, declared again or
   its address is taken; any later occurrence is E2, a second release E2b; branches, case groups, loop exits and goto
   labels join their predecessors (freed on any is freed); a loop body is read twice when the back edge adds a
   release; return, break, continue, goto and no-return calls end a path.  `#pragma unisa unsafe NAME` exempts a
   function.  Limits (silent misses): aliases, fields, globals, function pointers, cross-function transfer; the
   `own-flow: releases` comment of the prototype has no product form (comments are blanked before the scan).
   Only a unit that declares safe reaches cxf_unit.
   Capacity (plans/v0.0.78.md items 1-3): a limit is never a silent pass, each one refuses the unit by name.
     65536 tokens, 4096 nested brackets, unbalanced brackets (also across #if branches, whose directive lines
       are blank): "unisa safe: too large or unbalanced for the flow check in unit"
     64 released names per function (CXF_V): "more than 64 released names for the flow check in FUNC"
     32 goto labels per function (CXF_L): "more than 32 goto labels for the flow check in FUNC"
     64 nested loops and switches (CXF_D): "loops or switches nested deeper than 64 for the flow check in FUNC"
     goto: the body is walked again until the label states are stable, at most 64 walks (CXF_GP):
       "goto labels not stable after 64 walks for the flow check in FUNC"
     256 live shadowing declarations (CXF_SH): "more than 256 shadowing declarations for the flow check in FUNC"
     512 flow states (CXF_S): "FUNC is too deep for the flow check"
   A declaration in an inner block gives the name its own slot: the outer state is saved and comes back when the
   block ends.  The operand of sizeof (with or without parentheses) is not a use. ---- */
#define CXF_T 65536
#define CXF_V 64
#define CXF_D 64
#define CXF_L 32
#define CXF_S 512
#define CXF_NR 256
#define CXF_GP 64
#define CXF_SH 256
static int cxf_k[CXF_T]; static int cxf_o[CXF_T]; static int cxf_n[CXF_T]; static int cxf_ln[CXF_T]; static int cxf_mt[CXF_T];
static int cxf_nt; static int cxf_stk[4096];
static int cxf_nro[CXF_NR]; static int cxf_nrn[CXF_NR]; static int cxf_nnr;
static int cxf_vo[CXF_V]; static int cxf_vl[CXF_V]; static int cxf_vd[CXF_V]; static int cxf_nv;
/* flow states: slot 0 is the current state, 1..CXF_L the goto labels, the rest a stack for joins */
static int cxf_st[CXF_S][CXF_V]; static int cxf_fl[CXF_S][CXF_V]; static int cxf_dd[CXF_S]; static int cxf_ns; static int cxf_over;
static int cxf_frk[CXF_D]; static int cxf_frdef[CXF_D]; static int cxf_frb[CXF_D]; static int cxf_frc[CXF_D]; static int cxf_fre[CXF_D]; static int cxf_nfr;
static int cxf_lbo[CXF_L]; static int cxf_lbn[CXF_L]; static int cxf_nlb;
static int cxf_depth; static int cxf_hit; static int cxf_hv; static int cxf_hl; static int cxf_fo; static int cxf_fn;
/* shadowing: the outer state of a name declared again in a deeper block, restored when that block ends */
static int cxf_shv[CXF_SH]; static int cxf_shs[CXF_SH]; static int cxf_shf[CXF_SH]; static int cxf_shd[CXF_SH]; static int cxf_nsh;
/* label states before a goto walk, to see whether another walk can change them */
static int cxf_gs[CXF_L + 1][CXF_V]; static int cxf_gd[CXF_L + 1];
/* the first capacity limit met (1 states, 2 names, 3 labels, 4 nesting, 5 goto walks, 6 shadowing) */
static int cxf_ov(int r) { if (!cxf_over) cxf_over = r; return 0; }
static int cxf_sk(char *s, int j, int e) { while (j < e && (s[j] == ' ' || s[j] == '\t')) j = j + 1; return j; }
static int cxf_wd(char *s, int j, int e) { while (j < e && cxm_idch(s[j] & 255)) j = j + 1; return j; }
static int cxf_isrel(char *p, int n) {
    int r;
    if (cxm_eqn(p, n, "free") || cxm_eqn(p, n, "mm_free")) return 1;
    r = 0; while (r < cxf_nrel) { if (cxm_eqn(p, n, cxf_rel[r])) return 1; r = r + 1; }
    return 0;
}
/* `#define NAME(x) R(x)` with R a releaser, an optional `;`; s[b..e) follows the word define */
static int cxf_define(char *s, int b, int e) {
    int j; int a; int nb; int pa; int pb; int ra; int rb; int k;
    j = cxf_sk(s, b, e); a = j; j = cxf_wd(s, j, e); nb = j;
    if (nb == a || j >= e || s[j] != '(') return 0;
    j = cxf_sk(s, j + 1, e); pa = j; j = cxf_wd(s, j, e); pb = j; j = cxf_sk(s, j, e);
    if (pb == pa || j >= e || s[j] != ')') return 0;
    j = cxf_sk(s, j + 1, e); ra = j; j = cxf_wd(s, j, e); rb = j;
    if (!cxf_isrel(s + ra, rb - ra)) return 0;
    j = cxf_sk(s, j, e);
    if (j >= e || s[j] != '(') return 0;
    j = cxf_sk(s, j + 1, e);
    if (j + (pb - pa) > e) return 0;
    k = 0; while (k < pb - pa) { if (s[j + k] != s[pa + k]) return 0; k = k + 1; }
    j = cxf_sk(s, j + (pb - pa), e);
    if (j >= e || s[j] != ')') return 0;
    j = cxf_sk(s, j + 1, e);
    if (j < e && s[j] == ';') j = cxf_sk(s, j + 1, e);
    if (j < e && s[j] != '\n' && s[j] != '\r') return 0;
    if (nb - a < CXM_SYMW && cxf_nrel < CXF_R) { cxm_cpyn(cxf_rel[cxf_nrel], s + a, nb - a, CXM_SYMW); cxf_nrel = cxf_nrel + 1; }
    return 0;
}
static int cxf_tis(int i, char *w) { return i >= 0 && i < cxf_nt && cxm_eqn(cxm_src + cxf_o[i], cxf_n[i], w); }
static int cxf_tp(int i, int c) { return i >= 0 && i < cxf_nt && cxf_k[i] == 2 && cxm_src[cxf_o[i]] == c; }
static int cxf_tid(int i) { return i >= 0 && i < cxf_nt && cxf_k[i] == 1; }
static int cxf_same(int i, int o, int n) {
    int k;
    if (cxf_n[i] != n) return 0;
    k = 0; while (k < n) { if (cxm_src[cxf_o[i] + k] != cxm_src[o + k]) return 0; k = k + 1; }
    return 1;
}
/* tokens of the cleaned unit (comments and directive lines are blanks already); 1 bracket mismatch or too many */
static int cxf_lex(void) {
    char *s; int n; int i; int line; int c; int q; int sp; int k; int o;
    s = cxm_src; n = cxm_nsrc; i = 0; line = 1; cxf_nt = 0;
    while (i < n) {
        c = s[i] & 255;
        if (c == '\n') { line = line + 1; i = i + 1; continue; }
        if (c <= 32) { i = i + 1; continue; }
        if (cxf_nt >= CXF_T) return 1;
        cxf_o[cxf_nt] = i; cxf_ln[cxf_nt] = line; cxf_mt[cxf_nt] = 0 - 1;
        if (c == '"' || c == '\'') {
            q = c; i = i + 1;
            while (i < n && s[i] != q && s[i] != '\n') { if (s[i] == '\\') i = i + 1; i = i + 1; }
            i = i + 1; cxf_k[cxf_nt] = 3;
        } else if (cxm_idch(c) && !(c >= '0' && c <= '9')) {
            while (i < n && cxm_idch(s[i] & 255)) i = i + 1;
            cxf_k[cxf_nt] = 1;
        } else if (c >= '0' && c <= '9') {
            while (i < n && (cxm_idch(s[i] & 255) || s[i] == '.')) i = i + 1;
            cxf_k[cxf_nt] = 3;
        } else { i = i + 1; cxf_k[cxf_nt] = 2; }
        cxf_n[cxf_nt] = i - cxf_o[cxf_nt];
        cxf_nt = cxf_nt + 1;
    }
    sp = 0; k = 0;
    while (k < cxf_nt) {
        if (cxf_k[k] == 2) {
            c = s[cxf_o[k]];
            if (c == '(' || c == '[' || c == '{') { if (sp >= 4096) return 1; cxf_stk[sp] = k; sp = sp + 1; }
            else if (c == ')' || c == ']' || c == '}') { if (sp <= 0) return 1; sp = sp - 1; o = cxf_stk[sp]; cxf_mt[o] = k; cxf_mt[k] = o; }
        }
        k = k + 1;
    }
    return sp != 0;
}
static int cxf_releaser(int i) { return cxf_tid(i) && cxf_isrel(cxm_src + cxf_o[i], cxf_n[i]); }
/* releaser call at I: the token of its single plain-name argument (parentheses and one cast stripped), or -1 */
static int cxf_relarg(int i) {
    int a; int e; int c; int j; int typ;
    if (!cxf_releaser(i) || !cxf_tp(i + 1, '(') || cxf_mt[i + 1] < 0 || cxf_tp(i - 1, '.') || cxf_tp(i - 1, '>')) return 0 - 1;
    a = i + 2; e = cxf_mt[i + 1] - 1;
    while (a < e && cxf_tp(a, '(') && cxf_mt[a] == e) { a = a + 1; e = e - 1; }
    if (a < e && cxf_tp(a, '(') && cxf_mt[a] > a && cxf_mt[a] < e && cxf_tid(a + 1)) {
        c = cxf_mt[a]; typ = 1; j = a + 1;
        while (j < c) { if (!(cxf_tid(j) || cxf_tp(j, '*'))) typ = 0; j = j + 1; }
        if (typ) a = c + 1;
        while (a < e && cxf_tp(a, '(') && cxf_mt[a] == e) { a = a + 1; e = e - 1; }
    }
    if (a == e && cxf_tid(a)) return a;
    return 0 - 1;
}
static int cxf_has(int i, char *w) {
    int n; int j; int k;
    n = 0; while (w[n]) n = n + 1;
    if (!cxf_tid(i)) return 0;
    j = 0;
    while (j + n <= cxf_n[i]) { k = 0; while (k < n && cxm_src[cxf_o[i] + j + k] == w[k]) k = k + 1; if (k == n) return 1; j = j + 1; }
    return 0;
}
static int cxf_locnr(int i) { int r; r = 0; while (r < cxf_nnr) { if (cxf_same(i, cxf_nro[r], cxf_nrn[r])) return 1; r = r + 1; } return 0; }
static int cxf_noret(int i) {
    return cxf_tis(i, "exit") || cxf_tis(i, "abort") || cxf_tis(i, "_exit") || cxf_tis(i, "_Exit") || cxf_tis(i, "sy_exit") ||
           cxf_tis(i, "die") || cxf_tis(i, "fatal") || cxf_tis(i, "longjmp") || cxf_tis(i, "siglongjmp") || cxf_has(i, "panic") ||
           cxf_has(i, "die_") || cxf_has(i, "fatal_") || cxf_has(i, "_die") || cxf_locnr(i);
}
static int cxf_kw(int i) {
    return cxf_tis(i, "return") || cxf_tis(i, "else") || cxf_tis(i, "case") || cxf_tis(i, "do") || cxf_tis(i, "sizeof") || cxf_tis(i, "goto");
}
static int cxf_var(int i, int add) {
    int v;
    v = 0; while (v < cxf_nv) { if (cxf_same(i, cxf_vo[v], cxf_vl[v])) return v; v = v + 1; }
    if (!add) return 0 - 1;
    if (cxf_nv >= CXF_V) { cxf_ov(2); return 0 - 1; }
    cxf_vo[cxf_nv] = cxf_o[i]; cxf_vl[cxf_nv] = cxf_n[i]; cxf_vd[cxf_nv] = 0; cxf_st[0][cxf_nv] = 0; cxf_fl[0][cxf_nv] = 0;
    cxf_nv = cxf_nv + 1;
    return cxf_nv - 1;
}
/* the first finding of the unit (kind 1 E2, 2 E2b); nothing at an unreachable point */
static int cxf_report(int v, int line, int kind) {
    if (cxf_dd[0] || cxf_hit) return 0;
    cxf_hit = kind; cxf_hv = v; cxf_hl = line;
    return 0;
}
static int cxf_cpy(int a, int b) { int v; v = 0; while (v < CXF_V) { cxf_st[a][v] = cxf_st[b][v]; cxf_fl[a][v] = cxf_fl[b][v]; v = v + 1; } cxf_dd[a] = cxf_dd[b]; return 0; }
static int cxf_join(int a, int b) {
    int v;
    if (cxf_dd[b]) return 0;
    if (cxf_dd[a]) return cxf_cpy(a, b);
    v = 0; while (v < cxf_nv) { if (!cxf_st[a][v] && cxf_st[b][v]) { cxf_st[a][v] = 1; cxf_fl[a][v] = cxf_fl[b][v]; } v = v + 1; }
    return 0;
}
static int cxf_kill(int a) { int v; v = 0; while (v < CXF_V) { cxf_st[a][v] = 0; cxf_fl[a][v] = 0; v = v + 1; } cxf_dd[a] = 1; return 0; }
static int cxf_adds(int a, int b) {
    int v;
    if (cxf_dd[b]) return 0;
    v = 0; while (v < cxf_nv) { if (cxf_st[b][v] && (cxf_dd[a] || !cxf_st[a][v])) return 1; v = v + 1; }
    return 0;
}
static int cxf_new(void) { if (cxf_ns >= CXF_S) { cxf_ov(1); return CXF_S - 1; } cxf_ns = cxf_ns + 1; return cxf_ns - 1; }
/* a declaration of V at the current depth: a deeper block saves the outer state of V first */
static int cxf_shadow(int v) {
    if (cxf_vd[v] >= cxf_depth) return 0;
    if (cxf_nsh >= CXF_SH) { cxf_ov(6); return 0; }
    cxf_shv[cxf_nsh] = v; cxf_shs[cxf_nsh] = cxf_st[0][v]; cxf_shf[cxf_nsh] = cxf_fl[0][v]; cxf_shd[cxf_nsh] = cxf_vd[v];
    cxf_nsh = cxf_nsh + 1;
    cxf_vd[v] = cxf_depth;
    return 0;
}
/* the end of the operand of the sizeof at K (not evaluated, not a use) */
static int cxf_szend(int k, int b) {
    int j;
    j = k + 1;
    if (cxf_tp(j, '(') && cxf_mt[j] > j) return cxf_mt[j] + 1;
    while (j < b && (cxf_tp(j, '*') || cxf_tp(j, '&') || cxf_tp(j, '!') || cxf_tp(j, '~') || cxf_tp(j, '-') || cxf_tp(j, '+'))) j = j + 1;
    if (j < b && cxf_tp(j, '(') && cxf_mt[j] > j) return cxf_mt[j] + 1;
    if (j < b && cxf_tid(j)) j = j + 1;
    while (j < b) {
        if ((cxf_tp(j, '[') || cxf_tp(j, '(')) && cxf_mt[j] > j) { j = cxf_mt[j] + 1; continue; }
        if (cxf_tp(j, '.') && cxf_tid(j + 1)) { j = j + 2; continue; }
        if (cxf_tp(j, '-') && cxf_tp(j + 1, '>') && cxf_tid(j + 2)) { j = j + 3; continue; }
        break;
    }
    return j;
}
/* first token of the last statement before the closing brace RB, or -1 */
static int cxf_last(int lb, int rb) {
    int j;
    j = rb - 1;
    if (!cxf_tp(j, ';')) return 0 - 1;
    j = j - 1;
    while (j > lb) {
        if ((cxf_tp(j, ')') || cxf_tp(j, ']')) && cxf_mt[j] >= 0) { j = cxf_mt[j] - 1; continue; }
        if (cxf_tp(j, ';') || cxf_tp(j, '{') || cxf_tp(j, '}')) break;
        j = j - 1;
    }
    return j + 1;
}
static int cxf_isdecl(int k) {
    if (!(cxf_tp(k + 1, ';') || cxf_tp(k + 1, ',') || cxf_tp(k + 1, '=') || cxf_tp(k + 1, '[') || cxf_tp(k + 1, ')'))) return 0;
    if (cxf_tid(k - 1) && !cxf_kw(k - 1)) return 1;
    return cxf_tp(k - 1, '*') && (cxf_tid(k - 2) || cxf_tp(k - 2, '*') || cxf_tp(k - 2, ',')) && !cxf_kw(k - 2);
}
static int cxf_local(int i, int lb, int rb) {
    int k;
    k = lb; if (cxf_tp(lb - 1, ')') && cxf_mt[lb - 1] > 0) k = cxf_mt[lb - 1];
    while (k < rb) { if (cxf_tid(k) && cxf_same(k, cxf_o[i], cxf_n[i]) && cxf_isdecl(k)) return 1; k = k + 1; }
    return 0;
}
static int cxf_stmt(int i, int rb);
static int cxf_send(int i, int rb) {
    int j;
    j = i;
    while (j < rb) {
        if ((cxf_tp(j, '(') || cxf_tp(j, '[') || cxf_tp(j, '{')) && cxf_mt[j] > j) { j = cxf_mt[j] + 1; continue; }
        if (cxf_tp(j, ';') || cxf_tp(j, '}')) return j;
        j = j + 1;
    }
    return rb;
}
/* tokens [A, B) in source order: releases, uses, reassignments */
static int cxf_expr(int a, int b) {
    int k; int s; int v; int as; int de; int da; int ad; int sz;
    k = a;
    while (k < b) {
        if (cxf_tis(k, "sizeof")) { k = cxf_szend(k, b); continue; }
        s = cxf_relarg(k);
        if (s >= 0) {
            v = cxf_var(s, 0);
            if (v >= 0) {
                if (cxf_st[0][v]) cxf_report(v, cxf_ln[s], 2);
                else { cxf_st[0][v] = 1; cxf_fl[0][v] = cxf_ln[s]; }
            }
            k = cxf_mt[k + 1] + 1; continue;
        }
        if (cxf_tid(k) && !cxf_tp(k + 1, '(')) {
            v = cxf_var(k, 0);
            if (v < 0 || cxf_tp(k - 1, '.') || (cxf_tp(k - 1, '>') && cxf_tp(k - 2, '-'))) { k = k + 1; continue; }
            as = cxf_tp(k + 1, '=') && !cxf_tp(k + 2, '=');
            de = (cxf_tp(k + 1, ';') || cxf_tp(k + 1, ',') || cxf_tp(k + 1, '=') || cxf_tp(k + 1, '[')) &&
                 ((cxf_tid(k - 1) && !cxf_kw(k - 1)) || (cxf_tp(k - 1, '*') && (cxf_tid(k - 2) || cxf_tp(k - 2, '*') || cxf_tp(k - 2, ',')) && !cxf_kw(k - 2)));
            da = as && cxf_tp(k - 1, '*') && !de;
            ad = cxf_tp(k - 1, '&') && !(cxf_tid(k - 2) || cxf_tp(k - 2, ')') || cxf_tp(k - 2, ']') || (k >= 2 && cxf_k[k - 2] == 3));
            sz = cxf_tis(k - 1, "sizeof") || (cxf_tp(k - 1, '(') && cxf_tis(k - 2, "sizeof"));
            if (sz) { k = k + 1; continue; }
            if (de) cxf_shadow(v);
            if (de || ad || (as && !da && !cxf_tp(k - 1, ']'))) { cxf_st[0][v] = 0; k = k + 1; continue; }
            if (cxf_st[0][v]) cxf_report(v, cxf_ln[k], 1);
            k = k + 1; continue;
        }
        k = k + 1;
    }
    return 0;
}
static int cxf_block(int lb) {
    int rb; int j; int nj; int v;
    rb = cxf_mt[lb]; j = lb + 1;
    cxf_depth = cxf_depth + 1;
    while (j < rb) { nj = cxf_stmt(j, rb); if (nj > j) j = nj; else j = j + 1; }
    cxf_depth = cxf_depth - 1;
    while (cxf_nsh > 0 && cxf_vd[cxf_shv[cxf_nsh - 1]] > cxf_depth) {
        cxf_nsh = cxf_nsh - 1; v = cxf_shv[cxf_nsh];
        cxf_st[0][v] = cxf_shs[cxf_nsh]; cxf_fl[0][v] = cxf_shf[cxf_nsh]; cxf_vd[v] = cxf_shd[cxf_nsh];
    }
    v = 0; while (v < cxf_nv) { if (cxf_vd[v] > cxf_depth) { cxf_st[0][v] = 0; cxf_vd[v] = 0; } v = v + 1; }
    return rb + 1;
}
static int cxf_label(int i) {
    int l;
    l = 0; while (l < cxf_nlb) { if (cxf_same(i, cxf_lbo[l], cxf_lbn[l])) return l + 1; l = l + 1; }
    if (cxf_nlb >= CXF_L) { cxf_ov(3); return 0 - 1; }
    cxf_lbo[cxf_nlb] = cxf_o[i]; cxf_lbn[cxf_nlb] = cxf_n[i]; cxf_kill(cxf_nlb + 1);
    cxf_nlb = cxf_nlb + 1;
    return cxf_nlb;
}
static int cxf_push(int kind) {
    if (cxf_nfr >= CXF_D) { cxf_ov(4); return 0 - 1; }
    cxf_frk[cxf_nfr] = kind; cxf_frdef[cxf_nfr] = 0;
    cxf_frb[cxf_nfr] = cxf_new(); cxf_kill(cxf_frb[cxf_nfr]);
    cxf_frc[cxf_nfr] = cxf_new(); cxf_kill(cxf_frc[cxf_nfr]);
    cxf_fre[cxf_nfr] = cxf_new(); cxf_cpy(cxf_fre[cxf_nfr], 0);
    cxf_nfr = cxf_nfr + 1;
    return cxf_nfr - 1;
}
/* while and for: COND [CA, CB) before every iteration, STEP [SA, SB) after it; the body is read once from the entry
   and once more from entry join back edge when the back edge adds a release; exit = entry join back edge (none for
   a loop without a condition) join every break */
static int cxf_loop(int ca, int cb, int sa, int sb, int body, int rb, int forever) {
    int m; int e; int bk; int x; int f; int end; int it;
    m = cxf_ns; end = body;
    cxf_expr(ca, cb);
    e = cxf_new(); cxf_cpy(e, 0); bk = cxf_new(); cxf_cpy(bk, 0);
    f = cxf_push(1);
    if (f < 0) { cxf_ns = m; return cxf_send(body, rb) + 1; }
    it = 0;
    while (it < 2) {
        end = cxf_stmt(body, rb);
        cxf_join(0, cxf_frc[f]);
        cxf_expr(sa, sb); cxf_expr(ca, cb);
        cxf_cpy(bk, 0);
        if (it == 1 || !cxf_adds(e, bk)) break;
        cxf_cpy(0, e); cxf_join(0, bk);
        it = it + 1;
    }
    if (forever) cxf_kill(0); else { x = cxf_new(); cxf_cpy(x, e); cxf_join(x, bk); cxf_cpy(0, x); }
    cxf_join(0, cxf_frb[f]);
    cxf_nfr = cxf_nfr - 1; cxf_ns = m;
    return end;
}
static int cxf_ever(int ca, int cb) { return ca == cb || (cb == ca + 1 && (cxf_tis(ca, "1") || cxf_tis(ca, "true"))); }
static int cxf_stmt(int i, int rb) {
    int c; int j; int e; int a; int m; int s1; int s2; int f; int it; int fall; int bk;
    if (i >= rb) return rb;
    if (cxf_tp(i, '{') && cxf_mt[i] > i) return cxf_block(i);
    if (cxf_tp(i, ';')) return i + 1;
    m = cxf_ns;
    if (cxf_tis(i, "if") && cxf_tp(i + 1, '(') && cxf_mt[i + 1] > i) {
        c = cxf_mt[i + 1];
        cxf_expr(i + 2, c);
        e = cxf_new(); cxf_cpy(e, 0);
        j = cxf_stmt(c + 1, rb);
        a = cxf_new(); cxf_cpy(a, 0);
        if (cxf_tis(j, "else")) { cxf_cpy(0, e); j = cxf_stmt(j + 1, rb); cxf_join(0, a); }
        else cxf_join(0, e);
        cxf_ns = m;
        return j;
    }
    if (cxf_tis(i, "while") && cxf_tp(i + 1, '(') && cxf_mt[i + 1] > i) {
        c = cxf_mt[i + 1];
        return cxf_loop(i + 2, c, c, c, c + 1, rb, cxf_ever(i + 2, c));
    }
    if (cxf_tis(i, "for") && cxf_tp(i + 1, '(') && cxf_mt[i + 1] > i) {
        c = cxf_mt[i + 1]; s1 = 0 - 1; s2 = 0 - 1; j = i + 2;
        while (j < c) {
            if ((cxf_tp(j, '(') || cxf_tp(j, '[') || cxf_tp(j, '{')) && cxf_mt[j] > j) { j = cxf_mt[j] + 1; continue; }
            if (cxf_tp(j, ';')) { if (s1 < 0) s1 = j; else if (s2 < 0) s2 = j; }
            j = j + 1;
        }
        if (s1 < 0 || s2 < 0) { cxf_expr(i + 2, c); return cxf_loop(c, c, c, c, c + 1, rb, 0); }
        cxf_expr(i + 2, s1);
        return cxf_loop(s1 + 1, s2, s2 + 1, c, c + 1, rb, cxf_ever(s1 + 1, s2));
    }
    if (cxf_tis(i, "do")) {
        e = cxf_new(); cxf_cpy(e, 0); bk = cxf_new(); cxf_cpy(bk, 0);
        f = cxf_push(1); j = i + 1; c = 0 - 1;
        if (f < 0) { cxf_ns = m; return i + 1; }
        it = 0;
        while (it < 2) {
            j = cxf_stmt(i + 1, rb);
            cxf_join(0, cxf_frc[f]);
            if (cxf_tis(j, "while") && cxf_tp(j + 1, '(') && cxf_mt[j + 1] > j) { c = cxf_mt[j + 1]; cxf_expr(j + 2, c); }
            cxf_cpy(bk, 0);
            if (it == 1 || !cxf_adds(e, bk)) break;
            cxf_cpy(0, e); cxf_join(0, bk);
            it = it + 1;
        }
        cxf_cpy(0, bk);
        if (c > 0 && cxf_ever(j + 2, c)) cxf_kill(0);
        cxf_join(0, cxf_frb[f]);
        cxf_nfr = cxf_nfr - 1; cxf_ns = m;
        if (c > 0) { if (cxf_tp(c + 1, ';')) return c + 2; return c + 1; }
        return j;
    }
    if (cxf_tis(i, "switch") && cxf_tp(i + 1, '(') && cxf_mt[i + 1] > i) {
        c = cxf_mt[i + 1];
        cxf_expr(i + 2, c);
        if (!cxf_tp(c + 1, '{')) return cxf_stmt(c + 1, rb);
        f = cxf_push(2);
        if (f < 0) { cxf_ns = m; return cxf_mt[c + 1] + 1; }
        cxf_kill(0);
        j = cxf_block(c + 1);
        fall = cxf_new(); cxf_cpy(fall, 0);
        cxf_cpy(0, cxf_frb[f]);
        cxf_join(0, fall);
        if (!cxf_frdef[f]) cxf_join(0, cxf_fre[f]);
        cxf_nfr = cxf_nfr - 1; cxf_ns = m;
        return j;
    }
    if (cxf_tis(i, "case") || (cxf_tis(i, "default") && cxf_tp(i + 1, ':'))) {
        j = i + 1;
        f = cxf_nfr - 1; while (f >= 0 && cxf_frk[f] != 2) f = f - 1;
        if (cxf_tis(i, "case"))
            while (j < rb && !(cxf_tp(j, ':') && !cxf_tp(j + 1, ':'))) { if ((cxf_tp(j, '(') || cxf_tp(j, '[')) && cxf_mt[j] > j) j = cxf_mt[j]; j = j + 1; }
        if (f >= 0) { cxf_join(0, cxf_fre[f]); if (cxf_tis(i, "default")) cxf_frdef[f] = 1; }
        return j + 1;
    }
    if (cxf_tis(i, "return")) {
        e = cxf_send(i, rb);
        cxf_expr(i + 1, e);
        cxf_kill(0);
        if (cxf_tp(e, ';')) return e + 1;
        return e;
    }
    if ((cxf_tis(i, "break") || cxf_tis(i, "continue")) && cxf_tp(i + 1, ';')) {
        f = cxf_nfr - 1;
        if (cxf_tis(i, "continue")) while (f >= 0 && cxf_frk[f] != 1) f = f - 1;
        if (f >= 0) { if (cxf_tis(i, "break")) cxf_join(cxf_frb[f], 0); else cxf_join(cxf_frc[f], 0); }
        cxf_kill(0);
        return i + 2;
    }
    if (cxf_tis(i, "goto") && cxf_tid(i + 1)) {
        a = cxf_label(i + 1);
        if (a > 0) cxf_join(a, 0);
        cxf_kill(0);
        if (cxf_tp(i + 2, ';')) return i + 3;
        return i + 2;
    }
    if (cxf_tis(i, "else")) return i + 1;
    if (cxf_tid(i) && cxf_tp(i + 1, ':') && !cxf_tp(i + 2, ':')) {
        a = cxf_label(i);
        if (a > 0) cxf_join(0, a);
        return i + 2;
    }
    e = cxf_send(i, rb);
    cxf_expr(i, e);
    if (cxf_tid(i) && cxf_tp(i + 1, '(') && cxf_noret(i)) cxf_kill(0);
    if (cxf_tp(e, ';')) return e + 1;
    if (e > i) return e;
    return i + 1;
}
static int cxf_body(int lb, int rb) {
    int d; int s; int pass; int gt; int nl; int l; int ch;
    cxf_nv = 0; cxf_nlb = 0; gt = 0;
    d = lb + 1;
    while (d < rb) {
        s = cxf_relarg(d);
        if (s >= 0 && cxf_local(s, lb, rb)) cxf_var(s, 1);
        if (cxf_tis(d, "goto")) gt = 1;
        d = d + 1;
    }
    pass = 0;
    while (1) {
        nl = cxf_nlb; l = 1;
        while (l <= nl) { d = 0; while (d < CXF_V) { cxf_gs[l][d] = cxf_st[l][d]; d = d + 1; } cxf_gd[l] = cxf_dd[l]; l = l + 1; }
        d = 0; while (d < CXF_V) { cxf_st[0][d] = 0; cxf_fl[0][d] = 0; cxf_vd[d] = 0; d = d + 1; }
        cxf_dd[0] = 0; cxf_nfr = 0; cxf_depth = 0; cxf_ns = CXF_L + 1; cxf_nsh = 0;
        cxf_block(lb);
        pass = pass + 1;
        if (!gt || cxf_hit || cxf_over) break;
        ch = cxf_nlb != nl; l = 1;
        while (l <= nl && !ch) { d = 0; while (d < CXF_V) { if (cxf_gs[l][d] != cxf_st[l][d]) ch = 1; d = d + 1; } if (cxf_gd[l] != cxf_dd[l]) ch = 1; l = l + 1; }
        if (!ch) break;
        if (pass >= CXF_GP) { cxf_ov(5); break; }
    }
    return 0;
}
static int cxf_isuns(int nm) { int k; k = 0; while (k < cxm_nuns) { if (cxm_eqn(cxm_src + cxf_o[nm], cxf_n[nm], cxm_uns[k])) return 1; k = k + 1; } return 0; }
/* the safe unit u: every unsafe NAME is a function of the unit, then each other function body is walked */
static int cxf_unit(int u) {
    int k; int d; int round; int nm; int rb; int s; int j; int ok;
    if (cxf_lex()) { cxm_start(u, 1); cxm_m("unisa safe: too large or unbalanced for the flow check in unit"); return cxm_fail(); }
    cxf_nnr = 0; round = 0;
    while (round < 3) {
        d = 0; k = 0;
        while (k < cxf_nt) {
            if (cxf_tp(k, '{')) {
                if (d == 0 && cxf_tp(k - 1, ')') && cxf_mt[k - 1] > 0 && cxf_tid(cxf_mt[k - 1] - 1) && cxf_mt[k] > k) {
                    nm = cxf_mt[k - 1] - 1; rb = cxf_mt[k]; s = cxf_last(k, rb);
                    if (s >= 0 && cxf_tid(s) && cxf_tp(s + 1, '(') && cxf_noret(s) && !cxf_locnr(nm) && cxf_nnr < CXF_NR) {
                        cxf_nro[cxf_nnr] = cxf_o[nm]; cxf_nrn[cxf_nnr] = cxf_n[nm]; cxf_nnr = cxf_nnr + 1;
                    }
                    k = rb + 1; continue;
                }
                d = d + 1;
            } else if (cxf_tp(k, '}') && d > 0) d = d - 1;
            k = k + 1;
        }
        round = round + 1;
    }
    j = 0;
    while (j < cxm_nuns) {
        ok = 0; d = 0; k = 0;
        while (k < cxf_nt && !ok) {
            if (cxf_tp(k, '{') && d == 0 && cxf_tp(k - 1, ')') && cxf_mt[k - 1] > 0 && cxf_tid(cxf_mt[k - 1] - 1) && cxf_mt[k] > k) {
                if (cxm_eqn(cxm_src + cxf_o[cxf_mt[k - 1] - 1], cxf_n[cxf_mt[k - 1] - 1], cxm_uns[j])) ok = 1;
                k = cxf_mt[k] + 1; continue;
            }
            if (cxf_tp(k, '{')) d = d + 1; else if (cxf_tp(k, '}') && d > 0) d = d - 1;
            k = k + 1;
        }
        if (!ok) { cxm_start(u, cxm_unsl[j]); cxm_m("unisa unsafe "); cxm_m(cxm_uns[j]); cxm_m(": no function "); cxm_m(cxm_uns[j]); cxm_m(" in this unit"); return cxm_fail(); }
        j = j + 1;
    }
    cxf_hit = 0; cxf_over = 0; d = 0; k = 0;
    while (k < cxf_nt && !cxf_hit) {
        if (cxf_tp(k, '{')) {
            if (d == 0 && cxf_tp(k - 1, ')') && cxf_mt[k - 1] > 0 && cxf_tid(cxf_mt[k - 1] - 1) && cxf_mt[k] > k) {
                nm = cxf_mt[k - 1] - 1; rb = cxf_mt[k];
                cxf_fo = cxf_o[nm]; cxf_fn = cxf_n[nm];
                if (!cxf_isuns(nm)) cxf_body(k, rb);
                if (cxf_over == 1 && !cxf_hit) { cxm_start(u, cxf_ln[nm]); cxm_m("unisa safe: "); cxm_mn(cxm_src + cxf_fo, cxf_fn); cxm_m(" is too deep for the flow check"); return cxm_fail(); }
                if (cxf_over && !cxf_hit) {
                    cxm_start(u, cxf_ln[nm]); cxm_m("unisa safe: ");
                    if (cxf_over == 2) cxm_m("more than 64 released names");
                    else if (cxf_over == 3) cxm_m("more than 32 goto labels");
                    else if (cxf_over == 4) cxm_m("loops or switches nested deeper than 64");
                    else if (cxf_over == 5) cxm_m("goto labels not stable after 64 walks");
                    else cxm_m("more than 256 shadowing declarations");
                    cxm_m(" for the flow check in "); cxm_mn(cxm_src + cxf_fo, cxf_fn);
                    return cxm_fail();
                }
                k = rb + 1; continue;
            }
            d = d + 1;
        } else if (cxf_tp(k, '}') && d > 0) d = d - 1;
        k = k + 1;
    }
    if (!cxf_hit) return 0;
    cxm_start(u, cxf_hl);
    if (cxf_hit == 2) cxm_m("unisa safe: double free of "); else cxm_m("unisa safe: use after free of ");
    cxm_mn(cxm_src + cxf_vo[cxf_hv], cxf_vl[cxf_hv]); cxm_m(" in "); cxm_mn(cxm_src + cxf_fo, cxf_fn);
    return cxm_fail();
}
/* the directive at s[i] ('#' at the start of a line), ending at the first newline not after a splice.  mode 0 handles
   `#pragma unisa`; every mode reads `#pragma unisa export` (the list belongs to the scan), tracks #if nesting
   (*cond) and records #define names.  The line is blanked after. */
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
        if (mode == 0) cxf_define(s, b, e);
    }
    if (cxm_eqn(s + a, b - a, "pragma")) {
        we = cxm_id(s, b, e, &wb);
        if (cxm_eqn(s + wb, we - wb, "unisa") && (we >= e || (s[we] & 255) <= 32) && mode != 0) {
            b = cxm_word(s, we, e, &a);
            if (cxm_eqn(s + a, b - a, "module")) cxm_modseen = 1;
            if (cxm_eqn(s + a, b - a, "export")) {
                if (seen) { cxm_start(u, line); cxm_m("unisa pragma after first declaration"); return cxm_fail(); }
                if (*cond > 0) { cxm_start(u, line); cxm_m("unisa pragma inside conditional"); return cxm_fail(); }
                if (cxm_exportline(u, line, b, e)) return 1;
            }
        }
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
                        if (sl == 0 || k < wn) { cxm_start(u, line); cxm_m("bad symbol name \""); cxm_mn(w + sb, wn - sb); cxm_m("\""); return cxm_fail(); }
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
                    cxm_modseen = 1;
                }
            } else if (cxm_eqn(s + a, b - a, "export")) {
                if (cxm_exportline(u, line, b, e)) return 1;
            } else if (cxm_eqn(s + a, b - a, "owns")) {
                we = cxm_word(s, b, e, &wb);
                if (we == wb) { cxm_start(u, line); cxm_m("unisa owns takes a function name"); return cxm_fail(); }
                while (we > wb) {
                    k = wb; while (k < we && cxm_idch(s[k] & 255)) k = k + 1;
                    if (k < we || we - wb >= CXM_SYMW || cxm_nown >= CXM_MAXEXP) { cxm_start(u, line); cxm_m("unisa owns takes a function name"); return cxm_fail(); }
                    cxm_cpyn(cxm_own[cxm_nown], s + wb, we - wb, CXM_SYMW); cxm_ownl[cxm_nown] = line; cxm_nown = cxm_nown + 1;
                    we = cxm_word(s, we, e, &wb);
                }
            } else if (cxm_eqn(s + a, b - a, "safe")) {
                we = cxm_word(s, b, e, &wb);
                if (we > wb) { cxm_start(u, line); cxm_m("unisa safe takes no operand"); return cxm_fail(); }
                cxm_safe = line;
            } else if (cxm_eqn(s + a, b - a, "unsafe")) {
                we = cxm_word(s, b, e, &wb);
                if (we == wb) { cxm_start(u, line); cxm_m("unisa unsafe takes a function name"); return cxm_fail(); }
                while (we > wb) {
                    k = wb; while (k < we && cxm_idch(s[k] & 255)) k = k + 1;
                    if (k < we || we - wb >= CXM_SYMW || cxm_nuns >= CXM_MAXEXP) { cxm_start(u, line); cxm_m("unisa unsafe takes a function name"); return cxm_fail(); }
                    cxm_cpyn(cxm_uns[cxm_nuns], s + wb, we - wb, CXM_SYMW); cxm_unsl[cxm_nuns] = line; cxm_nuns = cxm_nuns + 1;
                    we = cxm_word(s, we, e, &wb);
                }
            } else if (cxm_eqn(s + a, b - a, "dock")) {
                /* `#pragma unisa dock` (bare) or `#pragma unisa dock include|libc`: a
                   declaration only -- it records which boundary the unit is allowed to
                   touch and produces no code. Per plans/cx-dock-pragma.md a unit that is
                   NOT a module (no `#pragma unisa module`, nobody `use`s it) needs no
                   dock, so this never becomes mandatory by itself. */
                we = cxm_word(s, b, e, &wb);
                if (we > wb && !(cxm_eqn(s + wb, we - wb, "include") || cxm_eqn(s + wb, we - wb, "libc"))) {
                    cxm_start(u, line); cxm_m("unisa dock takes nothing or include|libc"); return cxm_fail();
                }
            } else { cxm_start(u, line); cxm_m("unknown unisa pragma "); cxm_mn(s + a, b - a); return cxm_fail(); }
        }
    }
    k = i; while (k < e) { if (s[k] != '\n') s[k] = ' '; k = k + 1; }
    return 0;
}
/* every owns NAME of the scanned unit is a non-static function defined in it (prd_cx.md 11.7, the second message) */
int cxm_ownschk(int u) {
    int j; int k; int nb; int nl; int isfn; int ok;
    j = 0;
    while (j < cxm_nown) {
        ok = 0; k = 0;
        while (k < cxm_nitem && !ok) {
            if (cxm_ibody[k] >= 0 && cxm_dname(cxm_src, cxm_ib[k], cxm_ibody[k], &nb, &nl, &isfn) && isfn
                && cxm_eqn(cxm_src + nb, nl, cxm_own[j]) && !cxm_haskw(cxm_src, cxm_ib[k], cxm_ibody[k], "static")) ok = 1;
            k = k + 1;
        }
        if (!ok) { cxm_start(u, cxm_ownl[j]); cxm_m("unisa owns "); cxm_m(cxm_own[j]); cxm_m(": no non-static function "); cxm_m(cxm_own[j]); cxm_m(" in this unit"); return cxm_fail(); }
        j = j + 1;
    }
    return 0;
}
/* scan the loaded unit: directives (mode 0: the unisa pragmas), then the file-scope items (cxm_ib..) */
int cxm_scan(int u, int mode) {
    char *s; int n; int i; int line; int bol; int depth; int cond; int seen; int ib; int iline; int icond; int body; int eq; int last; int q; int e;
    s = cxm_src; n = cxm_nsrc; i = 0; line = 1; bol = 1; depth = 0; cond = 0; seen = 0;
    ib = 0 - 1; iline = 0; icond = 0; body = 0 - 1; eq = 0; last = 0;
    cxm_nitem = 0; cxm_nmac = 0; cxm_nexp = 0; cxm_modseen = 0; cxm_nown = 0; cxm_safe = 0; cxm_nuns = 0; cxf_nrel = 0;
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
    if (cxm_nown > 0 && cxm_ownschk(u)) return 1;
    if (cxm_safe) return cxf_unit(u);
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
    j = 0;
    while (j < cxm_nexp) {                           /* every listed name is a non-static function this unit defines */
        int why; why = 6;
        k = 0;
        while (k < cxm_nitem && why != 0) {
            if (cxm_dname(s, cxm_ib[k], cxm_ibody[k] >= 0 ? cxm_ibody[k] : cxm_ie[k], &nb, &nl, &isfn) && cxm_eqn(s + nb, nl, cxm_exp[j])) {
                if (cxm_ikind[k] == 2) why = 0;
                else if (cxm_ibody[k] >= 0) why = 7;
                else if (!isfn && why == 6) why = 8;
            }
            k = k + 1;
        }
        if (why != 0) {
            cxm_start(u, 1); cxm_m("module "); cxm_m(cxm_name[u]); cxm_m(" exports "); cxm_m(cxm_exp[j]);
            cxm_m(why == 6 ? " but does not define it" : why == 7 ? "; it is static" : "; only functions are listed"); return cxm_fail();
        }
        j = j + 1;
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
            int pub; int fb;
            cxm_dname(s, cxm_ib[k], cxm_ibody[k], &nb, &nl, &isfn);
            pub = cxm_listed(u, s + nb, nl);
            if (pub < 0) return 1;
            if (cxm_nexp > 0) {                      /* the source list decides; a cxpack.tsv row must agree with it */
                int in; in = cxm_exported(s + nb, nl);
                if (pub != 2 && pub != in) {
                    cxm_start(u, cxm_iline[k]); cxm_m("module "); cxm_m(cxm_name[u]); cxm_m(" export list differs from cxpack.tsv: "); cxm_mn(s + nb, nl); return cxm_fail();
                }
                pub = in;
            }
            if (pub == 0) { if (cxm_addsym(s + nb, nl, u, cxm_iline[k], 3, 0, 0)) return 1; k = k + 1; continue; }
            fb = cxm_nift;
            if (cxm_emit(cxm_ib[k], cxm_ibody[k]) || cxm_nift >= CXM_IFMAX - 2) { cxm_start(u, cxm_iline[k]); cxm_m("interface too large"); return cxm_fail(); }
            cxm_ift[cxm_nift] = ';'; cxm_ift[cxm_nift + 1] = '\n'; cxm_nift = cxm_nift + 2;
            if (cxm_addsym(s + nb, nl, u, cxm_iline[k], 0, fb, cxm_nift)) return 1;
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
    cxm_o("#ifndef CXIF_"); cxm_o(g); cxm_o("_"); cxm_o(cxm_sha[u]);
    if (sym && sym[0]) { cxm_o("_"); cxm_o(sym); }
    cxm_o("\n");
    cxm_o("#define CXIF_"); cxm_o(g); cxm_o("_"); cxm_o(cxm_sha[u]);
    if (sym && sym[0]) { cxm_o("_"); cxm_o(sym); }
    cxm_o("\n");
    cxm_on2(cxm_ift + cxm_tb[u], cxm_te[u] - cxm_tb[u]);
    if (full) {
        if (sym && sym[0]) {
            k = 0;
            while (k < cxm_nsym) {
                if (cxm_symu[k] == u && cxm_symk[k] == 0 && cxm_eq(cxm_sym[k], sym))
                    cxm_on_rename(cxm_ift, cxm_symb[k], cxm_syme[k], sym, alias ? alias : "");
                k = k + 1;
            }
        } else {
            cxm_on_rename(cxm_ift, cxm_fb[u], cxm_fe[u], "", "");
        }
    }
    cxm_o("#endif\n");
    return cxm_nout >= CXM_IFMAX;
}
int cxm_outsha(char *hex) {
    unisa_sha256_hex(cxm_out, (size_t)cxm_nout, hex);
    return 0;
}
/* finish a renamed-body path at out[p]: <sha16 of cxm_out>.cx, so the name follows the bytes
   (no fixed name shared between concurrent invocations) */
int cxm_bodyname(char *out, int p) {
    char hex[65]; int k;
    cxm_outsha(hex);
    k = 0; while (k < 16) { out[p] = hex[k]; p = p + 1; k = k + 1; }
    out[p] = '.'; out[p + 1] = 'c'; out[p + 2] = 'x'; out[p + 3] = 0;
    return 0;
}
/* write cxm_out to $TMPDIR/ua-cxif-<sha16>.h unless that file already holds these bytes */
int cxm_put(char *path) {
    char hex[65]; char *td; int p; int k; int n;
    cxm_outsha(hex);
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
/* write a body-symbol-renamed copy of module v: "#define SYM ALIAS" then the original
   source, to a fresh temp file; returns the path in OUT (0 on failure).  The rename is a
   macro, so the function definition, its recursive calls and any static use of SYM all
   change symbol together. */
int cxm_rewrite(int v, char *sym, char *alias, char *out) {
    char *td; int n; int k; int p;
    n = cxm_rd(cxm_arg[v], cxm_cmp, CXM_IFMAX);
    if (n < 0) return 1;
    td = getenv("TMPDIR"); if (td == 0 || td[0] == 0) td = getenv("TEMP");   /* TEMP: Windows, as the forwarding unit */
    if (td == 0 || td[0] == 0) td = "/tmp";
    p = cxm_cpy(out, td, CXM_PATHW - 40);
    k = cxm_cpy(out + p, "/ua-bodysrc-", 12); p = p + k;
    cxm_nout = 0;
    if (sym && sym[0]) {
        if (alias && alias[0]) { cxm_o("#define "); cxm_o(sym); cxm_o(" "); cxm_o(alias); cxm_o("\n"); }
        /* every other exported function of v gets a per-symbol unique name, so the
           functions not imported by this symbol-level use do not clash at link time. */
        k = 0;
        while (k < cxm_nsym) {
            if (cxm_symu[k] == v && cxm_symk[k] == 0 && !cxm_eq(cxm_sym[k], sym)) {
                cxm_o("#define "); cxm_o(cxm_sym[k]); cxm_o(" ua_sym_"); cxm_od(k); cxm_o("\n");
            }
            k = k + 1;
        }
    }
    cxm_on2(cxm_cmp, n);
    cxm_bodyname(out, p);
    if (cxm_wr(out, cxm_out, cxm_nout)) return 1;
    return 0;
}
int cxm_rewrite_multi(int u, int v, char *out) {
    char *td; int n; int k; int p;
    n = cxm_rd(cxm_arg[v], cxm_cmp, CXM_IFMAX);
    if (n < 0) return 1;
    td = getenv("TMPDIR"); if (td == 0 || td[0] == 0) td = getenv("TEMP");
    if (td == 0 || td[0] == 0) td = "/tmp";
    p = cxm_cpy(out, td, CXM_PATHW - 40);
    k = cxm_cpy(out + p, "/ua-bodysrc-", 12); p = p + k;
    cxm_nout = 0;
    k = 0;
    while (k < cxm_nuse[u]) {
        if (cxm_use[u][k] == v && cxm_usym[u][k][0]) {
            if (cxm_ualias[u][k][0]) {
                cxm_o("#define "); cxm_o(cxm_usym[u][k]); cxm_o(" "); cxm_o(cxm_ualias[u][k]); cxm_o("\n");
            }
        }
        k = k + 1;
    }
    k = 0;
    while (k < cxm_nsym) {
        int imported = 0, z = 0;
        if (cxm_symu[k] == v && cxm_symk[k] == 0) {
            while (z < cxm_nuse[u]) {
                if (cxm_use[u][z] == v && cxm_usym[u][z][0] && cxm_eq(cxm_usym[u][z], cxm_sym[k])) { imported = 1; break; }
                z = z + 1;
            }
            if (!imported) { cxm_o("#define "); cxm_o(cxm_sym[k]); cxm_o(" ua_sym_"); cxm_od(k); cxm_o("\n"); }
        }
        k = k + 1;
    }
    cxm_on2(cxm_cmp, n);
    cxm_bodyname(out, p);
    if (cxm_wr(out, cxm_out, cxm_nout)) return 1;
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
/* 1 when unit w imports symbol SYM of module v: w is v itself, or w directly uses
   v (full import, or a symbol-level import that names SYM), or w reaches v
   transitively (a transitive symbol-level filter is not tracked). */
int cxm_imports(int w, int v, char *sym) {
    int k;
    if (w == v) return 1;
    k = 0;
    while (k < cxm_nuse[w]) {
        if (cxm_use[w][k] == v) {
            if (!cxm_usym[w][k][0]) return 1;
            if (cxm_eq(cxm_usym[w][k], sym)) return 1;
            return 0;
        }
        k = k + 1;
    }
    return cxm_reach[w][v];
}
/* a symbol-level use must name a function its module exports (CX-U-2) */
int cxm_usechk(void) {
    int u; int k; int i; int v;
    u = 0;
    while (u < cxm_n) {
        k = 0;
        while (k < cxm_nuse[u]) {
            if (cxm_usym[u][k][0]) {
                v = cxm_use[u][k]; i = 0;
                while (i < cxm_nsym && !(cxm_symu[i] == v && cxm_symk[i] == 0 && cxm_eq(cxm_sym[i], cxm_usym[u][k]))) i = i + 1;
                if (i == cxm_nsym) {
                    cxm_start(0 - 1, 0); cxm_m("module "); cxm_m(cxm_name[v]); cxm_m(" does not export "); cxm_m(cxm_usym[u][k]);
                    cxm_m(" (used by "); cxm_m(cxm_arg[u]); cxm_m(")"); return cxm_fail();
                }
            }
            k = k + 1;
        }
        u = u + 1;
    }
    return 0;
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
                        if (cxm_imports(w, cxm_symu[a], cxm_sym[a]) && cxm_imports(w, cxm_symu[b], cxm_sym[b])) {
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
/* a word that opens a declaration (storage class or type specifier), not sizeof */
int cxm_declword(char *t, int l) {
    char *k; int i; int b;
    k = "auto char const double enum extern float inline int long register restrict short signed static struct "
        "typedef union unsigned void volatile _Bool _Complex _Imaginary _Noreturn __inline __inline__ __restrict "
        "__restrict__ __const __volatile__ __signed__ __extension__ _Thread_local ";
    i = 0;
    while (k[i]) {
        b = i; while (k[i] != ' ') i = i + 1;
        if (i - b == l) { int j; j = 0; while (j < l && k[b + j] == t[j]) j = j + 1; if (j == l) return 1; }
        i = i + 1;
    }
    return 0;
}
int cxm_addloc(int u, char *t, int l) {
    int i;
    if (l <= 0 || l >= CXM_SYMW) return 0;
    i = 0;
    while (i < cxm_nloc) { if (cxm_locu[i] == u && cxm_eqn(t, l, cxm_locn[i])) return 0; i = i + 1; }
    if (cxm_nloc >= CXM_LOCS) { cxm_start(u, 1); cxm_m("too many local declarations for the call check"); return cxm_fail(); }
    cxm_cpyn(cxm_locn[cxm_nloc], t, l, CXM_SYMW);
    cxm_locu[cxm_nloc] = u; cxm_nloc = cxm_nloc + 1;
    return 0;
}
/* 1 when unit u defines the name or directly uses the module that exports it */
int cxm_sees(int u, char *nm) {
    int i; int k; int v;
    i = 0;
    while (i < cxm_nloc) { if (cxm_locu[i] == u && cxm_eq(cxm_locn[i], nm)) return 1; i = i + 1; }
    k = 0;
    while (k < cxm_nuse[u]) {
        v = cxm_use[u][k];
        i = 0;
        while (i < cxm_nsym) {
            if (cxm_symk[i] == 0 && cxm_symu[i] == v) {
                /* a symbol-level use shows only its symbol, under the alias when it has one */
                if (!cxm_usym[u][k][0]) { if (cxm_eq(cxm_sym[i], nm)) return 1; }
                else if (cxm_eq(cxm_sym[i], cxm_usym[u][k]) && cxm_eq(cxm_ualias[u][k][0] ? cxm_ualias[u][k] : cxm_usym[u][k], nm)) return 1;
            }
            i = i + 1;
        }
        k = k + 1;
    }
    return 0;
}
/* kind 0 (module export) or kind 2 (a non-module unit's function). At most one: collisions already refused two. */
int cxm_owner(char *nm, int *ou) {
    int i;
    i = 0;
    while (i < cxm_nsym) {
        if ((cxm_symk[i] == 0 || cxm_symk[i] == 2) && cxm_eq(cxm_sym[i], nm)) { *ou = cxm_symu[i]; return 1; }
        i = i + 1;
    }
    i = 0;
    while (i < cxm_nsym) {                           /* kind 3: a module's function that is not public */
        if (cxm_symk[i] == 3 && cxm_eq(cxm_sym[i], nm)) { *ou = cxm_symu[i]; return 2; }
        i = i + 1;
    }
    return 0;
}
int cxm_base(char *p, char *out) {
    int n; int b;
    n = cxm_len(p); b = n;
    while (b > 0 && p[b - 1] != '/') b = b - 1;
    return cxm_cpy(out, p + b, CXM_NAMEW);
}
int cxm_callerr(int u, int line, char *nm, int owner) {
    char base[CXM_NAMEW]; char who[CXM_NAMEW]; int i; int pub;
    cxm_start(u, line);
    i = 0; pub = 0;
    while (i < cxm_nsym) { if (cxm_symu[i] == owner && (cxm_symk[i] == 0 || cxm_symk[i] == 2) && cxm_eq(cxm_sym[i], nm)) pub = 1; i = i + 1; }
    if (!pub) { cxm_m("call to undeclared function "); cxm_m(nm); cxm_m(" (private to "); cxm_m(cxm_name[owner]); cxm_m(")"); return cxm_fail(); }
    cxm_m("call to undeclared function "); cxm_m(nm); cxm_m(" (declared in ");
    if (cxm_lib[owner] && cxm_name[owner][0]) cxm_m(cxm_name[owner]);
    else { cxm_base(cxm_arg[owner], who); cxm_m(who); }
    cxm_m(", which "); cxm_base(cxm_arg[u], base); cxm_m(base); cxm_m(" does not use)");
    return cxm_fail();
}
int cxm_isskip(char *t, int l) {
    if (cxm_eqn(t, l, "__attribute__") || cxm_eqn(t, l, "__attribute")) return 1;
    if (cxm_eqn(t, l, "__asm__") || cxm_eqn(t, l, "__asm") || cxm_eqn(t, l, "asm")) return 1;
    if (cxm_eqn(t, l, "__declspec") || cxm_eqn(t, l, "_Alignas")) return 1;
    return 0;
}
int cxm_unitinc;
/* index of the last non-blank byte in s[b..tb), or -1 */
int cxm_backc(char *s, int b, int tb) {
    int p;
    p = tb - 1;
    while (p >= b && (s[p] & 255) <= 32) p = p - 1;
    return p;
}
/* 1 when the previous token is a name that makes `NAME (` a declarator (`type NAME (`), not a statement keyword */
int cxm_declprev(char *s, int b, int p) {
    int q;
    if (p < b || !cxm_idch(s[p] & 255)) return 0;
    q = p;
    while (q > b && cxm_idch(s[q - 1] & 255)) q = q - 1;
    if (s[q] >= '0' && s[q] <= '9') return 0;
    if (cxm_eqn(s + q, p - q + 1, "return") || cxm_eqn(s + q, p - q + 1, "else") || cxm_eqn(s + q, p - q + 1, "case")) return 0;
    if (cxm_eqn(s + q, p - q + 1, "do") || cxm_eqn(s + q, p - q + 1, "goto") || cxm_eqn(s + q, p - q + 1, "sizeof")) return 0;
    return 1;
}
/* 1 when the unit has a quoted or angle #include line (its headers may declare names this scan cannot see) */
int cxm_hasinc(void) {
    int i; int j;
    i = 0;
    while (i < cxm_nsrc) {
        j = i;
        while (j < cxm_nsrc && (cxm_src[j] == ' ' || cxm_src[j] == '\t')) j = j + 1;
        if (j < cxm_nsrc && cxm_src[j] == '#') {
            j = j + 1;
            while (j < cxm_nsrc && (cxm_src[j] == ' ' || cxm_src[j] == '\t')) j = j + 1;
            if (j + 7 <= cxm_nsrc && cxm_eqn(cxm_src + j, 7, "include")) return 1;
        }
        while (i < cxm_nsrc && cxm_src[i] != '\n') i = i + 1;
        i = i + 1;
    }
    return 0;
}
/* one file-scope item. A declaration's function declarator is local. Anything else followed by '(' is a call,
   except a member call (`.NAME(`, `->NAME(`), a `type NAME(` declarator, a `(*NAME)` function pointer, and any
   name in a unit that has an #include. A call whose name some other unit defines, and this unit cannot see, is refused. */
int cxm_scanreg(int u, char *s, int b, int e, int line) {
    int i; int from; int paren; int indecl; int seeneq;
    i = b; from = b; paren = 0; indecl = 0; seeneq = 0;
    while (i < e) {
        int tb; int tl; int after; int look; int line_here;
        i = cxm_tok(s, i, e);
        if (cxm_tokl == 0) break;
        while (from < cxm_tokb) { if (s[from] == '\n') line = line + 1; from = from + 1; }
        tb = cxm_tokb; tl = cxm_tokl; after = i; line_here = line; from = after;
        if (tl == 1 && !cxm_idch(s[tb] & 255)) {
            if (s[tb] == '(') paren = paren + 1;
            else if (s[tb] == ')') { if (paren > 0) paren = paren - 1; }
            else if (s[tb] == '{' || s[tb] == '}') { indecl = 0; seeneq = 0; }
            else if (s[tb] == ';' && paren == 0) { indecl = 0; seeneq = 0; }
            else if (s[tb] == '=' && paren == 0) { seeneq = 1; indecl = 0; }
            continue;
        }
        if (!cxm_idch(s[tb] & 255) || (s[tb] >= '0' && s[tb] <= '9')) continue;
        if (paren == 0 && !indecl && !seeneq && cxm_declword(s + tb, tl)) { indecl = 1; continue; }
        look = cxm_tok(s, after, e);
        if (cxm_tokl == 0) break;
        if (cxm_tokl == 1 && s[cxm_tokb] == ')') {
            int p1; int p2;
            p1 = cxm_backc(s, b, tb);
            if (p1 >= b && s[p1] == '*') {
                p2 = cxm_backc(s, b, p1);
                if (p2 >= b && s[p2] == '(' && cxm_addloc(u, s + tb, tl)) return 1;
            }
        }
        if (cxm_tokl == 1 && s[cxm_tokb] == '(' && cxm_isskip(s + tb, tl)) {
            i = cxm_skipgrp(s, look, e); from = after; continue;
        }
        if (cxm_tokl == 1 && s[cxm_tokb] == '(') {
            if (indecl && paren == 0 && !seeneq) { if (cxm_addloc(u, s + tb, tl)) return 1; }
            else if (!cxm_ismac(s + tb, tl)) {
                char nm[CXM_SYMW]; int owner; int pc;
                cxm_cpyn(nm, s + tb, tl, CXM_SYMW);
                pc = cxm_backc(s, b, tb);
                if (pc >= b && (s[pc] == '.' || (s[pc] == '>' && pc > b && s[pc - 1] == '-'))) { i = cxm_tokb; from = after; continue; }
                if (cxm_declprev(s, b, pc)) { if (cxm_addloc(u, nm, tl)) return 1; i = cxm_tokb; from = after; continue; }
                if (cxm_unitinc) { i = cxm_tokb; from = after; continue; }
                if (!cxm_sees(u, nm) && cxm_owner(nm, &owner) && owner != u) {
                    cxm_callerr(u, line_here, nm, owner); cxm_badcalls = 1;
                }
            }
            i = cxm_tokb; from = after; continue;
        }
        i = cxm_tokb; from = after;
    }
    return 0;
}
/* reload each .cx unit and refuse a call that would be compiled as an implicit declaration */
int cxm_checkcalls(void) {
    int u;
    cxm_nloc = 0; cxm_badcalls = 0;
    u = 0;
    while (u < cxm_n) {
        int r; int k;
        if (!cxm_iscx[u]) { u = u + 1; continue; }
        r = cxm_load(cxm_arg[u], u);
        if (r > 0) return 1;
        if (r < 0) { u = u + 1; continue; }
        cxm_unitinc = cxm_hasinc();
        if (cxm_scan(u, 1)) return 1;
        k = 0;
        while (k < cxm_nitem) {
            if (cxm_scanreg(u, cxm_src, cxm_ib[k], cxm_ie[k], cxm_iline[k])) return 1;
            k = k + 1;
        }
        u = u + 1;
    }
    return cxm_badcalls;
}
/* the program's units: paths[0..n) as given (the command line), then every module their `use` closure reaches.
   0 with cxm_n units (cxm_arg[u] is the path to compile, the given string for a command-line unit), 1 refused. */
int cxm_plan(char **paths, int n) {
    int u; int k; int t; int v; int r; int direct;
    cxm_n = 0; cxm_on = 1; cxm_nsym = 0; cxm_nift = 0; cxm_ntopo = 0; cxm_nstk = 0; cxm_nren = 0;
    cxm_rootn = n;
    u = 0;
    while (u < n) {
        if (u >= CXM_MAXU) { cxm_start(0 - 1, 0); cxm_m("more than "); cxm_md(CXM_MAXU); cxm_m(" units in one program"); cxm_on = 0; return cxm_fail(); }
        cxm_arg[u] = paths[u];
        if (cxm_norm(paths[u], cxm_path[u])) cxm_cpy(cxm_path[u], paths[u], CXM_PATHW);
        cxm_stemof(paths[u], cxm_stem[u]); cxm_cpy(cxm_name[u], cxm_stem[u], CXM_NAMEW);
        cxm_iscx[u] = cxm_iscxpath(paths[u]); cxm_lib[u] = 0; cxm_nuse[u] = 0;
        u = u + 1;
    }
    cxm_n = n;
    u = 0;
    while (u < cxm_n) {                              /* breadth-first: the list grows while it is walked */
        cxm_full[u][0] = 0; cxm_typ[u][0] = 0; cxm_ninc[u] = 0; cxm_color[u] = 0; cxm_bodysrc[u][0] = 0;
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
    if (cxm_usechk()) { cxm_on = 0; return 1; }
    if (cxm_collisions()) { cxm_on = 0; return 1; }
    if (cxm_checkcalls()) { cxm_on = 0; return 1; }
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
                    int direct = 0, full = 0, anysym = 0;
                    k = 0;
                    while (k < cxm_nuse[u]) {
                        if (cxm_use[u][k] == v) {
                            direct = 1;
                            if (cxm_usym[u][k][0]) anysym = 1; else full = 1;
                        }
                        k = k + 1;
                    }
                    if (direct) {
                        if (full) {
                            cxm_inc[u][cxm_ninc[u]] = 2 * v + 1; cxm_ninc[u] = cxm_ninc[u] + 1;
                        } else if (anysym) {
                            k = 0;
                            while (k < cxm_nuse[u]) {
                                if (cxm_use[u][k] == v && cxm_usym[u][k][0]) {
                                    if (cxm_text(v, 1, cxm_usym[u][k], cxm_ualias[u][k]) || cxm_put(cxm_renpool[cxm_nren])) { cxm_on = 0; return 1; }
                                    cxm_inc[u][cxm_ninc[u]] = 0 - (cxm_nren + 1); cxm_nren = cxm_nren + 1; cxm_ninc[u] = cxm_ninc[u] + 1;
                                }
                                k = k + 1;
                            }
                            if (!cxm_bodysrc[v][0]) {
                                if (cxm_rewrite_multi(u, v, cxm_bodysrc[v])) { cxm_on = 0; return 1; }
                            }
                        }
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
        cxm_o("\t"); cxm_o(cxm_bodysrc[u][0] ? cxm_bodysrc[u] : cxm_arg[u]);
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
