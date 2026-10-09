#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
typedef struct { const unsigned char *name; int n; const unsigned char *data; int len; } ResourceInput;
#include "../exec/c/librarywinimports.h"
_Static_assert(sizeof(uintptr_t)==8,"64-bit host resource addresses");
static void put32(unsigned char *p,uint32_t n){for(int i=0;i<4;i++)p[i]=(unsigned char)(n>>(8*i));}
static uintptr_t resolve(void *v,const char *s){int *calls=v;(*calls)++;return s[0]=='X'?0:(uintptr_t)UINT64_C(0xfedcba9876543210);}
static unsigned char *fixture(int count,size_t *sz){
    *sz=1024+(size_t)count*32;unsigned char *p=calloc(*sz,1);assert(p);
    p[0]='M';p[1]='Z';put32(p+60,128);memcpy(p+128,"PE\0\0",4);p[148]=240;
    p[152]=0x0b;p[153]=2;put32(p+152+108,16);put32(p+152+112,512);put32(p+152+116,40);
    put32(p+512+20,count);put32(p+512+24,count);put32(p+512+28,1024);
    put32(p+512+32,1024+count*4);put32(p+512+36,1024+count*8);
    for(int i=0;i<count;i++){
        uint32_t r=1024+count*10+i*22;put32(p+1024+i*4,64);
        put32(p+1024+count*4+i*4,r);p[1024+count*8+i*2]=(unsigned char)i;p[1024+count*8+i*2+1]=(unsigned char)(i>>8);
        snprintf((char *)p+r,22,"Export%08d",i);
    }return p;
}
int main(void){
    size_t sz;unsigned char *p=fixture(1500,&sz);int calls=0;LibraryWinImports x={0,0};
    assert(!library_winimports_read(&x,p,sz,resolve,&calls));assert(x.count==1500 && calls==1500);
    for(int i=0;i<x.count;i++){
        uint64_t a=0;assert(x.rows[i].len==8 && x.rows[i].name[0]==0);
        assert(!memcmp(x.rows[i].name,"\0process/import/kernel32.dll/",sizeof("\0process/import/kernel32.dll/")-1));
        for(int j=7;j>=0;j--){a=(a<<8)|x.rows[i].data[j];}
        assert(a==UINT64_C(0xfedcba9876543210));
    }
    memset(p,0,sz);assert(!memcmp(x.rows[0].name+sizeof("\0process/import/kernel32.dll/")-1,"Export00000000",14));
    library_winimports_free(&x);assert(!x.rows && !x.count);library_winimports_free(&x);free(p);
    for(int k=0;k<7;k++){
        p=fixture(3,&sz);calls=0;
        if(k==0)put32(p+60,UINT32_MAX);
        if(k==1)put32(p+512+32,UINT32_MAX);
        if(k==2)put32(p+512+24,UINT32_MAX);
        if(k==3){p[1024+3*8]=4;}
        if(k==4)put32(p+1024+3*4,UINT32_MAX);
        if(k==5)p[1024+3*10+22]='X'; /* partial allocation then failure */
        if(k==6)put32(p+1024,0);
        assert(library_winimports_read(&x,p,sz,resolve,&calls));assert(!x.rows && !x.count);free(p);
    }
    p=fixture(3,&sz);
    for(size_t n=0;n<sz;n++){
        calls=0;int rc=library_winimports_read(&x,p,n,resolve,&calls);
        if(!rc)library_winimports_free(&x);
        assert(!x.rows && !x.count);
    }
    /* The export address may be a forwarder RVA. Always ask the OS resolver
       using the name; never return a pointer into the forwarding string. */
    put32(p+1024,520);calls=0;
    assert(!library_winimports_read(&x,p,sz,resolve,&calls) && calls==3);
    uint64_t addr=0;for(int j=7;j>=0;j--)addr=(addr<<8)|x.rows[0].data[j];
    assert(addr==UINT64_C(0xfedcba9876543210));library_winimports_free(&x);free(p);
    puts("library-winimports: 1500 owned resources, 64-bit words, 7 rejects, truncation/forwarder/release ok");return 0;
}
