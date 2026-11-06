#include <stdio.h>
FILE *open_a(void); int act_b(FILE*,int); int more_a(FILE*);
int main(int argc,char **argv){int m=argv[1][0]-'0';FILE *f=open_a();if(!f)return 3;if(act_b(f,m))return 4;if(m==2&&more_a(f))return 5;return m==2?fclose(f)!=0:0;}
