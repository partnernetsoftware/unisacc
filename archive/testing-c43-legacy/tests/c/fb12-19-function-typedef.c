/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/19-function-typedef.c (+ .out.txt for the gcc-vs-unisacc record). */
/* A typedef of a function type (not a pointer), then used as `name *`,
   is refused: "expected ';'".  Standard idiom, used by stb_image_write.h:
     typedef void stbi_write_func(void *context, void *data, int size); */
#include <stdio.h>
typedef int binop(int a, int b);
static int add(int a, int b) { return a + b; }
static int apply(binop *f, int a, int b) { return f(a, b); }
int main(void) {
    binop *f = add;
    printf("%d %d\n", apply(add, 2, 3), f(4, 5));
    return 0;
}
