#include <stdio.h>
#include <stdint.h>
uint16_t y = 5;
uint32_t x = 7;
uint32_t *p = &x;
uint16_t *q = &y;
int main(void) {
    int z = (+y) < -1;
    x = ((*p = (1, (+((*q)--)))) > 0);
    printf("%lu %d %u %u\n", (unsigned long)sizeof(+y), z, x, (unsigned)y);
    return 0;
}
