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
int lt_name(const char *);
int lt_type_ok(int, int, int, int, int, int, int);
int lt_supported(int, int);
int lt_count(int);
int lt_args(int, int);
int ls_data(int, int, int, unsigned long long, unsigned long long, int, unsigned long long);
int ls_sig(const char *, int);
int ls_wire_base(void);
int ls_wire_type(void);
int ls_wire_tail(int);
int ls_wire_rec(int, int, int);
int ls_wire_tag(unsigned char *, int);
int ls_wire_flags(unsigned char *, int, int, int);
int ls_bytes(int, int *);
int lw_put(unsigned char *, int, unsigned long long);
int lt_type_diff(unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long);
int lt_wire3_diff(unsigned long long, unsigned long long);
int lt_v3_diff(unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long);
int lt_member_diff(unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long);
int lt_member_v3(unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long);
int lt_sig_diff(unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long);
int lt_member_n(unsigned long long, int);
int lt_null_diff(int, int);
int lt_graph_cap(void);
int lt_graph_room(int, int, int);

typedef struct us_binding_type {uint64_t depth,base,shape,kind,width,uns;} us_binding_type;
typedef struct us_binding {char *name;uintptr_t address;unsigned kind,variadic,supported,writable;size_t count;us_binding_type result,*args;uint64_t extent;unsigned char *signature;size_t signature_length;struct us_exports *typed;void (*typed_clear)(struct us_exports*);} us_binding;
typedef struct us_bindings {us_binding *items;size_t count;} us_bindings;
static int us_binding_error(char *error,size_t capacity,const char *message){if(error&&capacity)snprintf(error,capacity,"%s",message);return 1;}
static void us_bindings_clear(us_bindings *b){if(!b)return;for(size_t i=0;i<b->count;i++){free(b->items[i].name);free(b->items[i].args);free(b->items[i].signature);if(b->items[i].typed){b->items[i].typed_clear(b->items[i].typed);free(b->items[i].typed);}}free(b->items);memset(b,0,sizeof *b);}
static int us_binding_type_valid(const us_binding_type *t,int result){
 int kind,depth,width,uns,r;
 if(!t)return 0;
 kind=t->kind>2147483647ull?7:(int)t->kind;
 depth=t->depth>2147483647ull?1:(int)t->depth;
 width=t->width>2147483647ull?3:(int)t->width;
 uns=t->uns>2147483647ull?2:(int)t->uns;
 r=lt_type_ok(1,kind,depth,width,uns,result,(int)sizeof(void*));
 if(r==4)return 0;
 return r;
}
static int us_binding_type_supported(const us_binding_type *t,int result){if(t->kind>6)return 0;return lt_supported((int)t->kind,result);}
static int us_binding_name(const char *s){int r=lt_name(s);if(r==4)return 0;return r;}
static int us_binding_unique(const us_bindings *b,const char *name){for(size_t i=0;i<b->count;i++)if(!strcmp(name,b->items[i].name))return 0;return 1;}
static int us_binding_append(us_bindings *b,us_binding *x,char *error,size_t cap){
 if(lt_count(b->count>2147483647ull?4096:(int)b->count))return us_binding_error(error,cap,"binding count capacity exceeded");
 us_binding *items=realloc(b->items,(b->count+1)*sizeof *items);if(!items)return us_binding_error(error,cap,"binding allocation failed");b->items=items;items[b->count++]=*x;return 0;
}
static int us_bindings_add_function(us_bindings *b,const char *name,uintptr_t address,
 const us_binding_type *result,const us_binding_type *args,size_t count,unsigned variadic,char *error,size_t cap){
 if(!b||!us_binding_name(name)||!address||sizeof(uintptr_t)!=8||lt_args(count>2147483647ull?1025:(int)count,variadic>2147483647u?2:(int)variadic)||(count&&!args)||!us_binding_type_valid(result,1))return us_binding_error(error,cap,"invalid native function declaration");
 if(!us_binding_unique(b,name))return us_binding_error(error,cap,"duplicate binding name");
 for(size_t i=0;i<count;i++)if(!us_binding_type_valid(args+i,0))return us_binding_error(error,cap,"invalid native parameter declaration");
 us_binding x={0};int nb;if(ls_bytes(strlen(name)>2147483646ull?2147483647:(int)strlen(name),&nb))return us_binding_error(error,cap,"binding allocation failed");size_t n=(size_t)nb;x.name=malloc(n);x.args=count?malloc(count*sizeof *args):NULL;if(!x.name||(count&&!x.args)){free(x.name);free(x.args);return us_binding_error(error,cap,"binding allocation failed");}
 memcpy(x.name,name,n);if(count)memcpy(x.args,args,count*sizeof *args);x.address=address;x.result=*result;x.count=count;x.variadic=variadic;x.supported=!variadic&&count<=6&&us_binding_type_supported(result,1);
 for(size_t i=0;i<count;i++)if(!us_binding_type_supported(args+i,0))x.supported=0;
 if(us_binding_append(b,&x,error,cap)){free(x.name);free(x.args);return 1;}return 0;
}
static int us_bindings_add_data(us_bindings *b,const char *name,uintptr_t address,
 const us_binding_type *type,uint64_t extent,unsigned writable,char *error,size_t cap){
 if(!b||!us_binding_name(name)||ls_data(1,address!=0,sizeof(uintptr_t)==8,(unsigned long long)address,extent,writable>1?2:(int)writable,0)||!us_binding_type_valid(type,0)||extent<type->width)return us_binding_error(error,cap,"invalid native data declaration");
 if(!us_binding_unique(b,name))return us_binding_error(error,cap,"duplicate binding name");
 us_binding x={0};int nb;if(ls_bytes(strlen(name)>2147483646ull?2147483647:(int)strlen(name),&nb))return us_binding_error(error,cap,"binding allocation failed");size_t n=(size_t)nb;x.name=malloc(n);if(!x.name)return us_binding_error(error,cap,"binding allocation failed");memcpy(x.name,name,n);x.kind=1;x.address=address;x.result=*type;x.extent=extent;x.writable=writable;x.supported=us_binding_type_supported(type,0);
 if(us_binding_append(b,&x,error,cap)){free(x.name);return 1;}return 0;
}
#ifdef UNISACC_LIBRARYNATIVE_H
#include "librarycallplans.h"
static void us_binding_owned_clear(us_binding *x){if(!x)return;free(x->name);free(x->args);free(x->signature);if(x->typed){us_exports_clear(x->typed);free(x->typed);}memset(x,0,sizeof *x);}
static int us_bindings_add_function_typed_capability(us_bindings *b,const char *name,uintptr_t address,const void *signature,size_t length,int bridge,char *error,size_t cap){
 if(!b||!us_binding_name(name)||!address||ls_sig(signature?(const char *)signature:0,length>2147483647ull?16:(int)length)||!us_binding_unique(b,name))return us_binding_error(error,cap,"invalid or duplicate typed function declaration");
 us_binding x={0};x.typed_clear=us_exports_clear;x.typed=calloc(1,sizeof *x.typed);if(!x.typed)return us_binding_error(error,cap,"typed declaration allocation failed");
 if(us_exports_load_capability(x.typed,signature,length,bridge,error,cap)||x.typed->count!=1)goto bad;
 us_export *f=x.typed->items;if(strcmp(name,f->name)||f->linkage||f->defined!=1)goto bad;
 {int nb;if(ls_bytes(strlen(name)>2147483646ull?2147483647:(int)strlen(name),&nb))goto bad;x.name=malloc((size_t)nb);}x.signature=malloc(length);if(!x.name||!x.signature)goto bad;
 strcpy(x.name,name);memcpy(x.signature,signature,length);x.signature_length=length;x.address=address;x.count=(size_t)f->count;x.variadic=f->variadic;x.supported=bridge && us_export_has_callbacks(f) ? us_export_bridge_supported(f):us_export_supported(f);
 if(us_binding_append(b,&x,error,cap))goto bad;return 0;
 bad:us_binding_owned_clear(&x);return us_binding_error(error,cap,"invalid typed function declaration");
}
static int us_bindings_add_function_typed(us_bindings *b,const char *name,uintptr_t addr,const void *sig,size_t n,char *e,size_t cap){return us_bindings_add_function_typed_capability(b,name,addr,sig,n,0,e,cap);}
static int us_bindings_add_function_typed_bridge(us_bindings *b,const char *name,uintptr_t addr,const void *sig,size_t n,char *e,size_t cap){return us_bindings_add_function_typed_capability(b,name,addr,sig,n,1,e,cap);}
/* Binding declarations obey the same recursive callback ABI equality as plans.
   IDs/sharing/support are not ABI; nested arguments/results/mode are. */
