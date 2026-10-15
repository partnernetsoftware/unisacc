#ifndef US_LIBRARY_LATEBIND_H
#define US_LIBRARY_LATEBIND_H
#include "libraryrelocate.h"
/* Protocol-only decoder design for the 2b host adapter. No Tape scanning. */
static uint32_t us_late_u32(const unsigned char *p){uint32_t v=0;for(int i=0;i<4;i++)v|=(uint32_t)p[i]<<(8*i);return v;}
/* Must only be called with validated USLCON1 and model-produced USLSEL1. */
static int us_late_owner(const unsigned char *contract,size_t n,const unsigned char *sel,size_t sn,const char *name,unsigned *owner,unsigned *ident){
 if(!contract||n<16||memcmp(contract,"USLCON1\n",8)||!sel||sn<12||memcmp(sel,"USLSEL1\n",8))return 1;
 unsigned count=us_late_u32(contract+12);if(count>4096||us_late_u32(sel+8)!=count||sn!=12+5*(size_t)count)return 1;
 size_t at=16,nn=strlen(name);for(unsigned i=0;i<count;i++){
  if(n-at<36)return 1;size_t z=us_late_u32(contract+at),nm=us_late_u32(contract+at+12),sig=us_late_u32(contract+at+16);
  if(z<32||z>n-at-4||nm>z-32||sig!=z-32-nm||us_late_u32(contract+at+4)!=256+i||us_late_u32(sel+12+5*i)!=256+i||(sel[16+5*i]!=1&&sel[16+5*i]!=2&&sel[16+5*i]!=4))return 1;
  if(nn==nm&&!memcmp(contract+at+36,name,nn)){*owner=sel[16+5*i];*ident=256+i;return 0;}
  at+=4+z;
 }
 return 1;
}
/* Borrow descriptors; only the view's items array is owned here. */
static int us_late_legacy_view(const us_bindings *source,const unsigned char *con,size_t cn,const unsigned char *sel,size_t sn,us_bindings *view,char *err,size_t cap){
 memset(view,0,sizeof *view);if(source->count>4096)return us_binding_error(err,cap,"library selection: registry exceeds limit");
 if(source->count){view->items=calloc(source->count,sizeof *view->items);if(!view->items)return us_binding_error(err,cap,"library selection: allocation failed");}
 for(size_t i=0;i<source->count;i++){unsigned owner,id;if(us_late_owner(con,cn,sel,sn,source->items[i].name,&owner,&id)){free(view->items);memset(view,0,sizeof *view);return us_binding_error(err,cap,"library selection: missing-owner");}if(owner==2)view->items[view->count++]=source->items[i];}
 return 0;
}

