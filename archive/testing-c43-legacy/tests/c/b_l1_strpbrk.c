#include <stdio.h>
#include <string.h>
int main(void) {
    char s[] = "abcde";
    char *p = strpbrk(s, "xzdc");
    printf("%ld %d %d\n", p ? (long)(p-s) : -1L,
           strpbrk(s, "") == 0, strpbrk("", "a") == 0);
    return 0;
}
