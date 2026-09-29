/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/21-indirect-call-six-args.c (+ .out.txt for the gcc-vs-unisacc record). */
/* An indirect call (through a function pointer in a struct) with six
   arguments fails to compile: "not covered: six indirect register
   arguments leave no callee register".  Direct calls with six args and
   indirect calls with five work.  stb_image.h's JPEG decoder calls
   z->YCbCr_to_RGB_kernel(out, y, pcb, pcr, count, step) this way. */
#include <stdio.h>
typedef void (*k6)(int *out, int a, int b, int c, int d, int e);
struct Z { k6 fn; };
static void kern(int *out, int a, int b, int c, int d, int e) { *out = a + b + c + d + e; }
int main(void) {
    struct Z z;
    int r = 0;
    k6 f = kern;
    z.fn = kern;
    z.fn(&r, 1, 2, 3, 4, 5);
    printf("%d\n", r);
    f(&r, 2, 2, 3, 4, 5);
    printf("%d\n", r);
    return 0;
}
