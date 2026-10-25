/* Token-class names are ordinary C identifiers, never keywords. The old
   lexer treated str as a literal and num as a numeric constant. */
#include <stdio.h>
char *word = "one";
static int f(int num, const char *str) { return num + str[0]; }
int main(void) {
    char **str = &word;
    int num = 1, type = 2, id = 3, eof = 4;
    printf("%d %s %s %d\n", num + type + id + eof, str[0], str[0]+1, f(2,"a"));
    return 0;
}
