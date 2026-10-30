/* Observable shape of the existing runtime decoder; host reference only. */
#define UNISA_RUNTIME_LIBRARY
#include "../../archive/reference-c/exec/c/run.c"   /* reference C; its headers stay in exec/c (-I) */
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    int n;unsigned char *data=readfile(argv[1],&n,0);loadbytes(data,n);free(data);
    printf("model ns=%d nq=%d nrg=%d nstr=%d start=%d net=%d top=%d nqa=%d\n",NS,NQ,NRG,NSTR,START,ISNET,TOPMAX,NQA);
    for(int i=0;i<NSTR;i++) { printf("str %d %d",i,STRL[i]);for(int j=0;j<STRL[i];j++)printf(" %02x",(unsigned char)STR[i][j]);putchar('\n'); }
    for(int i=0;i<NQ;i++)printf("q %d %d %d\n",i,QOFF[i],QLEN[i]);
    printf("qa");for(int i=0;i<NQA;i++)printf(" %lld",(long long)QA[i]);putchar('\n');
    for(int i=0;i<NS;i++) {
        printf("state %d mode=%d count=%d",i,SMODE[i],ROWC[i]);
        if(ISNET)printf(" lo=%d hi=%d bn=%d bq=%d",NLO[i],NHI[i],BN[i],BQ[i]);
        printf(" ret=%d\n",RETS[i]);
        if(RETOK[i]) { printf("returns %d",i);for(int j=0;j<NS;j++)if(RETOK[i][j])printf(" %d",j);putchar('\n'); }
        if(ISNET||SMODE[i]==1)for(int j=0;j<ROWC[i];j++)printf("row %d %d %d %d\n",i,ROWK[i][j],ROWN[i][j],ROWQ[i][j]);
        else {
            int dn=ROWN[i][0],dq=ROWQ[i][0];printf("direct %d %d %d\n",i,dn,dq);
            for(int j=1;j<257;j++)if(ROWN[i][j]!=dn||ROWQ[i][j]!=dq)printf("direct-entry %d %d %d %d\n",i,j,ROWN[i][j],ROWQ[i][j]);
        }
    }
    Model m;model_capture(&m);model_release(&m);model_clear();return 0;
}
