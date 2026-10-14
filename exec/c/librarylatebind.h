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
  if(z<32||z>n-at-4||nm>z-32||sig!=z-32-nm||us_late_u32(contract+at+4)!=256+i||us_late_u32(sel+12+5*i)!=256+i||sel[16+5*i]<1||sel[16+5*i]>2)return 1;
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
static void us_late_unwrap(Buf *input,const char *magic,unsigned char **fragment,size_t *fragmentn){
 if(input->n<24||memcmp(input->b,magic,8))__us_panic("library fragment: malformed envelope");
 uint64_t main=us_lr_u64(input->b+8),side=us_lr_u64(input->b+16);size_t available=(size_t)input->n-24;
 if(main>available||side!=available-main)__us_panic("library fragment: malformed envelope");
 us_late_snapshot(fragment,fragmentn,input->b+24+(size_t)main,(size_t)side);
 memmove(input->b,input->b+24,(size_t)main);input->n=(int)main;
}
typedef struct UsLateState {unsigned char *contract,*selection,*e3,*lower;size_t cn,sn,en,ln;} UsLateState;
static UsLateState us_late_detach(us_context *c){
 UsLateState old={c->late_contract,c->late_selection,c->late_e3,c->late_lower,c->late_contract_n,c->late_selection_n,c->late_e3_n,c->late_lower_n};
 c->late_contract=c->late_selection=c->late_e3=c->late_lower=NULL;c->late_contract_n=c->late_selection_n=c->late_e3_n=c->late_lower_n=0;return old;
}
static void us_late_release(UsLateState *s){free(s->contract);free(s->selection);free(s->e3);free(s->lower);memset(s,0,sizeof *s);}
static void us_late_rollback(us_context *c,UsLateState *old){
 UsLateState fresh=us_late_detach(c);us_late_release(&fresh);
 c->late_contract=old->contract;c->late_selection=old->selection;c->late_e3=old->e3;c->late_lower=old->lower;
 c->late_contract_n=old->cn;c->late_selection_n=old->sn;c->late_e3_n=old->en;c->late_lower_n=old->ln;memset(old,0,sizeof *old);
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
   memcpy(input.b,"USLPAR1\n",8);input.b[10]=0;
   rc=runroute("library-selection",&input,"binding-ownership");
   if(!rc)us_late_snapshot(&c->late_selection,&c->late_selection_n,input.b,(size_t)input.n);
  }
 }
 free(frame);cache_settle();cleanup();active=NULL;RI=NULL;NRI=0;
 free(c->late_e3);c->late_e3=NULL;c->late_e3_n=0;free(c->late_lower);c->late_lower=NULL;c->late_lower_n=0;
 if(rc){free(c->late_contract);c->late_contract=NULL;c->late_contract_n=0;free(c->late_selection);c->late_selection=NULL;c->late_selection_n=0;}
 return rc;
}
static int us_late_provider(void *context,uint32_t id,uint64_t *address){
 us_context *c=context;*address=0;size_t at=16;char name[4097];
 unsigned count=us_late_u32(c->late_contract+12);
 for(unsigned i=0;i<count;i++){
  size_t z=us_late_u32(c->late_contract+at),nm=us_late_u32(c->late_contract+at+12);
  if(us_late_u32(c->late_contract+at+4)==id){
   if(nm>=sizeof name)return 1;memcpy(name,c->late_contract+at+36,nm);name[nm]=0;
   const us_binding *x=us_resolver_find(&c->bindings,name);
   if(x&&x->address){*address=x->address;return 0;}
#if !defined(_WIN32)
   if(!us_late_policy(c,name)){
    dlerror();void *a=dlsym(RTLD_DEFAULT,name);const char *e=dlerror();if(!e&&a){*address=(uintptr_t)a;return 0;}
    for(size_t j=0;j<c->resolver.handle_count;j++){dlerror();a=dlsym(c->resolver.handles[j].handle,name);e=dlerror();if(!e&&a){*address=(uintptr_t)a;return 0;}}
   }
#endif
   snprintf(c->error,sizeof c->error,"unisacc: relocate: missing-provider symbol %u name %s",id,name);return 1;
  }
  at+=4+z;
 }
 return 1;
}
#endif
