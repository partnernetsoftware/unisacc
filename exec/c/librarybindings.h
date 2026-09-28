#ifndef UNISACC_LIBRARYBINDINGS_H
#define UNISACC_LIBRARYBINDINGS_H
/* Explicit caller declarations -> borrowed native-address registry -> USBIND1.
   This is not a C parser or an implemented import resolver. Selection, collision
   with source definitions, and native-call generation belong to the model.
   Caller keeps all injected addresses/data alive until context destruction. */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct us_binding_type {uint64_t depth,base,shape,kind,width,uns;} us_binding_type;
typedef struct us_binding {char *name;uintptr_t address;unsigned kind,variadic,supported,writable;size_t count;us_binding_type result,*args;uint64_t extent;unsigned char *signature;size_t signature_length;struct us_exports *typed;void (*typed_clear)(struct us_exports*);} us_binding;
typedef struct us_bindings {us_binding *items;size_t count;} us_bindings;
static int us_binding_error(char *error,size_t capacity,const char *message){if(error&&capacity)snprintf(error,capacity,"%s",message);return 1;}
static void us_bindings_clear(us_bindings *b){if(!b)return;for(size_t i=0;i<b->count;i++){free(b->items[i].name);free(b->items[i].args);free(b->items[i].signature);if(b->items[i].typed){b->items[i].typed_clear(b->items[i].typed);free(b->items[i].typed);}}free(b->items);memset(b,0,sizeof *b);}
static int us_binding_type_valid(const us_binding_type *t,int result){
 if(!t||t->kind>6||t->uns>1)return 0;
 if(t->kind==0)return result&&!t->depth&&!t->width&&!t->uns;
 if(t->kind==1)return !t->depth&&(t->width==1||t->width==2||t->width==4||t->width==8);
 if(t->kind==2)return t->depth>0&&t->width==sizeof(void*)&&!t->uns;
 if(t->kind==3)return !t->depth&&(t->width==4||t->width==8)&&!t->uns;
 if(t->kind==4)return t->depth>0&&t->width==sizeof(void*)&&!t->uns;
 return 1; /* Aggregate/unknown opaque facts are preserved, never called. */
}
static int us_binding_type_supported(const us_binding_type *t,int result){return t->kind==1||t->kind==2||(result&&t->kind==0);}
static int us_binding_name(const char *s){if(!s||!*s)return 0;size_t i=0;for(;s[i];i++){unsigned c=(unsigned char)s[i];int a=(c>=65&&c<=90)||(c>=97&&c<=122)||c==95;if(i>=1024||(!a&&!(i&&c>=48&&c<=57)))return 0;}return 1;}
static int us_binding_unique(const us_bindings *b,const char *name){for(size_t i=0;i<b->count;i++)if(!strcmp(name,b->items[i].name))return 0;return 1;}
static int us_binding_append(us_bindings *b,us_binding *x,char *error,size_t cap){
 if(b->count>=4096)return us_binding_error(error,cap,"binding count capacity exceeded");
 us_binding *items=realloc(b->items,(b->count+1)*sizeof *items);if(!items)return us_binding_error(error,cap,"binding allocation failed");b->items=items;items[b->count++]=*x;return 0;
}
static int us_bindings_add_function(us_bindings *b,const char *name,uintptr_t address,
 const us_binding_type *result,const us_binding_type *args,size_t count,unsigned variadic,char *error,size_t cap){
 if(!b||!us_binding_name(name)||!address||sizeof(uintptr_t)!=8||variadic>1||count>1024||(count&&!args)||!us_binding_type_valid(result,1))return us_binding_error(error,cap,"invalid native function declaration");
 if(!us_binding_unique(b,name))return us_binding_error(error,cap,"duplicate binding name");
 for(size_t i=0;i<count;i++)if(!us_binding_type_valid(args+i,0))return us_binding_error(error,cap,"invalid native parameter declaration");
 us_binding x={0};size_t n=strlen(name)+1;x.name=malloc(n);x.args=count?malloc(count*sizeof *args):NULL;if(!x.name||(count&&!x.args)){free(x.name);free(x.args);return us_binding_error(error,cap,"binding allocation failed");}
 memcpy(x.name,name,n);if(count)memcpy(x.args,args,count*sizeof *args);x.address=address;x.result=*result;x.count=count;x.variadic=variadic;x.supported=!variadic&&count<=6&&us_binding_type_supported(result,1);
 for(size_t i=0;i<count;i++)if(!us_binding_type_supported(args+i,0))x.supported=0;
 if(us_binding_append(b,&x,error,cap)){free(x.name);free(x.args);return 1;}return 0;
}
static int us_bindings_add_data(us_bindings *b,const char *name,uintptr_t address,
 const us_binding_type *type,uint64_t extent,unsigned writable,char *error,size_t cap){
 if(!b||!us_binding_name(name)||!address||sizeof(uintptr_t)!=8||!extent||extent>UINTPTR_MAX-address||writable>1||!us_binding_type_valid(type,0)||extent<type->width)return us_binding_error(error,cap,"invalid native data declaration");
 if(!us_binding_unique(b,name))return us_binding_error(error,cap,"duplicate binding name");
 us_binding x={0};size_t n=strlen(name)+1;x.name=malloc(n);if(!x.name)return us_binding_error(error,cap,"binding allocation failed");memcpy(x.name,name,n);x.kind=1;x.address=address;x.result=*type;x.extent=extent;x.writable=writable;x.supported=us_binding_type_supported(type,0);
 if(us_binding_append(b,&x,error,cap)){free(x.name);return 1;}return 0;
}
#ifdef UNISACC_LIBRARYNATIVE_H
#include "librarycallplans.h"
static void us_binding_owned_clear(us_binding *x){if(!x)return;free(x->name);free(x->args);free(x->signature);if(x->typed){us_exports_clear(x->typed);free(x->typed);}memset(x,0,sizeof *x);}
static int us_bindings_add_function_typed(us_bindings *b,const char *name,uintptr_t address,const void *signature,size_t length,char *error,size_t cap){
 if(!b||!us_binding_name(name)||!address||!signature||length<16||memcmp(signature,"USLSIG2\n",8)||!us_binding_unique(b,name))return us_binding_error(error,cap,"invalid or duplicate typed function declaration");
 us_binding x={0};x.typed_clear=us_exports_clear;x.typed=calloc(1,sizeof *x.typed);if(!x.typed)return us_binding_error(error,cap,"typed declaration allocation failed");
 if(us_exports_load(x.typed,signature,length,error,cap)||x.typed->count!=1)goto bad;
 us_export *f=x.typed->items;if(strcmp(name,f->name)||f->linkage||f->defined!=1)goto bad;
 x.name=malloc(strlen(name)+1);x.signature=malloc(length);if(!x.name||!x.signature)goto bad;
 strcpy(x.name,name);memcpy(x.signature,signature,length);x.signature_length=length;x.address=address;x.count=(size_t)f->count;x.variadic=f->variadic;x.supported=us_export_supported(f);
 if(us_binding_append(b,&x,error,cap))goto bad;return 0;
 bad:us_binding_owned_clear(&x);return us_binding_error(error,cap,"invalid typed function declaration");
}
/* Binding declarations obey the same recursive callback ABI equality as plans.
   IDs/sharing/support are not ABI; nested arguments/results/mode are. */
