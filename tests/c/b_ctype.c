/* The libc floor: <ctype.h>, <limits.h>, <assert.h>, and exit().  Almost
   every real C program leans on all four, and until now we had none. */
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <assert.h>
#include <stdlib.h>

int main(void)
{
    const char *s = "aB3 .\t";
    int letters = 0, digits = 0, spaces = 0, punct = 0;
    int i = 0;
    while (s[i]) {
        if (isalpha(s[i])) letters = letters + 1;
        if (isdigit(s[i])) digits = digits + 1;
        if (isspace(s[i])) spaces = spaces + 1;
        if (ispunct(s[i])) punct = punct + 1;
        i = i + 1;
    }
    printf("%d %d %d %d\n", letters, digits, spaces, punct);
    printf("%c%c %d %d\n", toupper('q'), tolower('Q'),
           isxdigit('e'), isgraph(' '));
    printf("%d %d %d %d\n", CHAR_BIT, INT_MAX, SHRT_MAX, UCHAR_MAX);
    printf("%d\n", INT_MIN + 2147483647);

    assert(letters == 2);
    assert(CHAR_BIT == 8);
    if (digits != 1) exit(3);           /* exit() is a real call now */
    printf("done\n");
    return 0;
}
