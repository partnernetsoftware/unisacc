/* Compare legacy ledger numeric grammar against the CX implementation. */
#include "../../exec/c/resourceledger.h"
int cxsf_hex_digit(int);
int cxsf_length(const char *,int *);
int cxsf_ledger_identity(const char *,char *,size_t,char *);
int cxsf_ledger_key(const char *,size_t,unsigned char *);
int main(void) {
    long checks=0,failed=0; char text[64];
    const char *cases[]={"", "0", "00", "01", "1", "2147483647", "2147483648", "999999999999999999999", "-1", "+1", " 1", "1 ", "1x", 0};
    for(int i=-1;i<=256;i++){checks++;if(rl_hexv(i)!=cxsf_hex_digit(i))failed++;}
    for(int a=0;a<256;a++)for(int b=0;b<256;b++){
        char key[2]={(char)a,(char)b};unsigned char out=0;int hi=rl_hexv(a),lo=rl_hexv(b);
        int ok=cxsf_ledger_key(key,2,&out);checks++;
        if(ok!=(hi>=0&&lo>=0)||(ok&&out!=(unsigned char)(hi*16+lo)))failed++;
    }
    for(int i=0;cases[i];i++){int a=99,b=99;int x=rl_length(cases[i],&a),y=cxsf_length(cases[i],&b);checks++;if(x!=y||a!=b)failed++;}
    for(int i=0;i<10000;i++){int a=99,b=99;snprintf(text,sizeof text,"%u",(unsigned)i*429497u);int x=rl_length(text,&a),y=cxsf_length(text,&b);checks++;if(x!=y||a!=b)failed++;}
    char line[160], stage[128], hex[65];
    int n=snprintf(line,sizeof line,"#SC2\tstage=e3:lnx/x86_64\tinput_sha256=%064d",0);
    for(int i=5;i<n;i++){unsigned char save=(unsigned char)line[i];for(int c=0;c<256;c++){
        RLLedger ledger={0};line[i]=(char)c;
        int a=rl_parse_identity(line,&ledger),b=cxsf_ledger_identity(line,stage,sizeof stage,hex);
        checks++;if(a!=b||(a&&(strcmp(ledger.stage,stage)||strcmp(ledger.input_hex,hex))))failed++;
    }line[i]=(char)save;}
    printf("LEDGER_NUMBERS_COMPARE checks=%ld failed=%ld\n",checks,failed);return failed?1:0;
}