static int us_binding_graph_equal(const us_export_type *a,const us_export_type *b){
    us_native_type_pair *work=calloc((size_t)lt_graph_cap(),sizeof *work);
    us_native_signature_pair *seen=calloc((size_t)lt_graph_cap(),sizeof *seen);
    size_t pending=0,visited=0,steps=0;int equal=0;
    if(!work||!seen)goto done;
    work[pending++]=(us_native_type_pair){a,b};
    while(pending){
        us_native_type_pair pair=work[--pending];a=pair.a;b=pair.b;
        if(!a||!b||lt_graph_room(0,(int)++steps,0))goto done;
        if(lt_type_diff(a->depth,b->depth,a->kind,b->kind,a->width,b->width,a->uns,b->uns,a->alignment,b->alignment,a->tag,b->tag,a->nmembers,b->nmembers,a->count,b->count,a->stride,b->stride))goto done;
        if(lt_wire3_diff(a->wire_version,b->wire_version))goto done;
        if(a->wire_version==3 && lt_v3_diff(a->fp_rank,b->fp_rank,a->fp_format,b->fp_format,a->natural_alignment,b->natural_alignment,a->layout_flags,b->layout_flags,a->layout_known_mask,b->layout_known_mask,a->layout_origin,b->layout_origin))goto done;
        if(lt_null_diff(a->element==NULL,b->element==NULL)||lt_null_diff(a->signature==NULL,b->signature==NULL))goto done;
        if(lt_member_n(a->nmembers,a->members&&b->members))goto done;
        if(a->element){if(lt_graph_room(1,(int)pending,0))goto done;work[pending++]=(us_native_type_pair){a->element,b->element};}
        for(size_t i=0;i<(size_t)a->nmembers;i++){
            const us_export_member *x=a->members+i,*y=b->members+i;
            if(lt_member_diff(x->offset,y->offset,x->bit_offset,y->bit_offset,x->bit_width,y->bit_width,x->storage,y->storage)||lt_graph_room(1,(int)pending,0))goto done;
            if(a->wire_version==3 && lt_member_v3(x->ordinal,y->ordinal,x->entry_kind,y->entry_kind,x->effective_alignment,y->effective_alignment))goto done;
            work[pending++]=(us_native_type_pair){x->type,y->type};
        }
        if(a->signature){
            const us_export_signature *x=a->signature,*y=b->signature;size_t i;
            for(i=0;i<visited;i++)if(seen[i].a==x && seen[i].b==y)break;
            if(i<visited)continue;
            if(lt_graph_room(1,(int)visited,0)||lt_sig_diff(x->variadic,y->variadic,x->mode,y->mode,x->count,y->count,x->stored,y->stored)||
               (x->count && (!x->argtypes||!y->argtypes))||lt_graph_room(1,(int)pending,1+(int)x->count))goto done;
            seen[visited++]=(us_native_signature_pair){x,y};
            work[pending++]=(us_native_type_pair){&x->result,&y->result};
            for(i=0;i<(size_t)x->count;i++)work[pending++]=(us_native_type_pair){&x->argtypes[i],&y->argtypes[i]};
        }
    }
    equal=1;
done:free(work);free(seen);return equal;
}
#endif
static void us_binding_put64(unsigned char *p,uint64_t value){(void)lw_put(p,8,(unsigned long long)value);}
static void us_binding_puttype(unsigned char *p,const us_binding_type *t){us_binding_put64(p,t->depth);us_binding_put64(p+8,t->base);us_binding_put64(p+16,t->shape);us_binding_put64(p+24,t->kind);us_binding_put64(p+32,t->width);us_binding_put64(p+40,t->uns);}
/* Allocates one caller-owned byte blob. Full unsupported descriptors are kept.
   origin=0 injected, abi=0 native-system. No pointer ownership is transferred. */
