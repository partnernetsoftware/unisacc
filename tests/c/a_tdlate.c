/* sizeof a typedef whose struct is completed after the typedef (lua's
   `typedef struct CallInfo CallInfo;`), offsetof, a macro followed by a
   string, and a comma expression in a controlling expression */
#include <stdio.h>
#include <stddef.h>
typedef struct CI CI;
typedef struct LX { int a; struct { long x; char c[3]; } l; } LX;
struct CI { long a, b; CI *next; };
#define PFX "luaopen_"
int main(void) {
    int a = 0, b = 3;
    while ((void)(a++), b-- > 0) ;
    printf("%d %d %d %d\n", (int)sizeof(CI), (int)sizeof(struct CI), (int)offsetof(LX, l), (int)offsetof(LX, l.c[2]));
    printf(PFX"%s %d\n", "x", a);
    return 0;
}
