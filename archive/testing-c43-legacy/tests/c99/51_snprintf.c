#include <stdio.h>
int main(void){ char b[8]; int n = snprintf(b,sizeof b,"%d-%d",12,34); printf("%s %d\n", b, n); return 0; }
