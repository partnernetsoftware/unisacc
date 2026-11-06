#include <stdio.h>
#include <stdint.h>
static int fn(void) { return 7; }
int main(void) {
    long l = 0;
    unsigned long ul = 0;
    long long ll = 0;
    char buf[256];
    printf("sizes long=%d ulong=%d longlong=%d ptr=%d int=%d size_t=%d intptr=%d\n",
           (int)sizeof(long), (int)sizeof(unsigned long), (int)sizeof(long long),
           (int)sizeof(void *), (int)sizeof(int), (int)sizeof(size_t), (int)sizeof(intptr_t));
    printf("fn_ptr=0x%llx\n", (unsigned long long)(void *)fn);
    l = (long)(void *)fn;
    printf("cast_long=0x%llx roundtrip=%d\n", (unsigned long long)(unsigned long)l, ((void *)l == (void *)fn) ? 1 : 0);
    ul = (unsigned long)(void *)fn;
    printf("cast_ulong=0x%llx roundtrip=%d\n", (unsigned long long)ul, ((void *)ul == (void *)fn) ? 1 : 0);
    ll = (long long)(void *)&buf[64];
    printf("cast_ll=0x%llx\n", (unsigned long long)ll);
    return 0;
}
