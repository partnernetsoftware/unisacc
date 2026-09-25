/* Macros named s, dst, src, i, c, d, p before <string.h>. */
#define s 1
#define dst 2
#define src 3
#define i 4
#define c 5
#define d 6
#define p 7
#include <string.h>
#include <stdio.h>
int main(void) {
    char b[32];
    strcpy(b, "hello");
    strcat(b, " world");
    printf("%ld %d %d\n", (long)strlen(b), strcmp(b, "hello") > 0, strncmp(b, "help", 3));
    printf("%s|%s\n", strchr(b, 'o'), strrchr(b, 'o'));
    printf("%s\n", strstr(b, "wor"));
    memmove(b + 1, b, 5); b[6] = 0;
    printf("%s %d\n", b, memcmp("ab", "ac", 2) < 0);
    memset(b, 'x', 3); b[3] = 0;
    printf("%s %d %d %d %d %d %d %d\n", b, s, dst, src, i, c, d, p);
    return 0;
}
