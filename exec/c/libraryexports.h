#ifndef UNISACC_LIBRARYEXPORTS_H
#define UNISACC_LIBRARYEXPORTS_H
/* Host ABI adaptation of model-produced USLSIG1 declarations. No C parser.
   Owner must quiesce all callbacks before clear/recompile/free; returned code
   pointers then become invalid. Context invocation/error/TLS/stack/init rules
   belong to the injected host invoke callback, never to this byte decoder. */
#ifdef __APPLE__
#include <ffi/ffi.h>
#else
#include <ffi.h>
#endif
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef struct us_export_type { uint64_t depth,base,shape,kind,width,uns; } us_export_type;
typedef int (*us_export_invoke)(void *owner,const void *raw,const uint64_t slots[6],uint64_t *result);
typedef int (*us_export_lookup)(void *owner,const char *name,const void **raw,int *kind);
typedef struct us_export {
    char *name; unsigned linkage,defined,variadic,supported;
    uint64_t count,stored; us_export_type result,args[8];
    ffi_cif cif; ffi_type *ffiargs[6]; ffi_closure *closure; void *code;
    const void *raw; void *owner; us_export_invoke invoke;
    int last_status;
} us_export;
typedef struct us_exports { us_export *items; size_t count; } us_exports;
static int us_export_error(char *error,size_t cap,const char *text) {
    if (error && cap) snprintf(error,cap,"%s",text);return 1;
}
static void us_exports_clear(us_exports *set) {
    if (!set) return;
    for(size_t i=0;i<set->count;i++) {
        if(set->items[i].closure) ffi_closure_free(set->items[i].closure);
        free(set->items[i].name);
    }
    free(set->items);memset(set,0,sizeof *set);
}
static int us_export_u64(const unsigned char *bytes,size_t length,size_t *at,uint64_t *v) {
    if(*at>length || length-*at<8)return 1;
    *v=0;for(unsigned i=0;i<8;i++)*v|=(uint64_t)bytes[*at+i]<<(8*i);*at+=8;return 0;
}
static int us_export_descriptor(const unsigned char *bytes,size_t length,size_t *at,us_export_type *t) {
    return us_export_u64(bytes,length,at,&t->depth)||us_export_u64(bytes,length,at,&t->base)||
        us_export_u64(bytes,length,at,&t->shape)||us_export_u64(bytes,length,at,&t->kind)||
        us_export_u64(bytes,length,at,&t->width)||us_export_u64(bytes,length,at,&t->uns)||
        t->kind>6 || t->uns>1 || (t->width!=0 && t->width!=1 && t->width!=2 && t->width!=4 && t->width!=8);
}
static ffi_type *us_export_ffitype(const us_export_type *t,int result) {
    if(t->kind==0)return result && t->width==0 && t->depth==0 ? &ffi_type_void : NULL;
    if(t->kind==2)return t->width==sizeof(void*) && t->depth>0 ? &ffi_type_pointer : NULL;
    if(t->kind!=1 || t->depth)return NULL;
    switch(t->width) {
    case 1:return t->uns ? &ffi_type_uint8 : &ffi_type_sint8;
    case 2:return t->uns ? &ffi_type_uint16 : &ffi_type_sint16;
    case 4:return t->uns ? &ffi_type_uint32 : &ffi_type_sint32;
    case 8:return t->uns ? &ffi_type_uint64 : &ffi_type_sint64;
    default:return NULL;
    }
}
static int us_export_supported(const us_export *x) {
    if(x->linkage || x->defined!=1 || !x->supported || x->variadic || x->count>6 || x->stored!=x->count || !us_export_ffitype(&x->result,1))return 0;
    for(size_t i=0;i<(size_t)x->count;i++)if(!us_export_ffitype(&x->args[i],0))return 0;
    return 1;
}
/* Decode into an empty temporary set; on failure no partially owned entries.
   Unsupported records stay represented so lookup can explain refusal. */
