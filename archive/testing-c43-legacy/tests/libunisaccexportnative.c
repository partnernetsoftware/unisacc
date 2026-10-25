/* Native C host ABI -> actual model-declared library exports. */
#include "libunisacc.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static const char source[]=
"#include <stdlib.h>\n"
"int state=5;\n"
"static int hidden(void){return 99;}\n"
"int bump(int n){state+=n;return state;}\n"
"long mixed(signed char a,unsigned short b,int c,unsigned long d,int *p,signed char f){return a+b+c+d+*p+f;}\n"
"signed char s8(signed char x){return x-1;}\n"
"unsigned char u8(unsigned char x){return x+1;}\n"
"short s16(short x){return x-1;}\n"
"unsigned short u16(unsigned short x){return x+1;}\n"
"int s32(int x){return x-1;}\n"
"unsigned int u32(unsigned int x){return x+1;}\n"
"unsigned long u64(unsigned long x){return x;}\n"
"void store(int *p,int n){*p=n;}\n"
"int *echo(int *p){return p;}\n"
"int die(void){exit(37);return 9;}\n"
"int diezero(void){exit(0);return 9;}\n"
"int main(int argc,char **argv){return state+argc+(argv[1][0]==97);}\n";
#define CHECK(EX) do{if(!(EX)){fprintf(stderr,"line %d failed: %s; %s\n",__LINE__,#EX,us_error(c));rc=1;goto done;}}while(0)
#define ACQUIRE(VAR,NAME) do{void *p=us_sym(c,NAME);CHECK(p);memcpy(&VAR,&p,sizeof VAR);CHECK(us_sym(c,NAME)==p);}while(0)
static int probe(const char *pkg,const char *target,int level){
    us_context *c=us_new(pkg);if(!c)return 1;int rc=0;
    CHECK(us_add_source(c,"native-exports.c",source)==0);
    CHECK(us_compile(c,target,level)==0);
    CHECK(us_sym(c,"bump")==NULL);
    CHECK(us_relocate(c)==0);
    int(*bump)(int);long(*mixed)(signed char,unsigned short,int,unsigned long,int*,signed char);
    signed char(*s8)(signed char);unsigned char(*u8)(unsigned char);
    short(*s16)(short);unsigned short(*u16)(unsigned short);
    int(*s32)(int);unsigned int(*u32)(unsigned int);unsigned long(*u64)(unsigned long);
    void(*store)(int*,int);int*(*echo)(int*);int(*die)(void),(*diezero)(void);
    ACQUIRE(bump,"bump");ACQUIRE(mixed,"mixed");ACQUIRE(s8,"s8");ACQUIRE(u8,"u8");
    ACQUIRE(s16,"s16");ACQUIRE(u16,"u16");ACQUIRE(s32,"s32");ACQUIRE(u32,"u32");ACQUIRE(u64,"u64");
    ACQUIRE(store,"store");ACQUIRE(echo,"echo");ACQUIRE(die,"die");ACQUIRE(diezero,"diezero");
    CHECK(bump(2)==7);CHECK(bump(2)==9);
    int cell=11;
    for(int i=0;i<100;i++){
        CHECK(mixed(-3,65000,-20,100000,&cell,-2)==164986);
        CHECK(s8(-122)==-123);CHECK(u8(255)==0);CHECK(s16(-32000)==-32001);CHECK(u16(65535)==0);
        CHECK(s32(-1234567)==-1234568);CHECK(u32(0xffffffffU)==0);
        CHECK(u64(UINT64_C(0xfedcba9876543210))==UINT64_C(0xfedcba9876543210));
        CHECK(echo(&cell)==&cell);
    }
    store(&cell,43);CHECK(cell==43);
    CHECK(us_sym(c,"hidden")==NULL);CHECK(us_sym(c,"absent")==NULL);
    CHECK(us_sym(c,"state")==NULL); /* Data-address API is intentionally separate. */
    int status=-1;CHECK(die()==0);CHECK(us_call_status(c,&status)==1 && status==37);CHECK(strstr(us_error(c),"37")!=NULL);
    CHECK(diezero()==0);CHECK(us_call_status(c,&status)==1 && status==0);
    CHECK(bump(1)==10);CHECK(us_call_status(c,&status)==0);
    const char *args[]={"hosted","arg"};CHECK(us_run_main(c,2,args,&status)==0);CHECK(status==13);
    CHECK(bump(1)==11);CHECK(us_sym(c,"bump")!=NULL); /* Saved pointer remains callable. */
done:us_free(c);return rc;
}
int main(int argc,char **argv){
    if(argc!=3)return 2;
    for(int level=0;level<3;level++)if(probe(argv[1],argv[2],level))return 1;
    puts("true E3 metadata/native typed exports: O0/O1/O2; initonce/state; widths/void/pointer; exit37+exit0 host survives; main preserves saved pointers: ok");return 0;
}
