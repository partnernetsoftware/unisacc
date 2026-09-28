#ifndef UNISACC_LIBRARYRESOLVER_H
#define UNISACC_LIBRARYRESOLVER_H
/* Mechanical typed declarations + all native lookup candidates. No source
   parsing, priority selection or semantic filtering is performed here.
   Root owns invalidation/active guards and destroys images before clear(). */
#include "librarybindings.h"
#include <limits.h>
#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#else
#include <dlfcn.h>
#endif
#define US_RESOLVER_HANDLES 64
#define US_RESOLVER_NAMES 1024
#define US_RESOLVER_CANDIDATES 65536
typedef struct us_resolver_handle {char *path;void *handle;} us_resolver_handle;
typedef struct us_resolver {us_bindings declarations;us_resolver_handle handles[US_RESOLVER_HANDLES];size_t handle_count;uint64_t generation;} us_resolver;
static const us_binding *us_resolver_find(const us_bindings *b,const char *name){if(b)for(size_t i=0;i<b->count;i++)if(!strcmp(b->items[i].name,name))return b->items+i;return NULL;}
static int us_resolver_type_equal(const us_binding_type*a,const us_binding_type*b){return a->depth==b->depth&&a->kind==b->kind&&a->width==b->width&&a->uns==b->uns;}
static int us_resolver_declaration_equal(const us_binding*a,const us_binding*b){
 if(a->kind!=b->kind||a->count!=b->count||a->variadic!=b->variadic)return 0;
 if(a->typed||b->typed){
#ifdef UNISACC_LIBRARYNATIVE_H
  if(!a->typed||!b->typed)return 0;us_export *x=a->typed->items,*y=b->typed->items;
  if(!us_binding_graph_equal(&x->result,&y->result))return 0;
  for(size_t i=0;i<a->count;i++)if(!us_binding_graph_equal(us_export_arg(x,i),us_export_arg(y,i)))return 0;return 1;
#else
  return 0;
#endif
 }
 if(!us_resolver_type_equal(&a->result,&b->result))return 0;
 if(a->kind&&(a->extent!=b->extent||a->writable!=b->writable))return 0;
 for(size_t i=0;i<a->count;i++)if(!us_resolver_type_equal(a->args+i,b->args+i))return 0;return 1;
}
static int us_resolver_declare(us_resolver*r,const us_bindings*injected,const char*name,unsigned kind,const us_binding_type*result,const us_binding_type*args,size_t count,unsigned variadic,uint64_t extent,unsigned writable,char*error,size_t cap){
 if(!r||!us_binding_name(name)||kind>1||(kind&&(count||variadic))||r->generation==UINT64_MAX)return us_binding_error(error,cap,"invalid import declaration");
 if(us_resolver_find(&r->declarations,name))return us_binding_error(error,cap,"duplicate import declaration");
 if(r->declarations.count>=US_RESOLVER_NAMES)return us_binding_error(error,cap,"import declaration capacity exceeded");
 us_bindings temp={0};int rc=kind?us_bindings_add_data(&temp,name,1,result,extent,writable,error,cap):us_bindings_add_function(&temp,name,1,result,args,count,variadic,error,cap);
 if(rc)return rc;temp.items[0].address=0;
 const us_binding*x=us_resolver_find(injected,name);
 if(x&&!us_resolver_declaration_equal(x,temp.items)){us_bindings_clear(&temp);return us_binding_error(error,cap,"injected and import declarations disagree");}
 if(us_binding_append(&r->declarations,temp.items,error,cap)){us_bindings_clear(&temp);return 1;}
 free(temp.items);r->generation++;return 0;
}
/* Validate a future injection against existing import declarations, without
   registering anything or changing generation/ownership. */
