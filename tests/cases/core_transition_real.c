/* Exhaust the ordinary observation alphabet and each threshold boundary. */
#define UNISA_RUNTIME_LIBRARY
#define CORE_TRANSITION_NAME core_transition_c
#include "../../exec/c/run.c"
const char *cxtn_transition(const CoreModel *,int,int,int *,int *);
static long long checked,failed;
static void compare(const CoreModel *m,int q,int key){
 int a=77,b=77,c=88,d=88;const char *x=core_transition_c(m,q,key,&a,&b),*y=core_transition(m,q,key,&c,&d);checked++;
 if(a!=c||b!=d||!((!x&&!y)||(x&&y&&!strcmp(x,y)))){if(failed++<10)fprintf(stderr,"mismatch q=%d key=%d\n",q,key);}
}
int main(int argc,char **argv){
 if(argc!=2)return 2;int n;unsigned char *data=readfile(argv[1],&n,0);loadbytes(data,n);free(data);CoreModel m={0};core_model(&m);
 for(int q=0;q<m.ns;q++){
  for(int key=0;key<=256;key++)compare(&m,q,key);
  if(m.isnet){
   compare(&m,q,m.lo[q]);compare(&m,q,m.hi[q]);
   if(m.lo[q]>INT32_MIN)compare(&m,q,m.lo[q]-1);
   if(m.hi[q]<INT32_MAX)compare(&m,q,m.hi[q]+1);
   for(int j=0;j<m.count[q];j++){int key=m.keys[q][j];compare(&m,q,key);if(key>INT32_MIN)compare(&m,q,key-1);if(key<INT32_MAX)compare(&m,q,key+1);}
  }
  if(m.ret_ok&&m.ret_ok[q])for(int k=0;k<m.ns;k++)if(m.ret_ok[q][k])compare(&m,q,k);
 }
 Model owned={0};model_capture(&owned);model_release(&owned);model_clear();
 printf("CORE_TRANSITION_REAL checks=%lld failed=%lld\n",checked,failed);return failed?1:0;
}