static int us_bindings_serialize(const us_bindings *b,unsigned char **out,size_t *length,char *error,size_t cap){
 if(!b||!out||!length||b->count>4096||(b->count&&!b->items))return us_binding_error(error,cap,"invalid binding output");*out=NULL;*length=0;size_t total=(size_t)ls_wire_base();
 for(size_t i=0;i<b->count;i++){const us_binding *x=b->items+i;if(x->typed)return us_binding_error(error,cap,"typed bindings require V3 freeze");int rec=ls_wire_rec((int)strlen(x->name),x->kind?1:0,x->count>2147483647ull?2147483647:(int)x->count);if(rec==1||rec==4||(size_t)rec>SIZE_MAX-total-8)return us_binding_error(error,cap,"binding wire length overflow");total+=8+(size_t)rec;}
 unsigned char *p=malloc(total);if(!p)return us_binding_error(error,cap,"binding wire allocation failed");if(ls_wire_tag(p,8)){free(p);return us_binding_error(error,cap,"binding wire allocation failed");}us_binding_put64(p+8,b->count);size_t at=(size_t)ls_wire_base();
 for(size_t i=0;i<b->count;i++){
  const us_binding *x=b->items+i;size_t n=strlen(x->name);int rec=ls_wire_rec((int)n,x->kind?1:0,x->count>2147483647ull?2147483647:(int)x->count);us_binding_put64(p+at,(uint64_t)rec);at+=8;us_binding_put64(p+at,n);at+=8;memcpy(p+at,x->name,n);at+=n;
  if(ls_wire_flags(p+at,4,(int)x->kind,(int)x->variadic)){free(p);return us_binding_error(error,cap,"binding wire allocation failed");}at+=4;us_binding_put64(p+at,x->address);at+=8;us_binding_put64(p+at,x->count);at+=8;us_binding_puttype(p+at,&x->result);at+=(size_t)ls_wire_type();
  for(size_t j=0;j<x->count;j++){us_binding_puttype(p+at,x->args+j);at+=(size_t)ls_wire_type();}
  if(x->kind){us_binding_put64(p+at,x->extent);at+=8;p[at++]=(unsigned char)x->writable;}p[at++]=(unsigned char)x->supported;
 }
 if(at!=total){free(p);return us_binding_error(error,cap,"binding wire length mismatch");}*out=p;*length=total;return 0;
}
#endif
