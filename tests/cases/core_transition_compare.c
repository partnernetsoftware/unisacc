/* Compare the production C transition against the separately linked CX unit. */
#define CORE_TRANSITION_NAME core_transition_c
#include "../../exec/c/core.c"
#include <stdio.h>
const char *cxtn_transition(const CoreModel *,int,int,int *,int *);
static int same(CoreModel *m,int key){
 int a=77,b=77,c=88,d=88;const char *x=core_transition_c(m,0,key,&a,&b),*y=core_transition(m,0,key,&c,&d);
 return a==c&&b==d&&((!x&&!y)||(x&&y&&!strcmp(x,y)));
}
int main(void){
 int mode[]={1},count[]={3},keys0[]={0,2,4},next0[]={1,1,-1},seq0[]={1,0,1};
 int *keys[]={keys0},*next[]={next0},*seq[]={seq0},lo[]={0},hi[]={5},bn[]={0},bs[]={0},rets[]={1};
 unsigned char ret0[]={0,1,0},*retok[]={ret0};CoreModel m={0};m.ns=3;m.nq=2;m.mode=mode;m.count=count;m.keys=keys;m.next=next;m.seq=seq;m.lo=lo;m.hi=hi;m.base_next=bn;m.base_seq=bs;m.ret_seq=rets;m.ret_ok=retok;
 int tests=0,failed=0;
 for(int net=0;net<2;net++)for(int base=-2;base<4;base++)for(int seqbase=-1;seqbase<3;seqbase++)for(int key=-1;key<=6;key++){
  m.isnet=net;bn[0]=base;bs[0]=seqbase;tests++;if(!same(&m,key))failed++;
 }
 int directn[257],directs[257];for(int i=0;i<257;i++){directn[i]=(i%3)-1;directs[i]=i%2;}next[0]=directn;seq[0]=directs;m.isnet=0;mode[0]=0;
 for(int key=0;key<257;key++){tests++;if(!same(&m,key))failed++;}
 printf("CORE_TRANSITION_COMPARE cases=%d failed=%d\n",tests,failed);return failed?1:0;
}
