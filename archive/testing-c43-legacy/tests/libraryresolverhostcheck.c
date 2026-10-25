#include "../exec/c/libraryresolver.h"
#include <assert.h>
int resolver_shared(void){return 22;}
int resolver_data=23;
static int injected_shared(void){return 11;}
static uint64_t u64(const unsigned char*p){uint64_t x=0;for(unsigned i=0;i<8;i++)x|=(uint64_t)p[i]<<(8*i);return x;}
static void inspect(unsigned char*p,size_t n){
 assert(n>=16&&!memcmp(p,"USBIND2\n",8));assert(u64(p+8)==8);size_t at=16;unsigned seen[2][3]={{0}};
 for(unsigned i=0;i<8;i++){
  uint64_t len=u64(p+at);at+=8;assert(len<=n-at);size_t end=at+(size_t)len;
  uint64_t nl=u64(p+at);at+=8;assert(nl<100);char name[100];memcpy(name,p+at,(size_t)nl);name[nl]=0;at+=(size_t)nl;
  unsigned kind=p[at++],origin=p[at++];uint64_t ordinal=u64(p+at);at+=8;assert(p[at++]==0&&p[at++]==0);uint64_t address=u64(p+at);at+=8;
  uint64_t count=u64(p+at);at+=8;assert(count==0);at+=48;
  if(kind){assert(u64(p+at)==4);at+=8;assert(p[at++]==1);}assert(p[at++]==1);assert(at==end);
  unsigned k=!strcmp(name,"resolver_data");assert(k||!strcmp(name,"resolver_shared"));assert(kind==k&&origin<=2);
  if(origin==2){assert(ordinal==1||ordinal==2);seen[k][origin]|=1u<<(ordinal-1);}else{assert(ordinal==0);seen[k][origin]++;}
  if(!k){int value=((int(*)(void))(uintptr_t)address)();assert(value==(origin==0?11:origin==1?22:ordinal==1?33:44));}
  else {int value=*(int*)(uintptr_t)address;assert(value==(origin==0?13:origin==1?23:ordinal==1?34:45));}
 }
 assert(at==n);for(unsigned k=0;k<2;k++)assert(seen[k][0]==1&&seen[k][1]==1&&seen[k][2]==3);
}
int main(int argc,char**argv){assert(argc==3);char error[512];us_resolver r={0};us_bindings in={0};us_binding_type t={0,0,0,1,4,0},opaque=t,bad=t;opaque.base=999;opaque.shape=777;bad.width=8;int injected_data=13;
 assert(!us_bindings_add_function(&in,"resolver_shared",(uintptr_t)injected_shared,&t,NULL,0,0,error,sizeof error));
 assert(!us_bindings_add_data(&in,"resolver_data",(uintptr_t)&injected_data,&t,4,1,error,sizeof error));
 assert(!us_resolver_declare(&r,&in,"resolver_shared",0,&opaque,NULL,0,0,0,0,error,sizeof error));
 assert(!us_resolver_declare(&r,&in,"resolver_data",1,&opaque,NULL,0,0,4,1,error,sizeof error));
 uint64_t gen=r.generation;size_t size=r.declarations.count;
 assert(us_resolver_declare(&r,&in,"resolver_shared",0,&t,NULL,0,0,0,0,error,sizeof error));assert(r.generation==gen&&r.declarations.count==size);
 assert(!us_resolver_accept_injection(&r,"resolver_shared",0,&t,NULL,0,0,0,0,error,sizeof error));assert(r.generation==gen);
 assert(us_resolver_accept_injection(&r,"resolver_shared",0,&bad,NULL,0,0,0,0,error,sizeof error));assert(r.generation==gen);
 assert(us_resolver_declare(&r,&in,NULL,0,&t,NULL,0,0,0,0,error,sizeof error));
 assert(!us_resolver_load(&r,argv[1],error,sizeof error));assert(!us_resolver_load(&r,argv[2],error,sizeof error));gen=r.generation;
 assert(us_resolver_load(&r,argv[1],error,sizeof error));assert(us_resolver_load(&r,"/no/such/unisacc-resolver-fixture",error,sizeof error));assert(r.handle_count==2&&r.generation==gen);
 unsigned char*p=NULL,*q=NULL;size_t n=0,m=0;assert(!us_resolver_freeze(&r,&in,&p,&n,error,sizeof error));inspect(p,n);
 assert(!us_resolver_freeze(&r,&in,&q,&m,error,sizeof error));assert(n==m&&!memcmp(p,q,n));free(q);free(p);
 /* Nothing resolved: retain the declared name and a 16-byte nonempty resource. */
 us_resolver empty={0};assert(!us_resolver_declare(&empty,NULL,"no_such_us_symbol",0,&t,NULL,0,0,0,0,error,sizeof error));
 assert(!us_resolver_freeze(&empty,NULL,&p,&n,error,sizeof error));assert(n==16&&u64(p+8)==0&&empty.declarations.count==1);free(p);us_resolver_clear(&empty);
 /* Unsupported declarations remain candidates; the host never drops them. */
 us_resolver unsupported={0};us_binding_type fp={0,0,0,3,8,0};assert(!us_resolver_declare(&unsupported,NULL,"resolver_shared",0,&fp,NULL,0,0,0,0,error,sizeof error));
 assert(!us_resolver_freeze(&unsupported,NULL,&p,&n,error,sizeof error));assert(u64(p+8)==1&&p[n-1]==0);free(p);us_resolver_clear(&unsupported);
 /* Capacity failures occur before dlopen and leave the supplied state intact. */
 us_resolver full={0};full.handle_count=64;full.generation=91;assert(us_resolver_load(&full,argv[1],error,sizeof error));assert(full.handle_count==64&&full.generation==91);
 us_resolver expired={0};expired.generation=UINT64_MAX;assert(us_resolver_declare(&expired,NULL,"resolver_shared",0,&t,NULL,0,0,0,0,error,sizeof error));assert(us_resolver_accept_injection(&expired,"resolver_shared",0,&t,NULL,0,0,0,0,error,sizeof error));assert(expired.generation==UINT64_MAX&&!expired.declarations.count);
 /* New injection since declaration must be checked again at freeze. */
 in.items[0].result.width=8;p=(void*)1;n=999;assert(us_resolver_freeze(&r,&in,&p,&n,error,sizeof error));assert(!p&&!n&&r.handle_count==2);in.items[0].result.width=4;
 us_resolver_clear(&r);assert(!r.handle_count&&!r.declarations.count&&!r.generation);us_resolver_clear(&r);us_bindings_clear(&in);
 printf("all 8 candidates actual values; missing declarations retained; unsupported emitted; failure/generation/cleanup controls pass\n");return 0;
}
