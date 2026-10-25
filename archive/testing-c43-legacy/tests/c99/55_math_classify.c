#include <stdio.h>
#include <math.h>
int main(void){ double z = 0.0; printf("%d %d\n", isnan(z/z), isinf(1.0/z)); return 0; }
