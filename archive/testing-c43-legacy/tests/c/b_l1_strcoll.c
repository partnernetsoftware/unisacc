#include <stdio.h>
#include <string.h>
#include <locale.h>
int main(void) {
    int a, b, c;
    setlocale(LC_ALL, "C");
    a = strcoll("abc", "abd");
    b = strcoll("abc", "abc");
    c = strcoll("abd", "abc");
    printf("%d %d %d\n", (a > 0) - (a < 0), b, (c > 0) - (c < 0));
    return 0;
}
