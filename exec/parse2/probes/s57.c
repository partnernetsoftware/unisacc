/* Initializer names come from declarations, not identifiers in array bounds. */
#include <stdio.h>
enum { N=3 };
int a[N]={2,4,6}, b[N+1]={1,3,5,7};
unsigned long hi=9223372036854775808ull;
int main(void) {
    printf("%d %d %d %d %d\n", a[0],a[N-1],b[0],b[N],hi==0x8000000000000000UL);
    return 0;
}