static int us_resolver_accept_injection(const us_resolver*r,const char*name,unsigned kind,const us_binding_type*result,const us_binding_type*args,size_t count,unsigned variadic,uint64_t extent,unsigned writable,char*error,size_t cap){
 if(!r||!us_binding_name(name)||kind>1||(kind&&(count||variadic))||r->generation==UINT64_MAX)return us_binding_error(error,cap,"invalid injection declaration");
 us_bindings temp={0};int rc=kind?us_bindings_add_data(&temp,name,1,result,extent,writable,error,cap):us_bindings_add_function(&temp,name,1,result,args,count,variadic,error,cap);
 if(rc)return rc;const us_binding*x=us_resolver_find(&r->declarations,name);int equal=!x||us_resolver_declaration_equal(x,temp.items);us_bindings_clear(&temp);
 return equal?0:us_binding_error(error,cap,"injected and import declarations disagree");
}
#ifdef UNISACC_LIBRARYNATIVE_H
static int us_resolver_declare_typed(us_resolver*r,const us_bindings*injected,const char*name,const void*sig,size_t length,char*error,size_t cap){
 if(!r||!us_binding_name(name)||r->generation==UINT64_MAX||us_resolver_find(&r->declarations,name)||r->declarations.count>=US_RESOLVER_NAMES)return us_binding_error(error,cap,"invalid or duplicate typed import");
 us_bindings temp={0};if(us_bindings_add_function_typed(&temp,name,1,sig,length,error,cap))return 1;temp.items[0].address=0;
 const us_binding *x=us_resolver_find(injected,name);
 if(x&&!us_resolver_declaration_equal(x,temp.items)){us_bindings_clear(&temp);return us_binding_error(error,cap,"injected and import declarations disagree");}
 if(us_binding_append(&r->declarations,temp.items,error,cap)){us_bindings_clear(&temp);return 1;}free(temp.items);r->generation++;return 0;
}
static int us_resolver_accept_injection_typed(const us_resolver*r,const char*name,const void*sig,size_t length,char*error,size_t cap){
 if(!r||!name||r->generation==UINT64_MAX)return us_binding_error(error,cap,"invalid typed injection");
 us_bindings temp={0};if(us_bindings_add_function_typed(&temp,name,1,sig,length,error,cap))return 1;
 const us_binding*x=us_resolver_find(&r->declarations,name);int equal=!x||us_resolver_declaration_equal(x,temp.items);us_bindings_clear(&temp);
 return equal?0:us_binding_error(error,cap,"injected and import declarations disagree");
}
#endif
static int us_resolver_load(us_resolver*r,const char*path,char*error,size_t cap){
 if(!r||!path||!*path||r->generation==UINT64_MAX)return us_binding_error(error,cap,"invalid library path");
 if(r->handle_count>=US_RESOLVER_HANDLES)return us_binding_error(error,cap,"library handle capacity exceeded");
 for(size_t i=0;i<r->handle_count;i++)if(!strcmp(r->handles[i].path,path))return us_binding_error(error,cap,"duplicate library path");
 size_t n=strlen(path)+1;char*p=malloc(n);if(!p)return us_binding_error(error,cap,"library path allocation failed");memcpy(p,path,n);
#ifdef _WIN32
 void*h=(void*)LoadLibraryA(path);if(!h){if(error&&cap)snprintf(error,cap,"library load failed (Windows error %lu)",(unsigned long)GetLastError());free(p);return 1;}
#else
 void*h=dlopen(path,RTLD_LOCAL|RTLD_NOW);if(!h){const char*message=dlerror();us_binding_error(error,cap,message?message:"library load failed");free(p);return 1;}
#endif
 r->handles[r->handle_count++]=(us_resolver_handle){p,h};r->generation++;return 0;
}
static void us_resolver_clear(us_resolver*r){if(!r)return;
 while(r->handle_count){us_resolver_handle*h=r->handles+--r->handle_count;
#ifdef _WIN32
  FreeLibrary((HMODULE)h->handle);
#else
  dlclose(h->handle);
#endif
  free(h->path);h->handle=NULL;h->path=NULL;
 }
 us_bindings_clear(&r->declarations);memset(r,0,sizeof *r);
}
typedef struct us_resolver_bytes {unsigned char*p;size_t n,count;} us_resolver_bytes;
static int us_resolver_candidate(us_resolver_bytes*b,const us_binding*decl,uintptr_t address,unsigned origin,uint64_t ordinal,char*error,size_t cap){
 us_binding x=*decl;x.address=address;us_bindings one={&x,1};unsigned char*v=NULL;size_t n=0;
 if(us_bindings_serialize(&one,&v,&n,error,cap))return 1;
 if(n<24||n>SIZE_MAX-8||b->n>SIZE_MAX-n||b->n+n>(size_t)INT_MAX||b->count>=US_RESOLVER_CANDIDATES){free(v);return us_binding_error(error,cap,"resolver wire capacity exceeded");}
 /* V1 16-byte header + 8-byte length. V2 adds ordinal after origin. */
 size_t payload=n-24,at=8+strlen(decl->name)+2,total=payload+8;
 unsigned char*p=realloc(b->p,b->n+8+total);if(!p){free(v);return us_binding_error(error,cap,"resolver wire allocation failed");}b->p=p;
 us_binding_put64(p+b->n,total);p+=b->n+8;
 memcpy(p,v+24,at);p[at-1]=(unsigned char)origin;us_binding_put64(p+at,ordinal);memcpy(p+at+8,v+24+at,payload-at);
 b->n+=8+total;b->count++;free(v);return 0;
}
#ifdef _WIN32
/* One frozen OS module enumeration, in Toolhelp order. Context-owned handles
   are excluded even if another owner has a reference to the same module. */