static int us_late_policy(us_context *c,const char *name){for(size_t i=0;i<c->injected_only_n;i++)if(!strcmp(c->injected_only[i],name))return 1;return 0;}
static void us_late_snapshot(unsigned char **p,size_t *n,const unsigned char *b,size_t z){
 unsigned char *copy=malloc(z?z:1);if(!copy)__us_panic("library fragment: allocation failed");memcpy(copy,b,z);free(*p);*p=copy;*n=z;
}
/* Fixed physical partitions; logical categories may alias a partition. */
static void us_late_decode(us_context *c,Buf *input,const char *magic,unsigned phase){
 if(!input||!input->b||input->n<32||memcmp(input->b,magic,8)||phase>=US_LATE_PHASES)__us_panic("library fragment: malformed three-part envelope");
 uint64_t main=us_lr_u64(input->b+8),side=us_lr_u64(input->b+16),third=us_lr_u64(input->b+24);size_t available=(size_t)input->n-32;
 if(main>available||side>available-main||third!=available-main-side)__us_panic("library fragment: malformed three-part envelope");
 UsLateBuffer copies[US_LATE_PARTITIONS];copies[0].n=(size_t)side;copies[1].n=(size_t)third;
 for(unsigned i=0;i<US_LATE_PARTITIONS;i++)copies[i].bytes=NULL;
 for(unsigned i=0;i<US_LATE_PARTITIONS;i++){
  copies[i].bytes=malloc(copies[i].n?copies[i].n:1);
  if(!copies[i].bytes){for(unsigned j=0;j<US_LATE_PARTITIONS;j++)free(copies[j].bytes);__us_panic("library fragment: allocation failed");}
 }
 size_t at=32+(size_t)main;
 for(unsigned i=0;i<US_LATE_PARTITIONS;i++){memcpy(copies[i].bytes,input->b+at,copies[i].n);at+=copies[i].n;}
 us_late_clear_phase(c,phase);
 for(unsigned i=0;i<US_LATE_PARTITIONS;i++)c->late_buffers[phase*US_LATE_PARTITIONS+i]=copies[i];
 memmove(input->b,input->b+32,(size_t)main);input->n=(int)main;
}
typedef struct UsLateState {unsigned char *contract,*selection;size_t cn,sn;UsLateBuffer buffers[US_LATE_PHASES*US_LATE_PARTITIONS];} UsLateState;
static UsLateState us_late_detach(us_context *c){
 UsLateState old;memset(&old,0,sizeof old);old.contract=c->late_contract;old.selection=c->late_selection;old.cn=c->late_contract_n;old.sn=c->late_selection_n;
 c->late_contract=c->late_selection=NULL;c->late_contract_n=c->late_selection_n=0;
 for(unsigned i=0;i<US_LATE_PHASES*US_LATE_PARTITIONS;i++){old.buffers[i]=c->late_buffers[i];c->late_buffers[i].bytes=NULL;c->late_buffers[i].n=0;}return old;
}
static void us_late_release(UsLateState *s){free(s->contract);free(s->selection);for(unsigned i=0;i<US_LATE_PHASES*US_LATE_PARTITIONS;i++)free(s->buffers[i].bytes);memset(s,0,sizeof *s);}
static void us_late_rollback(us_context *c,UsLateState *old){
 UsLateState fresh=us_late_detach(c);us_late_release(&fresh);c->late_contract=old->contract;c->late_selection=old->selection;c->late_contract_n=old->cn;c->late_selection_n=old->sn;
 for(unsigned i=0;i<US_LATE_PHASES*US_LATE_PARTITIONS;i++)c->late_buffers[i]=old->buffers[i];memset(old,0,sizeof *old);
}
static int us_late_prepare(us_context *c,const char *target){
 unsigned char *frame=NULL;size_t size=0;
 if(us_contract_input(&c->bindings,&c->resolver.declarations,us_contract_target(target),&frame,&size,c->error,sizeof c->error))return 1;
 size_t at=16;unsigned count=us_late_u32(frame+12);
 for(unsigned i=0;i<count;i++){
  size_t z=us_late_u32(frame+at),nm=us_late_u32(frame+at+12);
  for(size_t j=0;j<c->injected_only_n;j++)if(strlen(c->injected_only[j])==nm&&!memcmp(frame+at+36,c->injected_only[j],nm))frame[at+11]|=2;
  at+=4+z;
 }
 active=c;volatile int rc=1;
 if(!setjmp(failure)){
  RI=NULL;NRI=0;NR=0;FILE_READ_RECORD=0;FILE_READ_COUNT=0;FILE_READ_PATHS=NULL;library_package(c);
  int contract_found=0,selection_found=0;
  for(int i=0;i<PS;i++){if(!strcmp(STAGES[i].route,"library-contract")&&!strcmp(STAGES[i].name,"contract"))contract_found=1;if(!strcmp(STAGES[i].route,"library-selection")&&!strcmp(STAGES[i].name,"ownership"))selection_found=1;}
  if(!contract_found||!selection_found)__us_panic("library selection: required model is missing");
  Buf input={0};input.b=tracked_realloc(NULL,size);memcpy(input.b,frame,size);input.n=(int)size;
  rc=runroute("library-contract",&input,"binding-contract");
  if(!rc){us_late_snapshot(&c->late_contract,&c->late_contract_n,input.b,(size_t)input.n);
   memcpy(input.b,"USLPAR1\n",8);input.b[10]=0;input.b[11]=1;
   rc=runroute("library-selection",&input,"binding-ownership");
   if(!rc)us_late_snapshot(&c->late_selection,&c->late_selection_n,input.b,(size_t)input.n);
  }
 }
 free(frame);cache_settle();cleanup();active=NULL;RI=NULL;NRI=0;
 us_late_clear_all(c);
 if(rc){free(c->late_contract);c->late_contract=NULL;c->late_contract_n=0;free(c->late_selection);c->late_selection=NULL;c->late_selection_n=0;}
 return rc;
}
static int us_late_data_error(us_context *c,const char *reason,uint32_t id,const char *name){
 snprintf(c->error,sizeof c->error,"unisacc: relocate: %s symbol %u name %s",reason,id,name);return 1;
}
/* Provider descriptors stay host-side. All six fields are compared against the
   canonical declaration; offer capability never changes model permissions. */
