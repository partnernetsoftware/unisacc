/* 0.0.31 H3: a static function reached only through a pointer keeps its
   forwarded calls (they bound to the program entry: rc 139) */
#include <stdio.h>
#include <string.h>
static void *dup(void *p) { puts(strdup("fwdptr")); return p; }
int main(void) { void *(*f)(void *) = dup; f(0); return 0; }
