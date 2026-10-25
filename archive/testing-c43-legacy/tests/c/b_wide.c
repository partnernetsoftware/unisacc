/* Wide string literals.  The source is read one byte per character, so the
   literal is still UTF-8 here and the lexer is the only place that knows it
   is a literal at all -- it decodes to code points.  wchar_t is four bytes
   on every target, so one tape still lowers to six. */
#include <wchar.h>
wchar_t g[] = L"abé你";
wchar_t *p2 = L"xy";
int main(void) {
    wchar_t s[] = L"hi" L"!\n";
    wchar_t *p;
    int n;
    n = 0;
    for (p = s; *p; p++) { printf("%04X ", (unsigned) *p); n++; }
    printf("| %d %d\n", n, (int) (sizeof s / sizeof s[0]));
    printf("%04X %04X %04X %04X | %04X %04X | %d\n", (unsigned) g[0],
           (unsigned) g[1], (unsigned) g[2], (unsigned) g[3], (unsigned) p2[0],
           (unsigned) p2[1], (int) wcslen(g));
    return 0;
}
