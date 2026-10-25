/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/15b-cast-in-case-label.c (+ .out.txt for the gcc-vs-unisacc record). */
/* Same family as 15: a cast inside a case label's integer constant
   expression is refused ("not covered: constant expression").
   stb_image.h: case STBI__PNG_TYPE('C','g','B','I'): with
   #define STBI__PNG_TYPE(a,b,c,d) (((unsigned)(a) << 24) + ...) */
#include <stdio.h>
#define TAG(a,b) (((unsigned)(a) << 8) + (unsigned)(b))
int main(void) {
    unsigned t = TAG('I', 'D');
    switch (t) {
    case TAG('I', 'H'): printf("IH\n"); break;
    case TAG('I', 'D'): printf("ID\n"); break;
    case (int)3: printf("3\n"); break;
    }
    return 0;
}