static int us_resolver_process(const us_resolver*r,HANDLE snapshot,const char*name,uintptr_t*out,char*error,size_t cap){
 MODULEENTRY32 entry;memset(&entry,0,sizeof entry);entry.dwSize=sizeof entry;*out=0;
 if(!Module32First(snapshot,&entry)){if(GetLastError()==ERROR_NO_MORE_FILES)return 0;return us_binding_error(error,cap,"process module enumeration failed");}
 for(;;){
  int owned=0;for(size_t j=0;j<r->handle_count;j++)if((HMODULE)r->handles[j].handle==entry.hModule){owned=1;break;}
  if(!owned){FARPROC address=GetProcAddress(entry.hModule,name);if(address){*out=(uintptr_t)address;return 0;}}
  if(!Module32Next(snapshot,&entry)){if(GetLastError()==ERROR_NO_MORE_FILES)return 0;return us_binding_error(error,cap,"process module enumeration failed");}
 }
}
#endif
#ifdef UNISACC_LIBRARYNATIVE_H
static int us_resolver_candidate3(us_resolver_bytes*b,const us_binding*decl,uintptr_t address,unsigned origin,uint64_t ordinal,uintptr_t dispatcher,us_native_plans*plans,char*error,size_t cap){
 unsigned char *v=NULL;size_t payload;uint64_t handle=0;
 if(decl->typed){
  if(!dispatcher)return us_binding_error(error,cap,"missing typed dispatcher");
  if(us_native_plan_add(plans,address,decl->signature,decl->signature_length,&handle,error,cap))return 1;
  payload=62+strlen(decl->name)+decl->signature_length;
  v=malloc(payload);if(!v)return us_binding_error(error,cap,"resolver wire allocation failed");size_t at=0,n=strlen(decl->name);
  us_binding_put64(v+at,n);at+=8;memcpy(v+at,decl->name,n);at+=n;
  v[at++]=0;v[at++]=(unsigned char)origin;us_binding_put64(v+at,ordinal);at+=8;v[at++]=0;v[at++]=(unsigned char)decl->variadic;
  us_binding_put64(v+at,address);at+=8;us_binding_put64(v+at,decl->count);at+=8;v[at++]=1;
  us_binding_put64(v+at,dispatcher);at+=8;us_binding_put64(v+at,handle);at+=8;us_binding_put64(v+at,decl->signature_length);at+=8;
  memcpy(v+at,decl->signature,decl->signature_length);at+=decl->signature_length;v[at++]=handle!=0;
  if(at!=payload){free(v);return us_binding_error(error,cap,"typed wire length mismatch");}
 }else{
  us_binding x=*decl;x.address=address;us_bindings one={&x,1};unsigned char*old=NULL;size_t n=0;
  if(us_bindings_serialize(&one,&old,&n,error,cap))return 1;
  size_t name=strlen(decl->name),prefix=10+name,desc=28+name;payload=n-24+9;v=malloc(payload);if(!v){free(old);return us_binding_error(error,cap,"resolver wire allocation failed");}
  memcpy(v,old+24,prefix);v[prefix-1]=(unsigned char)origin;us_binding_put64(v+prefix,ordinal);
  memcpy(v+prefix+8,old+24+prefix,desc-prefix);v[desc+8]=0;
  memcpy(v+desc+9,old+24+desc,n-24-desc);free(old);
 }
 if(payload>(size_t)INT_MAX || b->n>(size_t)INT_MAX-payload-8 || b->count>=US_RESOLVER_CANDIDATES){free(v);return us_binding_error(error,cap,"resolver wire capacity exceeded");}
 unsigned char *p=realloc(b->p,b->n+8+payload);if(!p){free(v);return us_binding_error(error,cap,"resolver wire allocation failed");}
 b->p=p;us_binding_put64(p+b->n,payload);memcpy(p+b->n+8,v,payload);b->n+=8+payload;b->count++;free(v);return 0;
}
#endif
static int us_resolver_freeze(const us_resolver*r,const us_bindings*injected,unsigned char**out,size_t*length,char*error,size_t cap){
 if(!out||!length)return us_binding_error(error,cap,"invalid resolver output");*out=NULL;*length=0;
 if(!r||r->handle_count>US_RESOLVER_HANDLES||r->declarations.count>US_RESOLVER_NAMES||(injected&&injected->count>US_RESOLVER_NAMES))return us_binding_error(error,cap,"invalid resolver registry");
 us_resolver_bytes b={malloc(16),16,0};if(!b.p)return us_binding_error(error,cap,"resolver wire allocation failed");memcpy(b.p,"USBIND2\n",8);
#ifdef _WIN32
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,GetCurrentProcessId());
 if(snapshot==INVALID_HANDLE_VALUE){free(b.p);return us_binding_error(error,cap,"process module snapshot failed");}
