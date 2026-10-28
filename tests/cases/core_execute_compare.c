/* Real decoded network, freshly run by both execution loops. */
#define UNISA_RUNTIME_LIBRARY
#include "../../exec/c/run.c"
int cx_core_run(const CoreModel *,unsigned char *,int,const char *,I,CoreResult *);
static int same(const CoreResult *a,const CoreResult *b,int x,int y){
 if(x!=y||a->out.n!=b->out.n||a->err.n!=b->err.n||a->reason_n!=b->reason_n)return 0;
 if(a->out.n&&memcmp(a->out.b,b->out.b,a->out.n))return 0;
 if(a->err.n&&memcmp(a->err.b,b->err.b,a->err.n))return 0;
 if(!a->reason&&!b->reason)return 1;if(!a->reason||!b->reason)return 0;
 return a->reason_n? !memcmp(a->reason,b->reason,a->reason_n):!strcmp(a->reason,b->reason);
}
int main(int argc,char **argv){
 if(argc!=2)return 2;int n;unsigned char *data=readfile(argv[1],&n,0);loadbytes(data,n);free(data);CoreModel m={0};core_model(&m);
 unsigned char empty[]={0},source[]="int main(void){return 7;}\n",binary[]={0,1,2,127,128,255};
 unsigned char *in[]={empty,source,binary};int lens[]={0,sizeof source-1,sizeof binary};int failures=0;
 for(int i=0;i<3;i++){
  CoreResult a={0},b={0};int x=core_run(&m,in[i],lens[i],"probe.c",200000,&a);int y=cx_core_run(&m,in[i],lens[i],"probe.c",200000,&b);
  if(!same(&a,&b,x,y)){fprintf(stderr,"mismatch case=%d c=%d cx=%d\n",i,x,y);failures++;}
  free(a.out.b);free(a.err.b);free(b.out.b);free(b.err.b);
 }
 Model owned={0};model_capture(&owned);model_release(&owned);model_clear();
 printf("CORE_EXECUTE_COMPARE cases=3 failed=%d\n",failures);return failures?1:0;
}
