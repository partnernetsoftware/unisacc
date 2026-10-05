/* 0.0.29 T3: preprocessor paths no probe reached -- character constants with every escape in #if,
   stringizing arguments that hold quotes and escapes, _Pragma, #line, and macros rescanning literals */
#include <stdio.h>
#include <string.h>
#if '\0' == 0 && '\2' == 2 && '\3' == 3 && '\4' == 4 && '\x41' == 65 && '\?' == 63 && '\n' == 10 && '\t' == 9
#define LIT_OK 1
#else
#define LIT_OK 0
#endif
#if '\\' == 92 && '\'' == 39 && '"' == 34 && '\a' == 7 && '\b' == 8 && '\f' == 12 && '\v' == 11 && '\r' == 13 && '\177' == 127
#define LIT_OK2 1
#else
#define LIT_OK2 0
#endif
#define STR(x) #x
#define XSTR(x) STR(x)
#define QUOTE(a, b) STR(a) STR(b)
#define WITH_LIT(c) ((c) == '\'' ? "quote" : (c) == '"' ? "dquote" : "other")
_Pragma("once")
_Pragma ( "GCC diagnostic push" )
_Pragma ( "GCC diagnostic pop" )
int main(void) {
    const char *s1 = STR('a' "b\"c" '\'' "\\" '\x7f');
    const char *s2 = QUOTE("it's", 'x');
    const char *s3 = XSTR(LIT_OK);
#line 100 "t3pp.c"
    int line = 100;   /* #line directives are the probe; __LINE__ is not in the Python control route */
#line 200
    int line2 = 200;
    printf("%d %d %s|%s|%s %d %d %s %s %zu\n", LIT_OK, LIT_OK2, s1, s2, s3, line, line2,
           WITH_LIT('\''), WITH_LIT('"'), strlen(s1));
    return 0;
}
