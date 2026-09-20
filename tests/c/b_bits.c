/* Bit-fields: packed in declaration order, a new storage unit when one
   would straddle, read by shifting up and back down so the sign question
   answers itself.  An enum bit-field is unsigned unless some enumerator is
   negative -- get that wrong and 148 in eight bits reads back as -108. */
enum tree_code { SOME_CODE = 148, LAST_CODE };
struct S {
    unsigned a : 3;
    int b : 5;
    int c;
    enum tree_code e : 8;
    signed f : 3;
    unsigned : 2;          /* unnamed padding */
    unsigned g : 1;
};
struct S gv = { 5, -7, 99, SOME_CODE, -3, 1 };
struct Straddle { unsigned p : 30; unsigned q : 6; };
int main(void) {
    struct S l = { 3, 9, 42, 148, 1, 0 };
    int x;
    printf("%u %d %d %d %d %u\n", gv.a, gv.b, gv.c, (int) gv.e, gv.f, gv.g);
    printf("%u %d %d %d %d %u\n", l.a, l.b, l.c, (int) l.e, l.f, l.g);
    gv.a = 6; gv.b = 14; gv.f = -4;
    x = gv.a++;
    printf("%u %d %d %d\n", gv.a, x, ++gv.b, gv.f);
    gv.b += 20;
    printf("%d %d %d\n", gv.b, (int) sizeof(struct S),
           (int) sizeof(struct Straddle));
    printf("%d\n", gv.e == SOME_CODE);
    return 0;
}