#endif
 size_t names=0;int rc=0;
 for(unsigned group=0;group<2&&!rc;group++){
  const us_bindings*list=group?&r->declarations:injected;if(!list)continue;
  for(size_t i=0;i<list->count&&!rc;i++){
   const us_binding*decl=list->items+i;const us_binding*x=us_resolver_find(injected,decl->name);
   if(group&&x){if(!us_resolver_declaration_equal(x,decl)){rc=us_binding_error(error,cap,"injected and import declarations disagree");break;}continue;}
   if(++names>US_RESOLVER_NAMES){rc=us_binding_error(error,cap,"resolver name capacity exceeded");break;}
   if(x)rc=us_resolver_candidate(&b,x,x->address,0,0,error,cap);
   if(rc)break;
#ifdef _WIN32
   uintptr_t address=0;rc=us_resolver_process(r,snapshot,decl->name,&address,error,cap);
   if(!rc&&address)rc=us_resolver_candidate(&b,decl,address,1,0,error,cap);
   for(size_t j=0;j<r->handle_count&&!rc;j++){FARPROC a=GetProcAddress((HMODULE)r->handles[j].handle,decl->name);if(a)rc=us_resolver_candidate(&b,decl,(uintptr_t)a,2,j+1,error,cap);}
#else
   dlerror();void*a=dlsym(RTLD_DEFAULT,decl->name);const char*failure=dlerror();if(!failure)rc=us_resolver_candidate(&b,decl,(uintptr_t)a,1,0,error,cap);
   for(size_t j=0;j<r->handle_count&&!rc;j++){dlerror();a=dlsym(r->handles[j].handle,decl->name);failure=dlerror();if(!failure)rc=us_resolver_candidate(&b,decl,(uintptr_t)a,2,j+1,error,cap);}
#endif
  }
 }
