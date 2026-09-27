#include <math.h>
int main(void) {
    double x = sqrt(2.0);
    if (x < 1.414 || x > 1.415) return 1;
    if (isnan(x)) return 2;
    return 0;
}
