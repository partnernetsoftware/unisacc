/* C99 6.5.7p3: each operand is promoted and the result has the type of the
   PROMOTED LEFT one -- the usual arithmetic conversions do not apply, so a
   shift never widens to the right operand's type.  And a macro argument is
   substituted VERBATIM: parenthesising it turns `(T) 1` into `((T)) 1`,
   which is not a cast at all. */
#define SH(X, T) ((X) << (T) 1)
#define SZ(M) ((int) sizeof(M))
int main(void) {
    short s = 3;
    unsigned char c = 200;
    printf("%d %d %d\n", SZ(s), SZ(SH(s, short)), SZ(SH(s, unsigned long)));
    printf("%d %d\n", SZ(SH(1L, int)), SZ(SH(1, long)));
    printf("%d %d\n", (int) (s << 2), (int) (c >> 3));
    printf("%d %d\n", (int) ((unsigned char) 255 >> 4), -8 >> 1);
    return 0;
}
