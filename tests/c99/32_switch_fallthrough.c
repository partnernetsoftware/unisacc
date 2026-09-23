#include <stdio.h>
int main(void){ int s=0; switch(2){ case 1: s+=1; case 2: s+=2; case 3: s+=4; break; default: s=99; } printf("%d\n", s); return 0; }
