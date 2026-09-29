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
