/* Compare output bytes, source positions, and decimal fill to the C core. */
#include "../../exec/c/core.c"
#include <stdio.h>
void core_buffer_put(Buf *,int,I);int core_decimal(char *,I);int core_field_fill(Buf *,I,I,I);
void core_host_panic(const char *s){fprintf(stderr,"panic %s\n",s);abort();}
int main(void){
 Buf a={0},b={0};int failed=0,checks=0;
 for(int i=0;i<1025;i++){core_put(&a,i,i*7);core_buffer_put(&b,i,i*7);checks++;if(a.n!=b.n||a.cap!=b.cap||a.b[i]!=b.b[i]||a.at[i]!=b.at[i])failed++;}
 const I v[]={INT64_MIN,INT64_MIN+1,-100,-1,0,1,9,10,INT64_MAX};
 for(int i=0;i<9;i++)for(int width=0;width<23;width++){
  char c[32],d[32];int n=decimal(c,v[i]),m=core_decimal(d,v[i]);checks++;if(n!=m||memcmp(c,d,n))failed++;
  int x=field_fill(&a,10,width,v[i]),y=core_field_fill(&b,10,width,v[i]);checks++;if(x!=y||memcmp(a.b,b.b,a.n))failed++;
 }
 free(a.b);free(a.at);free(b.b);free(b.at);printf("CORE_OUTPUT_COMPARE checks=%d failed=%d\n",checks,failed);return failed?1:0;
}
