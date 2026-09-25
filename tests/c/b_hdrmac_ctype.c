/* A macro named c before <ctype.h>. */
#include <stdio.h>
#define c 5
#include <ctype.h>
int main(void) {
    const char *t = "aZ9 _!";
    int j;
    for (j = 0; t[j]; j++)
        printf("%d%d%d%d%d%c ", isalpha(t[j]) != 0, isdigit(t[j]) != 0, isspace(t[j]) != 0,
               isupper(t[j]) != 0, ispunct(t[j]) != 0, toupper(t[j]));
    printf("%c %d\n", tolower('Q'), c);
    return 0;
}
