#define main hidden_main
#include "../c/run.c"
#undef main
int main(int argc,char **argv){if(argc<5)return 2;unsigned char one[8]={1};ResourceInput ri[6]={{(unsigned char*)"\0library/module",15,one,8},{(unsigned char*)"\0library/latebind",17,one,8},{(unsigned char*)"\0library/symbols",16,one,8},{(unsigned char*)"\0library/data-latebind",22,one,8},{(unsigned char*)"\0library/contract",17,0,0},{(unsigned char*)"\0library/selection",18,0,0}};ri[4].data=readfile(argv[3],&ri[4].len,0);ri[5].data=readfile(argv[4],&ri[5].len,0);RI=ri;NRI=6;load(argv[1]);int n;unsigned char *in=readfile(argv[2],&n,0);Buf out={0};int rc=execute(in,n,"fixture",&out);if(out.n)fwrite(out.b,1,out.n,stdout);return rc;}
