/* C99 default argument promotion: float passed through ... is double. */
#include <stdio.h>
int main(void) {
    float f = (float)1.5;
    printf("%.1f %.1f\n", f, (float)2.5);
    return 0;
}