#ifdef _WIN32
 CloseHandle(snapshot);
#endif
 if(rc){free(b.p);return rc;}us_binding_put64(b.p+8,b.count);*out=b.p;*length=b.n;return 0;
}
#ifdef UNISACC_LIBRARYNATIVE_H
static int us_resolver_freeze_with_plans(const us_resolver*r,const us_bindings*injected,uintptr_t dispatcher,us_native_plans*plans,unsigned char**out,size_t*length,char*error,size_t cap){
 us_native_plans tmpplans={0};if(!plans || plans->head)return us_binding_error(error,cap,"invalid native plan output");
 if(!out||!length)return us_binding_error(error,cap,"invalid resolver output");*out=NULL;*length=0;
 if(!r||r->handle_count>US_RESOLVER_HANDLES||r->declarations.count>US_RESOLVER_NAMES||(injected&&injected->count>US_RESOLVER_NAMES))return us_binding_error(error,cap,"invalid resolver registry");
 us_resolver_bytes b={malloc(16),16,0};if(!b.p)return us_binding_error(error,cap,"resolver wire allocation failed");memcpy(b.p,"USBIND3\n",8);
#ifdef _WIN32
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,GetCurrentProcessId());
 if(snapshot==INVALID_HANDLE_VALUE){free(b.p);return us_binding_error(error,cap,"process module snapshot failed");}
#endif
 size_t names=0;int rc=0;
 for(unsigned group=0;group<2&&!rc;group++){
  const us_bindings*list=group?&r->declarations:injected;if(!list)continue;
  for(size_t i=0;i<list->count&&!rc;i++){
   const us_binding*decl=list->items+i;const us_binding*x=us_resolver_find(injected,decl->name);
   if(group&&x){if(!us_resolver_declaration_equal(x,decl)){rc=us_binding_error(error,cap,"injected and import declarations disagree");break;}continue;}
   if(++names>US_RESOLVER_NAMES){rc=us_binding_error(error,cap,"resolver name capacity exceeded");break;}
   if(x)rc=us_resolver_candidate3(&b,x,x->address,0,0,dispatcher,&tmpplans,error,cap);
   if(rc)break;
#ifdef _WIN32
   uintptr_t address=0;rc=us_resolver_process(r,snapshot,decl->name,&address,error,cap);
   if(!rc&&address)rc=us_resolver_candidate3(&b,decl,address,1,0,dispatcher,&tmpplans,error,cap);
   for(size_t j=0;j<r->handle_count&&!rc;j++){FARPROC a=GetProcAddress((HMODULE)r->handles[j].handle,decl->name);if(a)rc=us_resolver_candidate3(&b,decl,(uintptr_t)a,2,j+1,dispatcher,&tmpplans,error,cap);}
#else
   dlerror();void*a=dlsym(RTLD_DEFAULT,decl->name);const char*failure=dlerror();if(!failure)rc=us_resolver_candidate3(&b,decl,(uintptr_t)a,1,0,dispatcher,&tmpplans,error,cap);
   for(size_t j=0;j<r->handle_count&&!rc;j++){dlerror();a=dlsym(r->handles[j].handle,decl->name);failure=dlerror();if(!failure)rc=us_resolver_candidate3(&b,decl,(uintptr_t)a,2,j+1,dispatcher,&tmpplans,error,cap);}
#endif
  }
 }
#ifdef _WIN32
 CloseHandle(snapshot);
#endif
 if(rc){free(b.p);us_native_plans_clear(&tmpplans);return rc;}us_binding_put64(b.p+8,b.count);*plans=tmpplans;*out=b.p;*length=b.n;return 0;
}
#endif

#endif
