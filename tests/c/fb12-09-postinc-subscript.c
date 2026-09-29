/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/09-postinc-subscript.c (+ .out.txt for the gcc-vs-unisacc record). */
/* Postfix operators do not chain: `p++[0]` (subscript applied to the result
   of a postfix ++) is rejected with "expected ')'".  Valid C (postfix-
   expression [ expression ]); used by tinyexpr's lexer: switch (s->next++[0]). */
#include <stdio.h>
struct st { const char *next; };
int main(void) {
    const char *p = "abc";
    struct st s = { "xyz" };
    struct st *sp = &s;
    char a = p++[0];
    char b = sp->next++[1];
    printf("%c %c %c %c\n", a, *p, b, *sp->next);
    return 0;
}
