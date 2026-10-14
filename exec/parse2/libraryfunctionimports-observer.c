#define UNISA_RUNTIME_LIBRARY
#include "../c/run.c"
static long provider_a(long x){return x+1000;}
static long provider_b(long x){return x+2000;}
static int data_object_a=101,data_object_b=202;
int main(int argc,char**argv){
 if(argc!=5&&argc!=6&&argc!=8)return 2;
 if(argc==6){void*p=!strcmp(argv[5],"A")?(void*)provider_a:(void*)provider_b;fprintf(stderr,"observation-provider %s %p\n",argv[5],p);}
 ResourceInput ri[5];int cn,bn,tn;
 unsigned char*c=readfile(argv[3],&cn,0),*b=readfile(argv[4],&bn,0),*t=readfile(argv[2],&tn,0);
 ri[0]=(ResourceInput){(const unsigned char*)"\0library/contract",17,c,cn};
 ri[1]=(ResourceInput){(const unsigned char*)"\0library/bindings",17,b,bn};RI=ri;NRI=2;
 if(argc==8){fprintf(stderr,"observation-data-provider %s %p\n",argv[5],(void*)(!strcmp(argv[5],"A")?&data_object_a:&data_object_b));int sn;unsigned char*sb=readfile(argv[6],&sn,0);static const unsigned char one[8]={1};ri[2]=(ResourceInput){(const unsigned char*)"\0library/data-contract",22,one,8};ri[3]=(ResourceInput){(const unsigned char*)"\0library/selection",18,sb,sn};ri[4]=(ResourceInput){(const unsigned char*)"\0cli/target",11,(const unsigned char*)argv[7],(int)strlen(argv[7])};NRI=5;}
 int mn;unsigned char*mb=readfile(argv[1],&mn,0);loadbytes(mb,mn);free(mb);
 Buf out={0};int rc=execute(t,tn,"typed-fixture",&out);
 if(!rc&&out.n)fwrite(out.b,1,out.n,stdout);return rc;
}
