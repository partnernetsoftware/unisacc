/* Brace elision (C99 6.7.8p17): an aggregate element takes its share of a
   flat list, so `PT a[] = {...}` is not one element per comma -- and a
   parenthesis in that list is arithmetic, not `(T){...}`.  A trailing comma
   adds no element either. */
typedef long I;
typedef struct { I c[2]; I b; } PT;
PT flat[] = { ((I)4L + (I)2L), (I)8L, -1L,
              (I)1L, ((I)2L + (I)4L), 3L, };
PT braced[] = { {{1, 2}, 3}, {{4, 5}, 6} };
int plain[] = { 1, 2, 3, };
int main(void) {
    printf("%ld %ld %ld %ld\n", flat[0].c[0], flat[0].c[1], flat[0].b,
           flat[1].c[1]);
    printf("%ld %ld %d %d\n", braced[1].c[0], braced[1].b,
           (int) (sizeof plain / sizeof plain[0]),
           (int) (sizeof flat / sizeof flat[0]));
    return 0;
}
