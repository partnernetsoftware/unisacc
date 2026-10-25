#include <stdio.h>
int main(void){ int s=0; for(int i=0;i<10;i++){ if(i%2) continue; if(i>6) break; s+=i; } printf("%d\n", s); return 0; }
