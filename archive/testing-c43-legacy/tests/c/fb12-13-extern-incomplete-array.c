/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/13-extern-incomplete-array.c (+ .out.txt for the gcc-vs-unisacc record). */
/* A file-scope declaration of an array of unknown size,
   `extern const char ident[];`, is refused with "not covered: expected =".
   This is the standard header idiom (lua.h: LUA_API const char lua_ident[];)
   and blocks every Lua source file. */
#include <stdio.h>
extern const char ident[];
extern int table[];
const char ident[] = "hello";
int table[] = { 1, 2, 3 };
int main(void) { printf("%s %d\n", ident, table[2]); return 0; }