static int us_exports_load(us_exports *set,const void *data,size_t length,char *error,size_t cap) {
    const unsigned char *bytes=data;size_t at=8;uint64_t count;us_exports tmp={0};
    if(!set || !bytes || length<16 || memcmp(bytes,"USLSIG1\n",8) || us_export_u64(bytes,length,&at,&count) || count>8192 || count>SIZE_MAX/sizeof(us_export))goto bad;
    tmp.items=calloc(count ? (size_t)count : 1,sizeof(us_export));if(!tmp.items)return us_export_error(error,cap,"export allocation failed");tmp.count=(size_t)count;
    for(size_t i=0;i<tmp.count;i++) {
        us_export *x=&tmp.items[i];uint64_t n;
        if(us_export_u64(bytes,length,&at,&n) || !n || n>length-at || n>=SIZE_MAX || memchr(bytes+at,0,(size_t)n))goto bad;
        x->name=malloc((size_t)n+1);if(!x->name)goto bad;memcpy(x->name,bytes+at,(size_t)n);x->name[n]=0;at+=(size_t)n;
        for(size_t j=0;j<(size_t)n;j++){unsigned ch=(unsigned char)x->name[j];int letter=(ch>=65&&ch<=90)||(ch>=97&&ch<=122)||ch==95;if(!letter && !(j && ((ch>=48&&ch<=57)||ch==46||ch==36)))goto bad;}
        for(size_t j=0;j<i;j++)if(!strcmp(x->name,tmp.items[j].name))goto bad;
        if(length-at<3)goto bad;x->linkage=bytes[at++];x->defined=bytes[at++];x->variadic=bytes[at++];
        if(x->linkage>1 || x->defined!=1 || x->variadic>1 || us_export_u64(bytes,length,&at,&x->count) || us_export_descriptor(bytes,length,&at,&x->result) || us_export_u64(bytes,length,&at,&x->stored) || x->stored!=(x->count<8 ? x->count : 8))goto bad;
        for(size_t j=0;j<(size_t)x->stored;j++)if(us_export_descriptor(bytes,length,&at,&x->args[j]))goto bad;
        if(at>=length || (x->supported=bytes[at++])>1)goto bad;
        /* A positive support flag must actually describe the declared subset. */
        if(x->supported && !us_export_supported(x))goto bad;
    }
    if(at!=length)goto bad;
    us_exports_clear(set);*set=tmp;return 0;
bad:us_exports_clear(&tmp);return us_export_error(error,cap,"malformed library signature declaration");
}
static uint64_t us_export_slot(const us_export_type *t,const void *p) {
    if(t->kind==2){void *v;memcpy(&v,p,sizeof v);return (uintptr_t)v;}
#define US_SLOT(W,U,S) if(t->width==W){if(t->uns){U v;memcpy(&v,p,W);return v;}else{S v;memcpy(&v,p,W);return (uint64_t)(int64_t)v;}}
    US_SLOT(1,uint8_t,int8_t) US_SLOT(2,uint16_t,int16_t) US_SLOT(4,uint32_t,int32_t) US_SLOT(8,uint64_t,int64_t)
#undef US_SLOT
    return 0;
}
static void us_export_result(const us_export_type *t,void *result,uint64_t value) {
    if(t->kind==0)return;
    if(t->kind==2){void *p=(void*)(uintptr_t)value;memcpy(result,&p,sizeof p);return;}
    if(t->uns){ffi_arg v=(ffi_arg)value;if(t->width<8)v&=(((uint64_t)1<<(8*t->width))-1);memcpy(result,&v,sizeof v);}
    else{ffi_sarg v=t->width==1 ? (int8_t)value : t->width==2 ? (int16_t)value : t->width==4 ? (int32_t)value : (int64_t)value;memcpy(result,&v,sizeof v);}
}
static void us_export_callback(ffi_cif *cif,void *result,void **args,void *userdata) {
    (void)cif;us_export *x=userdata;uint64_t slots[6]={0},value=0;
    for(size_t i=0;i<(size_t)x->count;i++)slots[i]=us_export_slot(&x->args[i],args[i]);
    x->last_status=x->invoke(x->owner,x->raw,slots,&value);
    if(x->last_status)value=0;
    us_export_result(&x->result,result,value);
}
static void *us_exports_symbol(us_exports *set,const char *name,void *owner,
        us_export_lookup lookup,us_export_invoke invoke,char *error,size_t cap) {
    if(!set || !name || !lookup || !invoke){us_export_error(error,cap,"invalid export lookup");return NULL;}
    us_export *x=NULL;for(size_t i=0;i<set->count;i++)if(!strcmp(name,set->items[i].name)){x=&set->items[i];break;}
    if(!x || !us_export_supported(x)){us_export_error(error,cap,"missing or unsupported function export");return NULL;}
    if(x->code){if(x->owner==owner && x->invoke==invoke)return x->code;us_export_error(error,cap,"export ownership changed");return NULL;}
    int kind=-1;const void *raw=NULL;
    if(lookup(owner,name,&raw,&kind) || !raw || kind!=0){us_export_error(error,cap,"export has no code address");return NULL;}
    for(size_t i=0;i<(size_t)x->count;i++)x->ffiargs[i]=us_export_ffitype(&x->args[i],0);
    if(ffi_prep_cif(&x->cif,FFI_DEFAULT_ABI,(unsigned)x->count,us_export_ffitype(&x->result,1),x->ffiargs)!=FFI_OK){us_export_error(error,cap,"ffi signature rejected");return NULL;}
    x->owner=owner;x->invoke=invoke;x->raw=raw;
    x->closure=ffi_closure_alloc(sizeof *x->closure,&x->code);
    if(!x->closure || !x->code || ffi_prep_closure_loc(x->closure,&x->cif,us_export_callback,x,x->code)!=FFI_OK){
        if(x->closure)ffi_closure_free(x->closure);x->closure=NULL;x->code=NULL;us_export_error(error,cap,"ffi closure creation failed");return NULL;
    }
    return x->code;
}
#endif