static int us_late_data_offer(us_context *c,const us_binding *x,const unsigned char *record,size_t nm,uint32_t id,const char *name,uint64_t *address){
 const unsigned char *t=record+36+nm;uint64_t extent=us_lr_u64(record+20),width=us_lr_u64(t+32);
 if(record[9]||record[10]||us_late_u32(record+16)!=48||us_late_u32(record+28)||record[32])return us_late_data_error(c,"data-type-mismatch",id,name);
 if(!x||x->kind!=1||x->typed||x->count||x->variadic||
    x->result.depth!=us_lr_u64(t)||x->result.base!=us_lr_u64(t+8)||x->result.shape!=us_lr_u64(t+16)||
    x->result.kind!=us_lr_u64(t+24)||x->result.width!=width||x->result.uns!=us_lr_u64(t+40))
  return us_late_data_error(c,"data-type-mismatch",id,name);
 if(!width||extent<width||x->extent<extent)return us_late_data_error(c,"data-extent-mismatch",id,name);
 if((record[11]&1)&&!x->writable)return us_late_data_error(c,"data-permission-mismatch",id,name);
 if(!x->address)return us_late_data_error(c,"missing-provider",id,name);
 *address=x->address;return 0;
}
static int us_late_function_provider(void *context,const unsigned char *record,size_t nm,uint32_t id,const char *name,uint64_t *address){
 us_context *c=context;const us_binding *x=us_resolver_find(&c->bindings,name);
 if(x&&x->address){*address=x->address;return 0;}return 2;
}
static int us_late_data_provider(void *context,const unsigned char *record,size_t nm,uint32_t id,const char *name,uint64_t *address){
 us_context *c=context;size_t sig=us_late_u32(record+16);
 if(sig!=48||record[9]||record[10]||us_late_u32(record+28)||record[32])return us_late_data_error(c,"data-type-mismatch",id,name);
 uint64_t width=us_lr_u64(record+36+nm+32),extent=us_lr_u64(record+20);
 if(!width||extent<width)return us_late_data_error(c,"data-extent-mismatch",id,name);
 const us_binding *x=us_resolver_find(&c->data_providers,name);
 if(!x){const us_binding *legacy=us_resolver_find(&c->bindings,name);if(legacy&&legacy->kind==1)x=legacy;}
 if(x)return us_late_data_offer(c,x,record,nm,id,name,address);return 2;
}
typedef int (*UsLateVerify)(void *,const unsigned char *,size_t,uint32_t,const char *,uint64_t *);
typedef struct UsLateClass {unsigned kind,wire_kind,owner,partition;const char *e3_missing,*lower_missing;int (*declare)(us_context *,const char *,const us_signature *);UsLateVerify verify;} UsLateClass;
#ifndef US_LATE_EXTRA_CLASSES
#define US_LATE_EXTRA_CLASSES(X)
#endif
#define US_LATE_CLASSES(X) \
 X(0,0,1,0,"library E3 fragment is unavailable","library lower fragment is unavailable",us_declare_injected_import,us_late_function_provider) \
 X(1,1,4,1,"library data E3 fragment is unavailable","library data lower fragment is unavailable",us_declare_injected_data,us_late_data_provider) \
 US_LATE_EXTRA_CLASSES(X)
