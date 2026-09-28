/* Binary host IO must preserve every byte, including CRLF and DOS EOF. */
#define main executor_main
#include "../exec/c/run.c"
#undef main
int main(int argc,char **argv){
 if(argc!=2)return 2;
 int n=0;unsigned char *p=readfile(argv[1],&n,0);
 if(n!=65539){fprintf(stderr,"length %d != 65539\n",n);free(p);return 3;}
 for(int i=0;i<n;i++)if(p[i]!=(unsigned char)(i%256)){fprintf(stderr,"byte %d changed\n",i);free(p);return 4;}
 free(p);puts("binary IO: 65539 bytes preserved");return 0;
}
