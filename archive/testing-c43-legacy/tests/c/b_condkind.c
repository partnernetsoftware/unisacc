/* Conditional integer arms undergo the usual arithmetic conversions. */
#include <stdio.h>
int main(void) {
    int k = 1;
    char c = 3;
    unsigned int u = 2;
    printf("%lu %lu %lu %d %d\n", sizeof(k ? 1 : c), sizeof(k ? c : 1),
           sizeof(k ? c : c), (k ? -1 : u) < 0, (!k ? u : -1) < 0);
    return 0;
}
