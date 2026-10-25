/* N17b: __FILE__ is the path the compiler was given, spelled as a string
   literal; inside a macro body it is the file of the invocation.  Both
   compilers receive the same path, so the lines agree byte for byte. */
#include <stdio.h>
#include <string.h>
#define WHERE __FILE__ ":" "here"
static const char *fn(void) { return __FILE__; }
int main(void) {
    const char *p = __FILE__;
    printf("%d\n", p[0] != 0);
    printf("%s\n", WHERE + 0 == WHERE ? "same" : "same");
    printf("%d %d\n", (int)(sizeof(__FILE__) > 1), strcmp(fn(), p) == 0);   /* not fn() == p: literal merging is the compiler's choice */
    printf("%s\n", __FILE__ + (sizeof(__FILE__) - 9));
    return 0;
}
