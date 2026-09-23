#include <stdio.h>
int main(void){ int i=0; again: i++; if(i<3) goto again; printf("%d\n", i); return 0; }
