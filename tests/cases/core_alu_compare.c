/* Compare CX operations with the original C core over boundary operands. */
#include "../../exec/c/core.c"
#include <stdio.h>
int64_t core_alu32(int,int64_t,int64_t);
int64_t core_alu64(int,int64_t,int64_t,int *);
void core_host_panic(const char *s){fprintf(stderr,"panic %s\n",s);abort();}
int main(void){
 const I v[]={INT64_MIN,INT64_MIN+1,-4294967296LL,-2147483648LL,-65,-64,-33,-32,-1,0,1,2,31,32,63,64,65,2147483647LL,4294967295LL,INT64_MAX};
 const int op64[]={0,1,2,5,6,7,8,9,10,11,12,13,14,15};int cases=0,failed=0;
 for(int i=0;i<(int)(sizeof v/sizeof v[0]);i++)for(int j=0;j<(int)(sizeof v/sizeof v[0]);j++){
  for(int op=0;op<10;op++){I a=alu32(op,v[i],v[j]),b=core_alu32(op,v[i],v[j]);cases++;if(a!=b)failed++;}
  for(int k=0;k<(int)(sizeof op64/sizeof op64[0]);k++){int z=9,w=9;I a=alu64(op64[k],v[i],v[j],&z),b=core_alu64(op64[k],v[i],v[j],&w);cases++;if(a!=b||z!=w)failed++;}
 }
 printf("CORE_ALU_COMPARE cases=%d failed=%d\n",cases,failed);return failed?1:0;
}
