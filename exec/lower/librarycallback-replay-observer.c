/* Observation only: use the shipped SC1/RF ledger reader and strict fetch. */
#define main hidden_main
#include "../c/run.c"
#undef main
int main(int argc,char**argv){if(argc!=4)return 2;char err[512];if(rl_load(argv[3],&RF_LEDGER,err,sizeof err)){fprintf(stderr,"%s\n",err);return 2;}RF_STRICT=1;RF_LOADED=1;load(argv[1]);int n;unsigned char*in=readfile(argv[2],&n,0);if(!in)return 2;Buf out={0};int rc=execute(in,n,"unit",&out);if(out.n)fwrite(out.b,1,out.n,stdout);return rc;}
