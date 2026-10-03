#include <stddef.h>
#include <stdio.h>

struct Inner { char c; long value; };
struct Outer { int tag; struct Inner row[3]; char tail; };
static int direct_offset(void) { return __builtin_offsetof(struct Outer, tail); }

int main(void) {
    printf("%lu %lu %lu\n",
           (unsigned long)offsetof(struct Outer, row),
           (unsigned long)offsetof(struct Outer, row[2].value),
           (unsigned long)__builtin_offsetof(struct Outer, tail));
    printf("%d\n", direct_offset());
    return 0;
}
