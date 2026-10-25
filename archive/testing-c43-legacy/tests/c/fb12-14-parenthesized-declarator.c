/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/14-parenthesized-declarator.c (+ .out.txt for the gcc-vs-unisacc record). */
/* A parenthesized declarator name, `T (name)(params)`, is refused with
   "not covered: expected *" (only `(*name)` is accepted).  The idiom
   suppresses function-like macro expansion and is used for every API
   declaration in lua.h/lauxlib.h: LUA_API lua_State *(lua_newstate) (...);
   It also works for definitions and plain objects: int (v); */
#include <stdio.h>
int (twice)(int x);
char *(greet)(void);
int (v) = 5;
int (twice)(int x) { return 2 * x; }
char *(greet)(void) { return "hi"; }
int main(void) { printf("%d %s %d\n", twice(21), greet(), v); return 0; }
