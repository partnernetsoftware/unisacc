/* Compare legacy ledger numeric grammar against the CX implementation. */
#include "../../exec/c/resourceledger.h"
int cxsf_hex_digit(int);
int cxsf_length(const char *,int *);
int main(void) {
    long checks=0,failed=0; char text[64];
    const char *cases[]={"", "0", "00", "01", "1", "2147483647", "2147483648", "999999999999999999999", "-1", "+1", " 1", "1 ", "1x", 0};
    for(int i=-1;i<=256;i++){checks++;if(rl_hexv(i)!=cxsf_hex_digit(i))failed++;}
    for(int i=0;cases[i];i++){int a=99,b=99;int x=rl_length(cases[i],&a),y=cxsf_length(cases[i],&b);checks++;if(x!=y||a!=b)failed++;}
    for(int i=0;i<10000;i++){int a=99,b=99;snprintf(text,sizeof text,"%u",(unsigned)i*429497u);int x=rl_length(text,&a),y=cxsf_length(text,&b);checks++;if(x!=y||a!=b)failed++;}
    printf("LEDGER_NUMBERS_COMPARE checks=%ld failed=%ld\n",checks,failed);return failed?1:0;
}
