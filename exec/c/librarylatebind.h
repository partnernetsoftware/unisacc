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
  int contract_found=0,selection_found=0;c->late_typed_enabled=0;c->late_callsite_enabled=0;c->late_callback_enabled=0;
  for(int i=0;i<PS;i++){
   if(!strcmp(STAGES[i].route,"library-contract")&&!strcmp(STAGES[i].name,"contract"))contract_found=1;
   if(!strcmp(STAGES[i].route,"library-selection")){
    if(!strcmp(STAGES[i].name,"ownership"))selection_found++;
    else if(!strcmp(STAGES[i].name,"ownership-fixedtyped")){selection_found++;c->late_typed_enabled=1;}
    else if(!strcmp(STAGES[i].name,"ownership-callsite")){selection_found++;c->late_typed_enabled=1;c->late_callsite_enabled=1;}
    else if(!strcmp(STAGES[i].name,"ownership-callback")){selection_found++;c->late_typed_enabled=1;c->late_callback_enabled=1;}
    else if(!strcmp(STAGES[i].name,"ownership-callback-callsite")){selection_found++;c->late_typed_enabled=1;c->late_callsite_enabled=1;c->late_callback_enabled=1;}
    else __us_panic("library selection: unknown ownership capability");
   }
  }
  if(selection_found>1)__us_panic("library selection: duplicate ownership capability");
  if(!contract_found||!selection_found)__us_panic("library selection: required model is missing");
  Buf input={0};input.b=tracked_realloc(NULL,size);memcpy(input.b,frame,size);input.n=(int)size;
  rc=runroute("library-contract",&input,"binding-contract");
  if(!rc){us_late_snapshot(&c->late_contract,&c->late_contract_n,input.b,(size_t)input.n);
   memcpy(input.b,"USLPAR1\n",8);input.b[10]=0;input.b[11]=(c->late_typed_enabled?3:1)|(c->late_callsite_enabled?4:0)|(c->late_callback_enabled?8:0);
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
/* Fixed typed is a logical class on the existing function physical channel.
   Native plans own their closure attachment; no independent registry/owner. */
typedef struct UsLateTypedFrame {
 us_context *context;us_native_plan *plan;ffi_cif cif;ffi_type *args[6];
 ffi_closure *closure;void *code;uint64_t symbol,ordinal;size_t wire_n;unsigned char wire[];
} UsLateTypedFrame;
static void us_late_typed_release(void *value){
 UsLateTypedFrame *f=value;if(!f)return;
 us_ffi_closure_drop(&f->closure,&f->code);
 US_LATE_NOTE_RELEASE();
 free(f);
}
static void us_late_typed_callback(ffi_cif *cif,void *result,void **args,void *value){
 UsLateTypedFrame *f=value;(void)cif;
 uint64_t a[6];for(unsigned i=0;i<6;i++)memcpy(a+i,args[i],8);
 uint64_t rc=library_typed_frame_dispatch(f->context,f->plan,a[0],a[1],a[2],a[3],a[4],a[5]);memcpy(result,&rc,8);
}
static int us_late_typed_origins(us_context *c){
 if(!c->late_typed_enabled)return 0;
 size_t at=16;unsigned count=us_late_u32(c->late_contract+12);
 for(unsigned i=0;i<count;i++){
  const unsigned char *r=c->late_contract+at;size_t z=us_late_u32(r),nm=us_late_u32(r+12);
  char name[4097];if(nm>=sizeof name)return us_late_data_error(c,"malformed-contract",us_late_u32(r+4),"unknown");
  memcpy(name,r+36,nm);name[nm]=0;unsigned owner,id;
  if(us_late_owner(c->late_contract,c->late_contract_n,c->late_selection,c->late_selection_n,name,&owner,&id))return us_late_data_error(c,"missing-owner",us_late_u32(r+4),name);
  /* The model's ownership result authorizes migration, never an address. */
  if(owner==1&&r[8]==0&&(r[9]==1||(c->late_callsite_enabled&&r[9]==2)||(c->late_callback_enabled&&r[9]==3))){
   if(c->signature_version!=3){snprintf(c->error,sizeof c->error,"unisacc: compile: fixed typed requires signature version 3 symbol %u name %s",id,name);return 1;}
   UsLateOrigin *o=c->late_origins;for(;o&&strcmp(o->name,name);o=o->next){}
   static const unsigned logical_kinds[]={0,2,3,4};
   unsigned logical=logical_kinds[r[9]];
   if(o&&o->kind!=logical)return us_late_data_error(c,"duplicate-owner",id,name);
   if(!o){o=calloc(1,sizeof *o);if(!o)return error(c,"out of memory");o->name=copy_string(name);if(!o->name){free(o);return error(c,"out of memory");}o->kind=logical;o->next=c->late_origins;c->late_origins=o;}
  }
  at+=4+z;
 }
 return 0;
}
static int us_late_typed_prepare(us_context *c,const unsigned char *record,size_t nm,uint32_t id,const char *name,us_native_plans *pending){
 const unsigned char *wire=record+36+nm;size_t wn=us_late_u32(record+16);us_carrier_certificate cert={0};uint64_t handle=0;
 int rc=library_carrier_model(c,c->target,wire,wn,&cert);
 if(rc){us_carrier_certificate_clear(&cert);return us_late_data_error(c,"not covered: typed-native-ABI",id,name);}
 rc=us_carrier_certificate_native_prepare(pending,&cert,&handle,c->error,sizeof c->error);us_carrier_certificate_clear(&cert);
 if(rc||!handle)return us_late_data_error(c,"typed-signature-unsupported",id,name);
 us_native_plan *plan=us_native_plan_find(pending,handle);
 if(plan->target)return us_late_data_error(c,"typed-plan-premature-address",id,name);
 UsLateTypedFrame *f=calloc(1,sizeof *f+wn);if(!f)return error(c,"out of memory");
 f->context=c;f->plan=plan;f->wire_n=wn;memcpy(f->wire,wire,wn);plan->attachment=f;plan->release_attachment=us_late_typed_release;return 0;
}
static int us_late_typed_provider(void *context,const unsigned char *record,size_t nm,uint32_t id,const char *name,uint64_t *address){
 us_context *c=context;const unsigned char *wire=record+36+nm;size_t wn=us_late_u32(record+16);
 if(record[8]||record[9]!=1||record[10]!=3||wn<16||memcmp(wire,"USLSIG3\n",8))return us_late_data_error(c,"typed-signature-mismatch",id,name);
 if(us_resolver_find(&c->data_providers,name))return us_late_data_error(c,"typed-provider-kind-mismatch",id,name);
 const us_binding *x=us_resolver_find(&c->bindings,name);uintptr_t raw=0;
 if(x){if(x->kind||!x->typed||x->variadic||x->signature_length!=wn||memcmp(x->signature,wire,wn))return us_late_data_error(c,"typed-signature-mismatch",id,name);raw=x->address;}
#if !defined(_WIN32)
 else if(!(record[11]&2)&&!us_late_policy(c,name)){
  /* Fallback trusts explicit declaration; it is not independent ABI evidence. */
  dlerror();void *a=dlsym(RTLD_DEFAULT,name);const char *e=dlerror();if(!e&&a)raw=(uintptr_t)a;
  for(size_t j=0;!raw&&j<c->resolver.handle_count;j++){dlerror();a=dlsym(c->resolver.handles[j].handle,name);e=dlerror();if(!e&&a)raw=(uintptr_t)a;}
 }
#endif
 if(!raw)return us_late_data_error(c,"missing-provider",id,name);
 if(!c->late_preparing)return us_late_data_error(c,"typed-plan-owner-unavailable",id,name);
 us_native_plan *plan=NULL;UsLateTypedFrame *f=NULL;
 for(us_native_plan *n=c->late_preparing->head;n;n=n->next)if(n->release_attachment==us_late_typed_release){UsLateTypedFrame *a=n->attachment;if(a->wire_n==wn&&!memcmp(a->wire,wire,wn)){plan=n;f=a;break;}}
 if(!plan||!f)return us_late_data_error(c,"typed-plan-not-prepared",id,name);
 if(f->code){if(plan->target!=raw)return us_late_data_error(c,"typed-provider-changed",id,name);*address=(uintptr_t)f->code;return 0;}
 plan->target=raw;
 for(unsigned i=0;i<6;i++)f->args[i]=&ffi_type_uint64;
 if(ffi_prep_cif(&f->cif,FFI_DEFAULT_ABI,6,&ffi_type_uint64,f->args)!=FFI_OK||us_ffi_closure_make(&f->cif,us_late_typed_callback,f,&f->closure,&f->code))return us_late_data_error(c,"typed-frame-closure-failed",id,name);
 *address=(uintptr_t)f->code;return 0;
}
/* Callsite category shares the pending plan owner and closure component.
   The symbol contract and per-callsite concrete signature remain separate. */
static void us_late_callsite_release(void *value){us_late_typed_release(value);}
static void us_late_callsite_callback(ffi_cif *cif,void *result,void **args,void *value){
 UsLateTypedFrame *f=value;(void)cif;uint64_t a[6];for(unsigned i=0;i<6;i++)memcpy(a+i,args[i],8);
 uint64_t rc=library_callsite_frame_dispatch(f->context,f->symbol,a[0],a[1],a[2],a[3],a[4],a[5]);memcpy(result,&rc,8);
}
static int us_late_callsite_prepare(us_context *c,const unsigned char *record,size_t nm,uint32_t id,const char *name,us_native_plans *pending){
 if(!c->late_callsites)return 0;
 if(c->late_callsites_n<16||memcmp(c->late_callsites,"USLCSF1\n",8))return us_late_data_error(c,"missing-callsite",id,name);
 size_t at=16;uint64_t count=us_lr_u64(c->late_callsites+8);if(count>8192)return us_late_data_error(c,"callsite-malformed-frame",id,name);
 for(uint64_t i=0;i<count;i++){
  if(at>c->late_callsites_n||c->late_callsites_n-at<40)return us_late_data_error(c,"callsite-malformed-frame",id,name);
  const unsigned char *r=c->late_callsites+at;uint64_t z=us_lr_u64(r),ordinal=us_lr_u64(r+8),symbol=us_lr_u64(r+16),fixed=us_lr_u64(r+24),wn=us_lr_u64(r+32);
  if(z<32||z>c->late_callsites_n-at-8||wn!=z-32||!ordinal)return us_late_data_error(c,"callsite-malformed-frame",id,name);
  if(symbol==id){
   unsigned char *canon=malloc(wn?wn:1);if(!canon)return error(c,"out of memory");memcpy(canon,r+40,wn);
   size_t q=8;uint64_t records=0,nn=0,argc=0;
   if(us_export_u64(canon,wn,&q,&records)||records!=1||us_export_u64(canon,wn,&q,&nn)||nn>wn-q||wn-q-nn<12){free(canon);return us_late_data_error(c,"callsite-signature-mismatch",id,name);}
   size_t mode=q+(size_t)nn+3,ca=mode+1;
   if(us_export_u64(canon,wn,&ca,&argc)){free(canon);return us_late_data_error(c,"callsite-signature-mismatch",id,name);}
   canon[mode]=argc>6?1:0;
   us_carrier_certificate cert={0};uint64_t handle=0;int rc=library_carrier_model(c,c->target,canon,wn,&cert);free(canon);
   if(rc){us_carrier_certificate_clear(&cert);return us_late_data_error(c,"not covered: callsite-native-ABI",id,name);}
   rc=us_carrier_certificate_native_prepare_mode(pending,&cert,1,fixed,&handle,c->error,sizeof c->error);us_carrier_certificate_clear(&cert);
   if(rc||!handle)return us_late_data_error(c,"callsite-signature-unsupported",id,name);
   us_native_plan *plan=us_native_plan_find(pending,handle);UsLateTypedFrame *f=calloc(1,sizeof *f+(size_t)wn);if(!f)return error(c,"out of memory");
   f->context=c;f->plan=plan;f->symbol=id;f->ordinal=ordinal;f->wire_n=wn;memcpy(f->wire,r+40,wn);plan->attachment=f;plan->release_attachment=us_late_callsite_release;
  }
  at+=8+(size_t)z;
 }
 if(at!=c->late_callsites_n)return us_late_data_error(c,"callsite-malformed-frame",id,name);return 0;
}
static int us_late_callsite_provider(void *context,const unsigned char *record,size_t nm,uint32_t id,const char *name,uint64_t *address){
 us_context *c=context;const unsigned char *wire=record+36+nm;size_t wn=us_late_u32(record+16);uintptr_t raw=0;
 if(record[8]||record[9]!=2||record[10]!=3)return us_late_data_error(c,"callsite-signature-mismatch",id,name);
 if(us_resolver_find(&c->data_providers,name))return us_late_data_error(c,"callsite-provider-kind-mismatch",id,name);
 const us_binding *x=us_resolver_find(&c->bindings,name);
 if(x){if(x->kind||!x->typed||!x->variadic||x->signature_length!=wn||memcmp(x->signature,wire,wn))return us_late_data_error(c,"callsite-signature-mismatch",id,name);raw=x->address;}
#if !defined(_WIN32)
 else if(!(record[11]&2)&&!us_late_policy(c,name)){
  dlerror();void *a=dlsym(RTLD_DEFAULT,name);const char *e=dlerror();if(!e&&a)raw=(uintptr_t)a;
  for(size_t j=0;!raw&&j<c->resolver.handle_count;j++){dlerror();a=dlsym(c->resolver.handles[j].handle,name);e=dlerror();if(!e&&a)raw=(uintptr_t)a;}
 }
#endif
 if(!raw)return us_late_data_error(c,"missing-provider",id,name);
 if(!c->late_preparing)return us_late_data_error(c,"callsite-plan-owner-unavailable",id,name);
 *address=0;
 for(us_native_plan *p=c->late_preparing->head;p;p=p->next)if(p->release_attachment==us_late_callsite_release){
  UsLateTypedFrame *f=p->attachment;if(f->symbol!=id)continue;
  p->target=raw;for(unsigned j=0;j<6;j++)f->args[j]=&ffi_type_uint64;
  if(ffi_prep_cif(&f->cif,FFI_DEFAULT_ABI,6,&ffi_type_uint64,f->args)!=FFI_OK||us_ffi_closure_make(&f->cif,us_late_callsite_callback,f,&f->closure,&f->code))return us_late_data_error(c,"callsite-frame-closure-failed",id,name);
  if(!*address)*address=(uintptr_t)f->code;
 }
 if(!*address)return us_late_data_error(c,"callsite-plan-not-prepared",id,name);return 0;
}
#if defined(US_LATE_TEST_OBSERVERS)
API const void *us_private_native_plan_bytes(us_context *c,size_t *n){if(n)*n=0;if(!c)return NULL;size_t z=0;for(us_native_plan *p=c->native_plans.head;p;p=p->next)z+=sizeof *p;if(n)*n=z;return z?c->native_plans.head:NULL;}
API const void *us_private_typed_plan_bytes(us_context *c,size_t *n){if(n)*n=0;if(!c)return NULL;for(us_native_plan *p=c->native_plans.head;p;p=p->next)if(p->release_attachment==us_late_typed_release||p->release_attachment==us_late_callsite_release){UsLateTypedFrame *f=p->attachment;if(n)*n=f->wire_n;return f->wire;}return NULL;}
API const void *us_private_callsite_frame_bytes(us_context *c,unsigned index,size_t *n){
 if(n)*n=0;if(!c||!c->late_callsites||c->late_callsites_n<16)return NULL;size_t at=16;uint64_t count=us_lr_u64(c->late_callsites+8);
 for(uint64_t i=0;i<count;i++){if(at>c->late_callsites_n||c->late_callsites_n-at<40)return NULL;uint64_t z=us_lr_u64(c->late_callsites+at),wn=us_lr_u64(c->late_callsites+at+32);if(z<32||z>c->late_callsites_n-at-8||wn!=z-32)return NULL;if(i==index){if(n)*n=wn;return c->late_callsites+at+40;}at+=8+(size_t)z;}return NULL;
}
API size_t us_private_typed_release_count(void){return us_late_typed_release_count;}
#endif
/* Callback logical class: address-free graph preparation only.
   No execution bridge or canonical identity claim is introduced by this draft. */
#if defined(US_LATE_TEST_OBSERVERS) && US_LATE_TEST_OBSERVERS
static int us_late_test_callback_carrier_status; /* default 0, test build only */
#endif
static int us_late_callback_prepare(us_context *c,const unsigned char *record,size_t nm,uint32_t id,const char *name,us_native_plans *pending){
 const unsigned char *wire=record+36+nm;size_t wn=us_late_u32(record+16);
 if(record[8]||record[9]!=3||record[10]!=3||wn<16||memcmp(wire,"USLSIG3\n",8))
  return us_late_data_error(c,"callback-signature-mismatch",id,name);
 us_carrier_certificate cert={0};uint64_t handle=0;
 int rc=library_carrier_model(c,c->target,wire,wn,&cert);
#if defined(US_LATE_TEST_OBSERVERS) && US_LATE_TEST_OBSERVERS
 /* Never replace the production model: inject only after a real certificate. */
 if(!rc&&us_late_test_callback_carrier_status){rc=us_late_test_callback_carrier_status;snprintf(c->error,sizeof c->error,"test callback certificate mechanism failure");}
#endif
 if(rc){
  us_carrier_certificate_clear(&cert);
  if(rc==2)return us_late_data_error(c,"not covered: callback-native-ABI",id,name);
  if(rc==1)return 1; /* Preserve the certificate/mechanism owner's named error. */
  return us_late_data_error(c,"internal callback-native-ABI status",id,name);
 }
 rc=us_carrier_certificate_native_prepare_mode(pending,&cert,US_NATIVE_PREPARE_CALLBACK_GRAPH,0,&handle,c->error,sizeof c->error);
 us_carrier_certificate_clear(&cert);
 if(rc||!handle)return us_late_data_error(c,"callback-signature-unsupported",id,name);
 us_native_plan *plan=us_native_plan_find(pending,handle);
 if(!plan||plan->target||!plan->attachment||plan->release_attachment!=us_callback_graph_release)
  return us_late_data_error(c,"callback-plan-not-prepared",id,name);
 return 0;
}
static int us_late_callback_provider(void *context,const unsigned char *record,size_t nm,uint32_t id,const char *name,uint64_t *address){
 us_context *c=context;(void)record;(void)nm;if(address)*address=0;
 /* Deliberately no publication: canonical identity/provider qualification and
    a callable frame address are a later wiring step, not this prepare draft. */
 return us_late_data_error(c,"not covered: callback-provider-bridge",id,name);
}
typedef int (*UsLateVerify)(void *,const unsigned char *,size_t,uint32_t,const char *,uint64_t *);
typedef int (*UsLatePrepare)(us_context *,const unsigned char *,size_t,uint32_t,const char *,us_native_plans *);
typedef struct UsLateClass {unsigned kind,wire_kind,owner,partition;const char *e3_missing,*lower_missing;int (*declare)(us_context *,const char *,const us_signature *);UsLateVerify verify;UsLatePrepare prepare;} UsLateClass;
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
#define US_LATE_CLASS_ROW(k,w,o,p,e,l,d,v) {k,w,o,p,e,l,d,v,NULL},
static const UsLateClass us_late_classes[]={US_LATE_CLASSES(US_LATE_CLASS_ROW)
 {2,0,1,0,"library E3 fragment is unavailable","library lower fragment is unavailable",NULL,us_late_typed_provider,us_late_typed_prepare},
 {3,0,1,0,"library E3 fragment is unavailable","library lower fragment is unavailable",NULL,us_late_callsite_provider,us_late_callsite_prepare},
 {4,0,1,0,"library E3 fragment is unavailable","library lower fragment is unavailable",NULL,us_late_callback_provider,us_late_callback_prepare}};
#undef US_LATE_CLASS_ROW
static const UsLateClass *us_late_class(unsigned kind){
 for(size_t i=0;i<sizeof us_late_classes/sizeof us_late_classes[0];i++)if(us_late_classes[i].kind==kind)return &us_late_classes[i];return NULL;
}
static const UsLateClass *us_late_origin_class(us_context *c,const char *name,unsigned wire){
 for(UsLateOrigin *o=c->late_origins;o;o=o->next)if(!strcmp(o->name,name))return us_late_class(o->kind);return us_late_class(wire);
}
/* One binding preparation loop; semantic preparation belongs to each class. */
static int us_late_prepare_bindings(us_context *c,us_native_plans *pending){
 if(!c->late_contract)return 0;
 size_t at=16;unsigned count=us_late_u32(c->late_contract+12);
 for(unsigned i=0;i<count;i++){
  const unsigned char *r=c->late_contract+at;size_t z=us_late_u32(r),nm=us_late_u32(r+12);char name[4097];
  if(nm>=sizeof name)return us_late_data_error(c,"malformed-contract",us_late_u32(r+4),"unknown");
  memcpy(name,r+36,nm);name[nm]=0;unsigned owner,id;
  if(us_late_owner(c->late_contract,c->late_contract_n,c->late_selection,c->late_selection_n,name,&owner,&id))return us_late_data_error(c,"missing-owner",us_late_u32(r+4),name);
  const UsLateClass *d=us_late_origin_class(c,name,r[8]);
  if(owner!=2){
   if(!d||d->wire_kind!=r[8]||d->owner!=owner)return us_late_data_error(c,"wrong-owner",id,name);
   if(d->prepare&&d->prepare(c,r,nm,id,name,pending))return 1;
  }
  at+=4+z;
 }
 return 0;
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
