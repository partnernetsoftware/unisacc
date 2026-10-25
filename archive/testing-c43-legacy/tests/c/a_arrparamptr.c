/* 0.0.22: an array parameter of pointers is a pointer to pointers (C99 6.7.5.3p7).
   The reference took `char *v[]` as char *, so v[1][0] read a byte of the pointer. */
#include <stdio.h>
static char *words[] = {"ab", "cd", "ef", 0};
static int x = 7, y = 9;
static int *ptrs[2] = {&x, &y};
static int count(char *const v[]) { int n = 0; while (v[n]) n = n + 1; return n; }
static int second(char *v[]) { return v[1][0] * 100 + (int)sizeof v[0]; }
static int deref(int *v[]) { return *v[0] + *v[1]; }
static int plain(int a[]) { return a[1]; }
int main(void) {
    int z[2] = {3, 4};
    printf("%d %d %d %d\n", count(words), second(words), deref(ptrs), plain(z));
    return 0;
}
