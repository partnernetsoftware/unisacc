/* Compare the real legacy parser and the CX parser over byte mutations. */
#define UNISA_RUNTIME_LIBRARY
#include "../../exec/c/run.c"
int cxsf_companion(const unsigned char *,int,char *,size_t,char *);
int main(void) {
    unsigned char line[128]; char old_stage[128],new_stage[128],old_hex[65],new_hex[65];
    int n=snprintf((char *)line,sizeof line,"#SC2\tstage=e3:lnx/x86_64\tinput_sha256=%064d\n",0);
    long checks=0,failed=0;
    for (int pos=0;pos<n;pos++) {
        unsigned char saved=line[pos];
        for (int c=0;c<256;c++) {
            line[pos]=(unsigned char)c;
            int a=rf_parse_e3_identity_bytes(line,n,old_stage,sizeof old_stage,old_hex);
            int b=cxsf_companion(line,n,new_stage,sizeof new_stage,new_hex);
            checks++;
            if (a!=b || (a && (strcmp(old_stage,new_stage)||strcmp(old_hex,new_hex)))) failed++;
        }
        line[pos]=saved;
    }
    for (int len=0;len<n;len++) {
        int a=rf_parse_e3_identity_bytes(line,len,old_stage,sizeof old_stage,old_hex);
        int b=cxsf_companion(line,len,new_stage,sizeof new_stage,new_hex);
        checks++; if (a!=b) failed++;
    }
    printf("SOURCE_COMPANION_COMPARE checks=%ld failed=%ld\n",checks,failed);
    return failed?1:0;
}