static int us_binding_graph_equal(const us_export_type *a,const us_export_type *b){
 return us_native_type_equal(a,b);
}
#endif
static void us_binding_put64(unsigned char *p,uint64_t value){for(unsigned i=0;i<8;i++)p[i]=(unsigned char)(value>>(8*i));}
static void us_binding_puttype(unsigned char *p,const us_binding_type *t){us_binding_put64(p,t->depth);us_binding_put64(p+8,t->base);us_binding_put64(p+16,t->shape);us_binding_put64(p+24,t->kind);us_binding_put64(p+32,t->width);us_binding_put64(p+40,t->uns);}
/* Allocates one caller-owned byte blob. Full unsupported descriptors are kept.
   origin=0 injected, abi=0 native-system. No pointer ownership is transferred. */
static int us_bindings_serialize(const us_bindings *b,unsigned char **out,size_t *length,char *error,size_t cap){
 if(!b||!out||!length||b->count>4096||(b->count&&!b->items))return us_binding_error(error,cap,"invalid binding output");*out=NULL;*length=0;size_t total=16;
 for(size_t i=0;i<b->count;i++){const us_binding *x=b->items+i;if(x->typed)return us_binding_error(error,cap,"typed bindings require V3 freeze");size_t record=strlen(x->name)+(x->kind?86:77)+48*x->count;if(record>SIZE_MAX-total-8)return us_binding_error(error,cap,"binding wire length overflow");total+=8+record;}
 unsigned char *p=malloc(total);if(!p)return us_binding_error(error,cap,"binding wire allocation failed");memcpy(p,"USBIND1\n",8);us_binding_put64(p+8,b->count);size_t at=16;
 for(size_t i=0;i<b->count;i++){
  const us_binding *x=b->items+i;size_t n=strlen(x->name),record=n+(x->kind?86:77)+48*x->count;us_binding_put64(p+at,record);at+=8;us_binding_put64(p+at,n);at+=8;memcpy(p+at,x->name,n);at+=n;
  p[at++]=(unsigned char)x->kind;p[at++]=0;p[at++]=0;p[at++]=(unsigned char)x->variadic;us_binding_put64(p+at,x->address);at+=8;us_binding_put64(p+at,x->count);at+=8;us_binding_puttype(p+at,&x->result);at+=48;
  for(size_t j=0;j<x->count;j++){us_binding_puttype(p+at,x->args+j);at+=48;}
  if(x->kind){us_binding_put64(p+at,x->extent);at+=8;p[at++]=(unsigned char)x->writable;}p[at++]=(unsigned char)x->supported;
 }
 if(at!=total){free(p);return us_binding_error(error,cap,"binding wire length mismatch");}*out=p;*length=total;return 0;
}
#endif
