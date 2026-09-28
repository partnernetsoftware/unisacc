/* Standalone typed native calls into genuine script text through closures. */
#include "libraryexports.h"
#include "librarycall.h"
#include "fixture.inc"
extern unsigned char script_blob[];
typedef struct Host {void *top;int fail;} Host;
static int lookup(void *owner,const char *name,const void **raw,int *kind){
    (void)owner;for(size_t i=0;i<sizeof positions/sizeof positions[0];i++)if(!strcmp(name,positions[i].name)){*raw=script_blob+positions[i].offset;*kind=0;return 0;}return 1;
}
static int invoke(void *owner,const void *raw,const uint64_t args[6],uint64_t *result){Host*h=owner;if(h->fail)return 7;*result=us_library_bridge_raw(raw,args,h->top);return 0;}
#define GET(NAME,TYPE) ((TYPE)us_exports_symbol(&set,NAME,&host,lookup,invoke,error,sizeof error))
int main(void){
    us_exports set={0};char error[256];Host host={0};unsigned char *stack=malloc(65552);if(!stack)return 1;host.top=(void*)(((uintptr_t)stack+65536)&~(uintptr_t)15);
    if(us_exports_load(&set,metadata,sizeof metadata,error,sizeof error))return 2;
    const void *initraw=NULL;int initkind;uint64_t zeros[6]={0},ignored;us_library_signature init={0,0,{0},0};
    if(lookup(&host,"__init",&initraw,&initkind) || initkind || !us_library_call(initraw,zeros,host.top,&init,&ignored))return 19;
    typedef unsigned long(*mixedfn)(signed char,unsigned short,int,unsigned long,long*,signed char);
    typedef signed char(*s8fn)(signed char);typedef unsigned char(*u8fn)(unsigned char);
    typedef short(*s16fn)(short);typedef unsigned short(*u16fn)(unsigned short);
    typedef int(*s32fn)(int);typedef unsigned int(*u32fn)(unsigned int);
    typedef long*(*ptrfn)(long*);typedef void(*voidfn)(int*);typedef long(*onefn)(long);
    mixedfn mixed=GET("mixed",mixedfn);s8fn s8=GET("s8",s8fn);u8fn u8=GET("u8",u8fn);
    s16fn s16=GET("s16",s16fn);u16fn u16=GET("u16",u16fn);s32fn s32=GET("s32",s32fn);u32fn u32=GET("u32",u32fn);
    ptrfn identity=GET("identity",ptrfn);voidfn bump=GET("bump",voidfn);onefn recursive=GET("recursive",onefn);
    if(!mixed||!s8||!u8||!s16||!u16||!s32||!u32||!identity||!bump||!recursive)return 3;
    if((void*)mixed!=us_exports_symbol(&set,"mixed",&host,lookup,invoke,error,sizeof error))return 4;
    for(int i=0;i<100;i++) {
        long n=9;int k=42;
        if(mixed(-7,65535,-123,1UL<<40,&n,-2)!=(1UL<<40)+65412)return 5;
        if(s8(-122)!=-123||u8(255)!=0||s16(-32000)!=-32001||u16(65535)!=0||s32(-1234567)!=-1234568||u32(0xffffffffU)!=0)return 6;
        if(identity(&n)!=&n)return 7;bump(&k);if(k!=43)return 8;
        if(recursive(10)!=60)return 9;
    }
    for(size_t i=0;i<sizeof refused/sizeof refused[0];i++)if(us_exports_symbol(&set,refused[i],&host,lookup,invoke,error,sizeof error))return 10;
    host.fail=1;if(mixed(1,2,3,4,NULL,5)!=0)return 11;int k=8;bump(&k);if(k!=8)return 12;host.fail=0;if(s8(-122)!=-123)return 13;
    us_exports_clear(&set);if(set.items||set.count)return 14;
    /* Every proper prefix must reject; errors leave the destination empty. */
    for(size_t n=0;n<sizeof metadata;n++){if(!us_exports_load(&set,metadata,n,error,sizeof error)||set.items||set.count)return 15;}
    unsigned char *bad=malloc(sizeof metadata+1);if(!bad)return 16;memcpy(bad,metadata,sizeof metadata);bad[sizeof metadata]=0;
    if(!us_exports_load(&set,bad,sizeof metadata+1,error,sizeof error))return 17;
    for(size_t i=0;i<sizeof mutations/sizeof mutations[0];i++){
        memcpy(bad,metadata,sizeof metadata);bad[mutations[i].offset]=mutations[i].value;
        if(!us_exports_load(&set,bad,sizeof metadata,error,sizeof error)||set.items)return 18;
    }
    free(bad);free(stack);puts("native closure -> genuine script bridge: widths/pointer/void/recursion/100 repeat/error-zero/lifetime/malformed guards ok");return 0;
}
