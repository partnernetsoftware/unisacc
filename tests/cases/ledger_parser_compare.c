/* Every byte of two valid ledgers is independently mutated and truncated. */
#include <stdio.h>
#include <string.h>
#include <stddef.h>
int old_probe(const unsigned char *,size_t,char *,size_t);
int cx_probe(const unsigned char *,size_t,char *,size_t);
int main(void) {
    unsigned char input[512];char old[2048],cx[2048];long checks=0,failed=0;
    const char *rows[]={
        "#SC1\tstage=e3:lnx/x86_64/tape/O0\tinput_sha256=0000000000000000000000000000000000000000000000000000000000000000\n00\t0\t-\t0\t-\n01\t1\tempty\t0\te3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855\n",
        "#SC2\tstage=e3:lnx/x86_64/tape/O0\tinput_sha256=0000000000000000000000000000000000000000000000000000000000000000\n00\t0\t-\t0\t-\n00\t0\t-\t0\t-\n02\t1\tabc\t3\tba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\n",0};
    for(int f=0;rows[f];f++) {
        size_t n=strlen(rows[f]);memcpy(input,rows[f],n);
        checks++;if(old_probe(input,n,old,sizeof old)||cx_probe(input,n,cx,sizeof cx)||strcmp(old,cx)||old[0]!='0')failed++;
        for(size_t p=0;p<n;p++){unsigned char save=input[p];for(int c=0;c<256;c++){
            input[p]=(unsigned char)c;int a=old_probe(input,n,old,sizeof old),b=cx_probe(input,n,cx,sizeof cx);
            checks++;if(a||b||strcmp(old,cx)){if(failed<5)fprintf(stderr,"mismatch fixture=%d offset=%lu byte=%d old=%s cx=%s\n",f,(unsigned long)p,c,old,cx);failed++;}
        }input[p]=save;}
        for(size_t len=0;len<n;len++){int a=old_probe(input,len,old,sizeof old),b=cx_probe(input,len,cx,sizeof cx);checks++;if(a||b||strcmp(old,cx))failed++;}
    }
    printf("LEDGER_PARSER_COMPARE checks=%ld failed=%ld\n",checks,failed);return failed?1:0;
}
