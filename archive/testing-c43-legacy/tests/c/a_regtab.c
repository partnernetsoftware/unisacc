/* typedef struct with a function-pointer-typedef member: sizeof and a luaL_Reg-style table (lua lauxlib/linit) -- compared with cc */
#include <stdio.h>
typedef int (*CF)(void *L);
typedef struct Reg { const char *name; CF func; } Reg;
static int fa(void *L) { return 1; }
static int fb(void *L) { return 2; }
static const Reg tab[] = { {"a", fa}, {"b", fb}, {NULL, NULL} };
static const Reg libs[] = { {"x", fb}, {"y", NULL} };
int main(void) { const Reg *r; int s = 0; for (r = tab; r->func; r++) s = s * 10 + r->func(0);
  { CF f = libs[0].func; s = s * 10 + (*f)(0); }
  printf("%d\n", s); return 0; }
