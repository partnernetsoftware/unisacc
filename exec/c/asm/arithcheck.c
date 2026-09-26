/* Generic word-machine contract. C reference is compiled from core.c, never
   maintained as a second test algorithm. Explicit expectations cover ISA traps
   and signed/unsigned cases separately from the differential samples. */
#include "../core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
I core_alu32(int,I,I); I core_alu64(int,I,I,int *);
I core_alu32_c(int,I,I); I core_alu64_c(int,I,I,int *);
int core_host_fetch(const unsigned char *p,int n,unsigned char **b,int *len) {
    (void)p;(void)n;(void)b;(void)len;return 0;
}
void core_host_panic(const char *s) { fprintf(stderr,"panic:%s\n",s);exit(2); }
static long checks;
static void probe(int wide,int op,I a,I b,int fixed,I expected,int flag) {
    int cflag=91,aflag=91;
    I c=wide ? core_alu64_c(op,a,b,&cflag) : core_alu32_c(op,a,b);
    I v=wide ? core_alu64(op,a,b,&aflag) : core_alu32(op,a,b);
    if(c!=v || cflag!=aflag || (fixed && (v!=expected || (wide && aflag!=flag)))) {
        fprintf(stderr,"DIFF %d op %d a=%llx b=%llx C=%llx/%d ASM=%llx/%d\n",
                wide,op,(unsigned long long)a,(unsigned long long)b,
                (unsigned long long)c,cflag,(unsigned long long)v,aflag);exit(1);
    }
    checks++;
}
static void pair(I a,I b) {
    for(int op=0;op<=9;op++)probe(0,op,a,b,0,0,0);
    for(int op=0;op<=15;op++)if(op!=3 && op!=4)probe(1,op,a,b,0,0,0);
}
int main(int argc,char **argv) {
    if(argc==3) {
        int op=atoi(argv[2]),z=0;
        if(!strcmp(argv[1],"c32"))core_alu32_c(op,7,3);
        else if(!strcmp(argv[1],"a32"))core_alu32(op,7,3);
        else if(!strcmp(argv[1],"c64"))core_alu64_c(op,7,3,&z);
        else if(!strcmp(argv[1],"a64"))core_alu64(op,7,3,&z);
        else return 1;
        return 0;
    }
    I v[]={0,1,-1,2,-2,31,32,33,63,64,65,255,256,INT32_MIN,INT32_MAX,
        4294967295LL,4294967296LL,INT64_MIN,INT64_MAX,
        (I)0x8000000100000003ull,(I)0xfedcba9876543210ull};
    int nv=sizeof(v)/sizeof(v[0]);
    for(int i=0;i<nv;i++)for(int j=0;j<nv;j++)pair(v[i],v[j]);
    uint64_t rng=71;
    for(int i=0;i<4096;i++) {
        rng=rng*6364136223846793005ull+1442695040888963407ull;I a=(I)rng;
        rng=rng*6364136223846793005ull+1442695040888963407ull;pair(a,(I)rng);
    }
    probe(0,0,INT32_MAX,1,1,INT32_MIN,0);
    probe(0,1,INT32_MIN,1,1,INT32_MAX,0);
    probe(0,2,INT32_MAX,2,1,-2,0);
    probe(0,3,INT32_MIN,-1,1,INT32_MIN,0);probe(0,4,INT32_MIN,-1,1,0,0);
    probe(0,3,7,0,1,0,0);probe(0,4,7,0,1,0,0);
    probe(0,3,-7,3,1,-2,0);probe(0,4,-7,3,1,-1,0);
    probe(0,8,1,31,1,INT32_MIN,0);probe(0,8,1,32,1,1,0);
    probe(0,9,INT32_MIN,-1,1,-1,0);
    probe(0,0,0x100000003LL,0x200000004LL,1,7,0);
    probe(1,0,INT64_MAX,1,1,INT64_MIN,0);probe(1,1,INT64_MIN,1,1,INT64_MAX,0);
    probe(1,2,INT64_MAX,2,1,-2,0);
    probe(1,10,INT64_MIN,-1,1,INT64_MIN,0);probe(1,11,INT64_MIN,-1,1,0,0);
    for(int op=10;op<=13;op++)probe(1,op,7,0,1,0,1);
    probe(1,10,-7,3,1,-2,0);probe(1,11,-7,3,1,-1,0);
    probe(1,12,-1,2,1,INT64_MAX,0);probe(1,13,-1,2,1,1,0);
    probe(1,8,1,63,1,INT64_MIN,0);probe(1,8,7,64,1,7,0);
    probe(1,9,INT64_MIN,-1,1,-1,0);probe(1,15,INT64_MIN,-1,1,1,0);
    probe(1,14,0,123,1,-1,0);
    printf("word arithmetic C/ASM: %ld checks, wrap/shift/division/flag contracts passed\n",checks);
    return 0;
}
