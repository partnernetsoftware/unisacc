/* Function scope and literal order: each occurrence pools independently. */
#include <stdio.h>
const char *alpha(void) { return __func__; }
const char *beta(void) { const char *p = "before"; printf("%s %s\n", p, __func__); return __func__; }
int main(void) {
    const char *a = alpha();
    const char *b = beta();
    printf("%s %s %s %s\n", a, b, __func__, "after");
    return 0;
}
