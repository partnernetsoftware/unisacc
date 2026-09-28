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
/* Independent oracle: the defining net.py sum over every unit, no early exit. */
static int full_eval(const CoreModel *m,int q,int key,int *n,int *s) {
    if (m->ret_ok && m->ret_ok[q] && key>=0 && key<m->ns && m->ret_ok[q][key]) { *n=key; *s=m->ret_seq[q]; return 1; }
    if (key < m->lo[q] || key > m->hi[q]) return 0;
    long long nn=m->base_next[q], ss=m->base_seq[q];
    for (int j=0;j<m->count[q];j++) if (key>=m->keys[q][j]) { nn+=m->next[q][j]; ss+=m->seq[q][j]; }
    if (nn<-1 || nn>=m->ns || ss<0 || ss>=m->nq) return 0;
    *n=(int)nn; *s=(int)ss; return 1;
}
static void check(CoreModel *m,int q,int key,int expect,int n,int s) {
    int cn=99,cs=99,an=99,as=99;
    const char *c=core_transition_c(m,q,key,&cn,&cs);
    const char *a=core_transition(m,q,key,&an,&as);
    if (m->isnet) {
        int fn=99,fs=99,ok=full_eval(m,q,key,&fn,&fs);
        if (ok!=!a || (ok && (an!=fn || as!=fs))) {
            fprintf(stderr,"FULL q=%d key=%d full=(%d,%d,%d) ASM=(%s,%d,%d)\n",q,key,ok,fn,fs,a?a:"ok",an,as);exit(1);
        }
    }
    if ((!!c)!=(!!a) || (c && strcmp(c,a)) || cn!=an || cs!=as ||
        (expect>=0 && ((!!a)!=expect || (!a && (an!=n || as!=s))))) {
        fprintf(stderr,"DIFF q=%d key=%d C=(%s,%d,%d) ASM=(%s,%d,%d)\n",
                q,key,c?c:"ok",cn,cs,a?a:"ok",an,as);exit(1);
    }
    checks++;
}
static unsigned rng=37;
static int random_n(int n) { rng=rng*1664525u+1013904223u; return (int)(rng%(unsigned)n); }
/* High bits: an LCG's low bits cycle with a short period. */
static int random_hi(int n) { rng=rng*1664525u+1013904223u; return (int)((rng>>16)%(unsigned)n); }
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
    /* Declared returns against the original stack row function f, compiled
       the way net.py does: continuations -> return set, units for the rest. */
    {
        static unsigned char okbits[4][260]; unsigned char *okp[4]; int rsq[4];
        int saved_ns=m.ns; m.ns=200; m.ret_ok=okp; m.ret_seq=rsq;
        for(int round=0;round<256;round++) for(int i=0;i<4;i++) {
            int fn[258],fs[258];
            mode[i]=1;lo[i]=-1;hi[i]=256;rsq[i]=random_hi(7);okp[i]=okbits[i];memset(okbits[i],0,sizeof okbits[i]);
            for(int key=-1;key<=256;key++) {
                int r=random_hi(8);
                if(key>=0 && key<m.ns && r<3){okbits[i][key]=1;fn[key+1]=key;fs[key+1]=rsq[i];}
                else if(r<5){fn[key+1]=-1;fs[key+1]=0;}
                else {fn[key+1]=random_hi(m.ns+1)-1;fs[key+1]=random_hi(7);}
            }
            bn[i]=fn[0];bq[i]=fs[0];count[i]=0;
            for(int key=0,ln=fn[0],ls=fs[0];key<=256;key++) {
                if(okbits[i][key]) continue;
                if(fn[key+1]!=ln || fs[key+1]!=ls) {
                    int j=count[i]++;keys[i][j]=key;next[i][j]=fn[key+1]-ln;seq[i][j]=fs[key+1]-ls;ln=fn[key+1];ls=fs[key+1];
                }
            }
            for(int key=-1;key<=256;key++) {
                int an=99,as=99;const char *a=core_transition(&m,i,key,&an,&as);
                if(a || an!=fn[key+1] || as!=fs[key+1]) {
                    fprintf(stderr,"RET q=%d key=%d f=(%d,%d) ASM=(%s,%d,%d)\n",i,key,fn[key+1],fs[key+1],a?a:"ok",an,as);exit(1);
                }
                check(&m,i,key,-1,0,0);
            }
            check(&m,i,257,1,0,0);check(&m,i,-2,1,0,0);
        }
        /* A null per-state entry means no returns; an undeclared continuation is
           answered by the units only. */
        okp[0]=0;count[0]=0;bn[0]=-1;bq[0]=0;check(&m,0,5,0,-1,0);
        m.ret_ok=0;m.ret_seq=0;m.ns=saved_ns;
    }
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
