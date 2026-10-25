#include <stdio.h>
int main(void){ int a[5] = { [1]=10, [3]=30 }; printf("%d %d %d\n", a[0], a[1], a[3]); return 0; }
