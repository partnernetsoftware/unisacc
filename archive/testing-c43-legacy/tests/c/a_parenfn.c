/* `T (name)(args);` declares a function even when T is a function-pointer typedef (lua.h API prototypes), and a call before the definition resolves to it -- compared with cc */
#include <stdio.h>
typedef int (*CF)(int);
static int inc(int x){return x+1;}
extern CF (getf) (int which);
int use(void){ CF f = getf(0); return f(4); }
CF getf (int which) { return inc; }
int main(void){ printf("%d\n", use()); return 0; }
