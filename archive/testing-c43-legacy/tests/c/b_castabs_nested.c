/* 0.0.34 L2 (d8166d04): nested abstract declarators in a cast, as sqlite's dlsym cast */
#include <stdio.h>
static int hit;
static void target(void) { hit = 7; }
static void *getter(void *h, const char *s) { return (void *)target; }
int main(void) {
    void (*(*x)(void *, const char *))(void);
    int (*a)[3]; static int arr[3] = { 1, 2, 3 };
    x = (void (*(*)(void *, const char *))(void))getter;
    ((void (*)(void))x(0, "t"))();
    a = (int (*)[3])arr;
    printf("%d %d\n", hit, (*a)[2]);
    return 0;
}
