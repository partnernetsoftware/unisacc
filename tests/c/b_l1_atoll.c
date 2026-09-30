#include <stdio.h>
#include <stdlib.h>
int main(void) {
    printf("%lld %lld %lld\n", atoll("-17"), atoll(" +42tail"), atoll("0"));
    return 0;
}
