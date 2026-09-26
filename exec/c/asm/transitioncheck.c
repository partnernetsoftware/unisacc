/* Independent interface probes plus differential checking of the retained
   C implementation. Test-only code; no expected answers enter the runtime. */
#include "../core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
const char *core_transition_c(const CoreModel *,int,int,int *,int *);
int core_host_fetch(const unsigned char *p,int n,unsigned char **b,int *len) {
    (void)p; (void)n; (void)b; (void)len; return 0;
}
void core_host_panic(const char *s) { fprintf(stderr,"panic %s\n",s); exit(2); }
static long checks;
static void check(CoreModel *m,int q,int key,int expect,int n,int s) {
    int cn=99,cs=99,an=99,as=99;
    const char *c=core_transition_c(m,q,key,&cn,&cs);
    const char *a=core_transition(m,q,key,&an,&as);
    if ((!!c)!=(!!a) || (c && strcmp(c,a)) || cn!=an || cs!=as ||
        (expect>=0 && ((!!a)!=expect || (!a && (an!=n || as!=s))))) {
        fprintf(stderr,"DIFF q=%d key=%d C=(%s,%d,%d) ASM=(%s,%d,%d)\n",
                q,key,c?c:"ok",cn,cs,a?a:"ok",an,as);exit(1);
    }
    checks++;
}
static unsigned rng=37;
static int random_n(int n) { rng=rng*1664525u+1013904223u; return (int)(rng%(unsigned)n); }
int main(void) {
    CoreModel m={0};int mode[4],count[4],lo[4],hi[4],bn[4],bq[4];
    int keys[4][257],next[4][257],seq[4][257];
    int *kp[4],*np[4],*sp[4];
    m.ns=4;m.nq=7;m.isnet=1;m.mode=mode;m.count=count;m.lo=lo;m.hi=hi;
    m.base_next=bn;m.base_seq=bq;m.keys=kp;m.next=np;m.seq=sp;
    for(int i=0;i<4;i++){kp[i]=keys[i];np[i]=next[i];sp[i]=seq[i];}
    for(int round=0;round<512;round++) {
        for(int i=0;i<4;i++) {
            mode[i]=random_n(3);lo[i]=mode[i]==1?-1:0;hi[i]=256;
            count[i]=random_n(9);bn[i]=random_n(5)-1;bq[i]=random_n(7);
            int n=bn[i],s=bq[i];
            for(int j=0;j<count[i];j++) {
                keys[i][j]=j*29+1;
                int nn=random_n(5)-1,ss=random_n(7);
                next[i][j]=nn-n;seq[i][j]=ss-s;n=nn;s=ss;
            }
            for(int key=-2;key<=257;key++)check(&m,i,key,-1,0,0);
            check(&m,i,INT32_MIN,1,0,0);check(&m,i,INT32_MAX,1,0,0);
        }
    }
    /* Missing next=-1 is a valid output. Domain errors preserve (-1,0). */
    count[2]=0;bn[2]=-1;bq[2]=0;lo[2]=-1;hi[2]=256;
    check(&m,2,256,0,-1,0);check(&m,2,-2,1,0,0);
    /* A 32-bit accumulator would wrap these invalid outputs into valid ones. */
    count[2]=3;bn[2]=0;bq[2]=0;
    for(int j=0;j<3;j++){keys[2][j]=j;seq[2][j]=0;}
    next[2][0]=INT32_MAX;next[2][1]=INT32_MAX;next[2][2]=2;
    check(&m,2,2,1,0,0);
    next[2][0]=INT32_MIN;next[2][1]=INT32_MIN;next[2][2]=1;
    check(&m,2,2,1,0,0);
    next[2][0]=0;next[2][1]=0;next[2][2]=0;
    seq[2][0]=INT32_MAX;seq[2][1]=INT32_MAX;seq[2][2]=2;
    check(&m,2,2,1,0,0);
    count[2]=0;bn[2]=4;bq[2]=0;check(&m,2,0,1,0,0);
    bn[2]=-2;check(&m,2,0,1,0,0);bn[2]=0;bq[2]=-1;check(&m,2,0,1,0,0);
    bq[2]=7;check(&m,2,0,1,0,0);
    /* Full signed observation range, including a threshold at INT32_MAX. */
    count[1]=1;lo[1]=-1;hi[1]=INT32_MAX;bn[1]=-1;bq[1]=0;
    keys[1][0]=INT32_MAX;next[1][0]=4;seq[1][0]=6;
    check(&m,1,INT32_MAX-1,0,-1,0);check(&m,1,INT32_MAX,0,3,6);
    m.isnet=0;
    for(int i=0;i<4;i++) {
        mode[i]=i%3;count[i]=3;
        for(int j=0;j<257;j++){keys[i][j]=j*3;next[i][j]=j%5-1;seq[i][j]=j%7;}
        if(mode[i]==1) {
            keys[i][0]=-1;keys[i][1]=12;keys[i][2]=INT32_MAX;
            for(int key=-2;key<=257;key++)check(&m,i,key,-1,0,0);
            check(&m,i,INT32_MAX,0,1,2);
            count[i]=0;check(&m,i,12,0,-1,0);
        } else for(int key=0;key<=256;key++)check(&m,i,key,0,key%5-1,key%7);
    }
    printf("transition C/ASM: %ld checks, signed/domain/output/64-bit/table boundaries passed\n",checks);
    return 0;
}
