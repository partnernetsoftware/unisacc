#include <stdio.h>
FILE *open_a(void){FILE *f=fopen("result","w");if(!f||fwrite("A",1,1,f)!=1)return 0;return f;}
int more_a(FILE *f){return fwrite("C",1,1,f)==1?0:1;}
