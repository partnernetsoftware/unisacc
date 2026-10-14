#define main hidden_main
#include "../c/run.c"
#undef main
int main(int argc,char **argv){if(argc!=8)return 2; unsigned char one[8]={1};int cn,sn;unsigned char *c=readfile(argv[3],&cn,0),*s=readfile(argv[4],&sn,0);int bn;unsigned char*b=readfile(argv[6],&bn,0);ResourceInput ri[6]={{(unsigned char*)"\0library/module",15,one,8},{(unsigned char*)"\0library/latebind",17,one,8},{(unsigned char*)"\0library/selection",18,s,sn},{(unsigned char*)"\0library/symbols",16,one,8},{(unsigned char*)"\0library/contract",17,c,cn},{(unsigned char*)"\0library/bindings",17,b,bn}};if(atoi(argv[5])){RI=ri;NRI=6;}load(argv[1]);int n;unsigned char*in=readfile(argv[2],&n,0);Buf out={0};int rc=execute(in,n,"unit",&out);if(out.n)fwrite(out.b,1,out.n,stdout);return rc;}