#define US_LATE_VERIFY_DECL(k,w,o,p,e,l,d,v) static int v(void *,const unsigned char *,size_t,uint32_t,const char *,uint64_t *);
US_LATE_CLASSES(US_LATE_VERIFY_DECL)
#undef US_LATE_VERIFY_DECL
#define US_LATE_CLASS_ROW(k,w,o,p,e,l,d,v) {k,w,o,p,e,l,d,v},
static const UsLateClass us_late_classes[]={US_LATE_CLASSES(US_LATE_CLASS_ROW)};
#undef US_LATE_CLASS_ROW
static const UsLateClass *us_late_class(unsigned kind){
 for(size_t i=0;i<sizeof us_late_classes/sizeof us_late_classes[0];i++)if(us_late_classes[i].kind==kind)return &us_late_classes[i];return NULL;
}
static const UsLateClass *us_late_origin_class(us_context *c,const char *name,unsigned wire){
 for(UsLateOrigin *o=c->late_origins;o;o=o->next)if(!strcmp(o->name,name))return us_late_class(o->kind);return us_late_class(wire);
}
static const void *us_late_query(us_context *c,unsigned kind,unsigned phase,size_t *n){
 if(n)*n=0;if(!c)return NULL;const UsLateClass *d=us_late_class(kind);
 if(!d||phase>=US_LATE_PHASES||d->partition>=US_LATE_PARTITIONS){error(c,"unknown library late-binding kind or phase");return NULL;}
 UsLateBuffer *b=&c->late_buffers[phase*US_LATE_PARTITIONS+d->partition];
 if(!b->bytes){error(c,phase==US_LATE_E3?d->e3_missing:d->lower_missing);return NULL;}if(n)*n=b->n;return b->bytes;
}
static int us_late_declare_kind(us_context *c,unsigned kind,const char *name,const us_signature *sig){
 const UsLateClass *d=us_late_class(kind);if(!c||!d||!sig||!name)return error(c,"unknown library late-binding kind or invalid declaration");
 if(!d->declare||d->partition>=US_LATE_PARTITIONS)return error(c,"unknown library late-binding kind");
 UsLateOrigin *origin=NULL;
 if(kind!=d->wire_kind){origin=calloc(1,sizeof *origin);if(!origin)return error(c,"out of memory");origin->name=copy_string(name);if(!origin->name){free(origin);return error(c,"out of memory");}origin->kind=kind;}
 us_signature normalized=*sig;normalized.kind=d->wire_kind;
 int rc=d->declare(c,name,&normalized);
 if(rc){if(origin){free(origin->name);free(origin);}return rc;}
 if(origin){origin->next=c->late_origins;c->late_origins=origin;}return 0;
}
static int us_late_provider(void *context,uint32_t id,uint64_t *address){
 us_context *c=context;*address=0;size_t at=16;char name[4097];
 if(!c->late_contract||c->late_contract_n<16)return us_late_data_error(c,"malformed-contract",id,"unknown");
 unsigned count=us_late_u32(c->late_contract+12);if(count>4096)return us_late_data_error(c,"malformed-contract",id,"unknown");
 for(unsigned i=0;i<count;i++){
  if(at>c->late_contract_n||c->late_contract_n-at<36)return us_late_data_error(c,"malformed-contract",id,"unknown");
  const unsigned char *record=c->late_contract+at;size_t z=us_late_u32(record),nm=us_late_u32(record+12),sig=us_late_u32(record+16);
  if(z<32||z>c->late_contract_n-at-4||nm>z-32||sig!=z-32-nm)return us_late_data_error(c,"malformed-contract",id,"unknown");
  if(us_late_u32(record+4)==id){
   if(!nm||nm>=sizeof name)return us_late_data_error(c,"malformed-contract",id,"unknown");memcpy(name,record+36,nm);name[nm]=0;
   unsigned owner,selected_id;if(us_late_owner(c->late_contract,c->late_contract_n,c->late_selection,c->late_selection_n,name,&owner,&selected_id)||selected_id!=id)return us_late_data_error(c,"missing-owner",id,name);
   const UsLateClass *d=us_late_origin_class(c,name,record[8]);
   if(!d||d->wire_kind!=record[8]||d->owner!=owner)return us_late_data_error(c,"wrong-owner",id,name);
   int verified=d->verify(context,record,nm,id,name,address);if(verified!=2)return verified;
#if !defined(_WIN32)
   /* Process/module fallback trusts declarations, not discovered ABI/capability. */
   if(!(record[11]&2)&&!us_late_policy(c,name)){
    dlerror();void *a=dlsym(RTLD_DEFAULT,name);const char *e=dlerror();if(!e&&a){*address=(uintptr_t)a;return 0;}
    for(size_t j=0;j<c->resolver.handle_count;j++){dlerror();a=dlsym(c->resolver.handles[j].handle,name);e=dlerror();if(!e&&a){*address=(uintptr_t)a;return 0;}}
   }
#endif
   return us_late_data_error(c,"missing-provider",id,name);
  }
  at+=4+z;
 }
 return us_late_data_error(c,"missing-provider",id,"unknown");
}
#endif
