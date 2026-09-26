/* Actual C implementation and ASM checked against libc decimal rendering.
   Field writes must preserve attributes and every byte outside the field. */
#include "../core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
int decimal_c(char *,I);
int core_decimal(char *,I);
int field_fill_c(Buf *,I,I,I);
int core_field_fill(Buf *,I,I,I);
static int panic_expected;
static unsigned char storage[80];
void core_host_panic(const char *s) {
    assert(panic_expected && !strcmp(s,"fill past the reservation"));
    for(int i=0;i<80;i++)assert(storage[i]==0xa5);
    puts("field bounds: rejected before writing");exit(2);
}
int core_host_fetch(const unsigned char *p,int n,unsigned char **b,int *len) {
    (void)p;(void)n;(void)b;(void)len;return 0;
}
static void check(I value) {
    char a[40],b[40],want[40];memset(a,0xa5,40);memset(b,0xa5,40);
    int n=snprintf(want,sizeof want,"%lld",(long long)value);
    assert(decimal_c(a,value)==n && core_decimal(b,value)==n);
    assert(!memcmp(a,want,n) && !memcmp(b,want,n));
    for(int j=n;j<40;j++)assert((unsigned char)a[j]==0xa5 && (unsigned char)b[j]==0xa5);
    for(int width=-1;width<=23;width++) {
        unsigned char x[80],y[80],z[80];I attr[80];
        memset(x,0xa5,80);memset(y,0xa5,80);memset(z,0xa5,80);
        for(int j=0;j<80;j++)attr[j]=j*17;
        Buf ca={x,attr,80,80},cb={y,attr,80,80};
        int fail=n>width;
        if(!fail) { memset(z+13,' ',width-n);memcpy(z+13+width-n,want,n); }
        assert(field_fill_c(&ca,13,width,value)==fail);
        assert(core_field_fill(&cb,13,width,value)==fail);
        assert(!memcmp(x,z,80) && !memcmp(y,z,80));
        assert(ca.n==80 && cb.n==80 && ca.cap==80 && cb.cap==80);
        for(int j=0;j<80;j++)assert(attr[j]==j*17);
    }
}
int main(int argc,char **argv) {
    if(argc==3) {
        int k=atoi(argv[2]);assert(k>=0 && k<5);memset(storage,0xa5,80);
        Buf b={storage,0,80,80};
        I at[]={-1,81,79,INT64_MAX,1};I width[]={1,1,2,1,INT64_MAX};
        panic_expected=1;
        if(argv[1][0]=='c')field_fill_c(&b,at[k],width[k],7);
        else core_field_fill(&b,at[k],width[k],7);
        return 1;
    }
    assert(argc==1);
    I edge[]={0,1,-1,9,-9,10,-10,99,-99,100,-100,INT64_MIN,INT64_MAX};
    for(size_t i=0;i<sizeof edge/sizeof *edge;i++)check(edge[i]);
    uint64_t v=1;
    for(int i=0;i<10000;i++) { v=v*6364136223846793005ull+1442695040888963407ull;check((I)v); }
    /* Width failure wins over invalid destination and never dereferences it. */
    Buf absent={0};assert(field_fill_c(&absent,INT64_MIN,0,7)==1);
    assert(core_field_fill(&absent,INT64_MIN,0,7)==1);
    puts("decimal/field C/ASM: 10013 values, 250325 fields, exact bytes and attributes preserved");return 0;
}
