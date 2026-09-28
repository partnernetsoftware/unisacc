/* Generic named exports from a loaded OS module. No source/tape inspection.
   Include after ResourceInput. The owner retains keys and little-endian words
   until library_winimports_free; pointers are never represented by host long. */
#ifndef UNISACC_LIBRARY_WINIMPORTS_H
#define UNISACC_LIBRARY_WINIMPORTS_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#if defined(_WIN32) && !defined(__UNISA__)
#include <windows.h>
#endif

typedef struct { ResourceInput *rows; int count; } LibraryWinImports;
typedef uintptr_t (*LibraryWinResolve)(void *,const char *);

static void library_winimports_free(LibraryWinImports *x) {
    for(int i=0;i<x->count;i++)free((void *)x->rows[i].name);
    free(x->rows);x->rows=0;x->count=0;
}
static uint32_t library_win_u32(const unsigned char *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static int library_win_span(size_t n,uint32_t r,size_t len) {
    return (size_t)r<=n && len<=n-(size_t)r;
}
/* Parse the mapped PE directory, not the on-disk section layout. Resolution
   belongs to the OS loader; this also handles forwarded export functions. */
static int library_winimports_read(LibraryWinImports *out,
    const unsigned char *image,size_t size,LibraryWinResolve resolve,void *ctx) {
    static const unsigned char prefix[]="\0process/import/kernel32.dll/";
    LibraryWinImports x={0,0};uint32_t pe,er,es,nn,names,ords,nf,funcs;
    size_t opt,dir;uint16_t magic,opts;
    if(!out || out->rows || out->count || !resolve || !image || size<64 ||
       image[0]!='M' || image[1]!='Z')return 1;
    pe=library_win_u32(image+60);
    if(!library_win_span(size,pe,24) || memcmp(image+pe,"PE\0\0",4))return 1;
    opts=(uint16_t)(image[pe+20]|image[pe+21]<<8);opt=(size_t)pe+24;
    if(opts<2 || opt>size || opts>size-opt)return 1;
    magic=(uint16_t)(image[opt]|image[opt+1]<<8);
    if(magic==0x20b)dir=112;else if(magic==0x10b)dir=96;else return 1;
    if(opts<dir+8 || library_win_u32(image+opt+dir-4)<1)return 1;
    er=library_win_u32(image+opt+dir);es=library_win_u32(image+opt+dir+4);
    if(!er || es<40 || !library_win_span(size,er,es))return 1;
    nf=library_win_u32(image+er+20);nn=library_win_u32(image+er+24);
    funcs=library_win_u32(image+er+28);names=library_win_u32(image+er+32);
    ords=library_win_u32(image+er+36);
    size_t count=nn;
    if(!nn || nn>INT_MAX || count>SIZE_MAX/sizeof(ResourceInput) ||
       !library_win_span(size,names,(size_t)nn*4) ||
       !library_win_span(size,ords,(size_t)nn*2) ||
       !library_win_span(size,funcs,(size_t)nf*4))return 1;
    x.rows=calloc(nn,sizeof(ResourceInput));if(!x.rows)return 1;
    for(uint32_t i=0;i<nn;i++) {
        uint32_t nr=library_win_u32(image+names+(size_t)i*4);
        unsigned int ordinal=image[ords+(size_t)i*2]|image[ords+(size_t)i*2+1]<<8;
        const unsigned char *end;size_t len,total;unsigned char *key;uintptr_t addr;
        if(ordinal>=nf || !library_win_span(size,nr,1))goto fail;
        uint32_t fr=library_win_u32(image+funcs+(size_t)ordinal*4);
        if(!fr || !library_win_span(size,fr,1))goto fail;
        end=memchr(image+nr,0,size-nr);if(!end)goto fail;
        len=(size_t)(end-(image+nr));
        if(!len || len>INT_MAX-(sizeof(prefix)-1) || len>SIZE_MAX-sizeof(prefix)-8)goto fail;
        addr=resolve(ctx,(const char *)image+nr);if(!addr)goto fail;
        total=sizeof(prefix)-1+len;
        key=malloc(total+8);if(!key)goto fail;
        memcpy(key,prefix,sizeof(prefix)-1);memcpy(key+sizeof(prefix)-1,image+nr,len);
        for(int j=0;j<8;j++)key[total+j]=(unsigned char)((uint64_t)addr>>(j*8));
        x.rows[i].name=key;x.rows[i].n=(int)total;
        x.rows[i].data=key+total;x.rows[i].len=8;x.count++;
    }
    *out=x;return 0;
fail:
    library_winimports_free(&x);return 1;
}
#if defined(_WIN32) && !defined(__UNISA__)
static uintptr_t library_win_resolve(void *module,const char *name) {
    return (uintptr_t)GetProcAddress((HMODULE)module,name);
}
static int library_winimports_init(LibraryWinImports *out) {
    HMODULE module=GetModuleHandleA("kernel32.dll");const unsigned char *p=(const unsigned char *)module;
    uint32_t pe,size;
    if(!module || p[0]!='M' || p[1]!='Z')return 1;
    /* This is an already loaded, trusted OS image. Its mapped extent is the
       loader's SizeOfImage; the parser checks every export-directory access. */
    pe=library_win_u32(p+60);
    if(pe>1048576 || memcmp(p+pe,"PE\0\0",4))return 1;
    size=library_win_u32(p+pe+24+56);
    return library_winimports_read(out,p,size,library_win_resolve,module);
}
#endif
#endif
