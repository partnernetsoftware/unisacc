#include <stdio.h>
int act_b(FILE *f,int mode){if(fwrite("B",1,1,f)!=1)return 1;if(mode==0)return fclose(f)!=0;if(mode==2)return fflush(f)!=0;return 0;}
