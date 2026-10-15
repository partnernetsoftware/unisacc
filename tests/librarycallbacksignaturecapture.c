/* gen-source-sig PRODUCT TARGET SOURCE OUT: the product compiles a real C source (USLSIG3) and the signature bytes it
   emits are written to OUT.  Build from the export root: cc -I. gen-source-sig.c (includes exec/c/libunisacc.c). */
#include "exec/c/libunisacc.c"
int main(int argc, char **argv) {
    if (argc != 5) { fprintf(stderr, "usage: gen-source-sig PRODUCT TARGET SOURCE OUT\n"); return 2; }
    FILE *s = fopen(argv[3], "rb"); if (!s) return 2;
    static char src[65536]; size_t n = fread(src, 1, sizeof src - 1, s); fclose(s); src[n] = 0;
    us_context *c = us_new(argv[1]); if (!c) return 3;
    us_set_signature_version(c, 3);
    if (us_add_source(c, argv[3], src)) { puts(us_error(c)); return 4; }
    if (us_compile(c, argv[2], 0)) { puts(us_error(c)); return 5; }
    FILE *o = fopen(argv[4], "wb"); if (!o) return 6;
    fwrite(c->signatures, 1, c->signatures_length, o); fclose(o);
    printf("signature %zu bytes from %s\n", c->signatures_length, argv[3]);
    us_free(c); return 0;
}
