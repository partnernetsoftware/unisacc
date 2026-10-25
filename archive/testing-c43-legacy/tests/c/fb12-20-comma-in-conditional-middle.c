/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/20-comma-in-conditional-middle.c (+ .out.txt for the gcc-vs-unisacc record). */
/* The middle operand of ?: is a full `expression` (C11 6.5.15), so it may
   contain an unparenthesized comma:  c ? f(), 0 : 0.  unisacc stops at the
   comma: "expected ':'".  stb_image_write.h's stb_sb_free() macro expands
   to exactly this: (void)((a) ? free(...), 0 : 0). */
#include <stdio.h>
static int n;
static void bump(void) { n = n + 1; }
int main(void) {
    int c = 1;
    int r = c ? bump(), 7 : 0;
    (void)(c ? bump(), 0 : 0);
    printf("%d %d\n", r, n);
    return 0;
}
