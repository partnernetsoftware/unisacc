/* LAYER (measured on candidate 0.0.13-dev4, 2026-09-30): E3, the signature pool.
   dev4 rejects the build at `ctype.h:20:19: error: not covered: function signature
   pool exhausted` (`static int isupper(...)`), i.e. while walking the bundled
   header.  It carries a position, unlike the no-position rejects below.  Thirty
   translation units exhaust a pool sized for fewer; the smallest repro is this
   fixture's own `w*.c` set, and the pool is an E3 table, not a src/ limit.
*/
/* Driver for bug 27; the program is this file + 27-signature-pool/w1.c ..
   w30.c (each unit: #include stdio/stdlib/string/ctype, one static qsort
   comparator, one function calling ~12 common libc functions).
   With 20 units it compiles and runs; with 30 units compilation fails:
     "error: not covered: function signature pool exhausted"
   (with -fno-trim-libc the limit is hit at ~10 small units).
   Every unit gets its own renamed static copy of each libc body, so real
   multi-file programs hit this: sbase tools built with libutil+libutf
   (~30 units) all fail (cat, rev, cut, uniq, seq, comm, paste, cmp,
   strings). */
int f1(const char *);
int main(void) { return f1("3") == 5 ? 0 : 1; }
