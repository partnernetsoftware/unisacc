/* Standalone native Windows candidate probe. Build this same source as each
   fixture DLL with -DUS_RESOLVER_FIXTURE=33 or 44, and as an exported EXE. */
#ifdef US_RESOLVER_FIXTURE
#include <windows.h>
__declspec(dllexport) int resolver_shared(void){return US_RESOLVER_FIXTURE;}
__declspec(dllexport) int resolver_data=US_RESOLVER_FIXTURE+1;
__declspec(dllexport) int resolver_owned_only(void){return US_RESOLVER_FIXTURE;}
#else
#include <windows.h>
#include <tlhelp32.h>
#undef NDEBUG
#include <assert.h>
/* Fault controls still call real SDK APIs when not injecting failure. Only
   the header under test is intercepted; the independent oracle is native. */
static unsigned fault,opened,closed;
static HANDLE probe_snapshot(DWORD flags,DWORD pid){if(fault==1){SetLastError(ERROR_ACCESS_DENIED);return INVALID_HANDLE_VALUE;}HANDLE h=CreateToolhelp32Snapshot(flags,pid);if(h!=INVALID_HANDLE_VALUE)opened++;return h;}
static BOOL probe_first(HANDLE h,LPMODULEENTRY32 e){if(fault==2){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}return Module32First(h,e);}
static BOOL probe_next(HANDLE h,LPMODULEENTRY32 e){if(fault==3){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}return Module32Next(h,e);}
static FARPROC probe_address(HMODULE h,LPCSTR name){return fault==3?NULL:GetProcAddress(h,name);}
static BOOL probe_close(HANDLE h){BOOL ok=CloseHandle(h);if(ok)closed++;return ok;}
#define CreateToolhelp32Snapshot probe_snapshot
#define Module32First probe_first
#define Module32Next probe_next
#define GetProcAddress probe_address
#define CloseHandle probe_close
#include "../exec/c/libraryresolver.h"
#undef CreateToolhelp32Snapshot
#undef Module32First
#undef Module32Next
#undef GetProcAddress
#undef CloseHandle
__declspec(dllexport) int resolver_shared(void){return 22;}
__declspec(dllexport) int resolver_data=23;
static int injected_shared(void){return 11;}
static uint64_t u64(const unsigned char*p){uint64_t x=0;for(unsigned i=0;i<8;i++)x|=(uint64_t)p[i]<<(8*i);return x;}
/* Independent OS oracle: compare the process candidate address to the first
   exported match in a fresh Toolhelp enumeration, skipping owned modules. */
static uintptr_t oracle(const us_resolver*r,const char*name){
 HANDLE h=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,GetCurrentProcessId());assert(h!=INVALID_HANDLE_VALUE);
 MODULEENTRY32 e={0};e.dwSize=sizeof e;uintptr_t address=0;BOOL more=Module32First(h,&e);assert(more);
 while(more){int owned=0;for(size_t j=0;j<r->handle_count;j++)if(e.hModule==(HMODULE)r->handles[j].handle)owned=1;
  if(!owned){FARPROC a=GetProcAddress(e.hModule,name);if(a){address=(uintptr_t)a;break;}}
  more=Module32Next(h,&e);
 }assert(CloseHandle(h));return address;
}
static void inspect(const us_resolver*r,unsigned char*p,size_t n){
 assert(n>=16&&!memcmp(p,"USBIND2\n",8));assert(u64(p+8)==11);size_t at=16;unsigned seen[2][3]={{0}},owned=0,system=0;
 for(unsigned i=0;i<11;i++){
  uint64_t len=u64(p+at);at+=8;assert(len<=n-at);size_t end=at+(size_t)len;
  uint64_t nl=u64(p+at);at+=8;assert(nl<100);char name[100];memcpy(name,p+at,(size_t)nl);name[nl]=0;at+=(size_t)nl;
  unsigned kind=p[at++],origin=p[at++];uint64_t ordinal=u64(p+at);at+=8;assert(p[at++]==0&&p[at++]==0);uint64_t address=u64(p+at);at+=8;
  uint64_t count=u64(p+at);at+=8;assert(count==0);at+=48;
  if(kind){assert(u64(p+at)==4);at+=8;assert(p[at++]==1);}assert(p[at++]==1);assert(at==end&&origin<=2&&address);
  if(origin==1)assert(!ordinal&&address==oracle(r,name));
  if(!strcmp(name,"GetCurrentProcessId")){assert(!kind&&origin==1&&!ordinal);assert(((DWORD(WINAPI*)(void))(uintptr_t)address)()==GetCurrentProcessId());system++;continue;}
  if(!strcmp(name,"resolver_owned_only")){assert(!kind&&origin==2&&ordinal>=1&&ordinal<=2);assert(((int(*)(void))(uintptr_t)address)()==(ordinal==1?33:44));owned|=1u<<(ordinal-1);continue;}
  unsigned k=!strcmp(name,"resolver_data");assert(k||!strcmp(name,"resolver_shared"));assert(kind==k);
  if(origin==2){assert(ordinal==1||ordinal==2);seen[k][origin]|=1u<<(ordinal-1);}else{assert(ordinal==0);seen[k][origin]++;}
  int value=k?*(int*)(uintptr_t)address:((int(*)(void))(uintptr_t)address)();
  assert(value==(origin==0?(k?13:11):origin==1?(k?23:22):ordinal==1?(k?34:33):(k?45:44)));
 }
 assert(at==n&&owned==3&&system==1);for(unsigned k=0;k<2;k++)assert(seen[k][0]==1&&seen[k][1]==1&&seen[k][2]==3);
}
int main(int argc,char**argv){
 assert(argc==3&&sizeof(uintptr_t)==8&&sizeof(long)==4);char error[512];us_resolver r={0};us_bindings in={0};us_binding_type t={0,0,0,1,4,0},opaque=t,bad=t;opaque.base=999;opaque.shape=777;bad.width=8;int injected_data=13;
 assert(!GetModuleHandleA(argv[1])&&!GetModuleHandleA(argv[2]));
 assert(!us_bindings_add_function(&in,"resolver_shared",(uintptr_t)injected_shared,&t,NULL,0,0,error,sizeof error));
 assert(!us_bindings_add_data(&in,"resolver_data",(uintptr_t)&injected_data,&t,4,1,error,sizeof error));
 assert(!us_resolver_declare(&r,&in,"resolver_shared",0,&opaque,NULL,0,0,0,0,error,sizeof error));
 assert(!us_resolver_declare(&r,&in,"resolver_data",1,&opaque,NULL,0,0,4,1,error,sizeof error));
 assert(!us_resolver_declare(&r,&in,"resolver_owned_only",0,&t,NULL,0,0,0,0,error,sizeof error));
 assert(!us_resolver_declare(&r,&in,"GetCurrentProcessId",0,&t,NULL,0,0,0,0,error,sizeof error));
 uint64_t gen=r.generation;size_t size=r.declarations.count;
 assert(us_resolver_declare(&r,&in,"resolver_shared",0,&t,NULL,0,0,0,0,error,sizeof error));assert(r.generation==gen&&r.declarations.count==size);
 assert(!us_resolver_accept_injection(&r,"resolver_shared",0,&t,NULL,0,0,0,0,error,sizeof error));assert(r.generation==gen);
 assert(us_resolver_accept_injection(&r,"resolver_shared",0,&bad,NULL,0,0,0,0,error,sizeof error));assert(r.generation==gen);
 assert(!us_resolver_load(&r,argv[1],error,sizeof error));assert(!us_resolver_load(&r,argv[2],error,sizeof error));gen=r.generation;
 assert(us_resolver_load(&r,argv[1],error,sizeof error));assert(us_resolver_load(&r,"C:\\no-such-unisacc-resolver-fixture.dll",error,sizeof error));assert(r.handle_count==2&&r.generation==gen);
 unsigned char*p=NULL,*q=NULL;size_t n=0,m=0;assert(!us_resolver_freeze(&r,&in,&p,&n,error,sizeof error));inspect(&r,p,n);
 DWORD before=0,after=0;assert(GetProcessHandleCount(GetCurrentProcess(),&before));
 for(unsigned i=0;i<100;i++){assert(!us_resolver_freeze(&r,&in,&q,&m,error,sizeof error));assert(n==m&&!memcmp(p,q,n));free(q);}
 assert(GetProcessHandleCount(GetCurrentProcess(),&after)&&before==after);free(p);
 us_resolver empty={0};assert(!us_resolver_declare(&empty,NULL,"no_such_us_symbol",0,&t,NULL,0,0,0,0,error,sizeof error));
 assert(!us_resolver_freeze(&empty,NULL,&p,&n,error,sizeof error));assert(n==16&&u64(p+8)==0&&empty.declarations.count==1);free(p);us_resolver_clear(&empty);
 us_resolver unsupported={0};us_binding_type fp={0,0,0,3,8,0};assert(!us_resolver_declare(&unsupported,NULL,"resolver_shared",0,&fp,NULL,0,0,0,0,error,sizeof error));
 assert(!us_resolver_freeze(&unsupported,NULL,&p,&n,error,sizeof error));assert(u64(p+8)==1&&p[n-1]==0);free(p);us_resolver_clear(&unsupported);
 us_resolver full={0};full.handle_count=64;full.generation=91;assert(us_resolver_load(&full,argv[1],error,sizeof error));assert(full.handle_count==64&&full.generation==91);
 us_resolver expired={0};expired.generation=UINT64_MAX;assert(us_resolver_declare(&expired,NULL,"resolver_shared",0,&t,NULL,0,0,0,0,error,sizeof error));assert(us_resolver_accept_injection(&expired,"resolver_shared",0,&t,NULL,0,0,0,0,error,sizeof error));assert(expired.generation==UINT64_MAX&&!expired.declarations.count);
 for(fault=1;fault<=3;fault++){p=(void*)1;n=999;assert(us_resolver_freeze(&r,&in,&p,&n,error,sizeof error));assert(!p&&!n&&opened==closed&&r.handle_count==2&&r.generation==gen);}fault=0;
 in.items[0].result.width=8;p=(void*)1;n=999;assert(us_resolver_freeze(&r,&in,&p,&n,error,sizeof error));assert(!p&&!n&&r.handle_count==2);in.items[0].result.width=4;
 assert(GetProcessHandleCount(GetCurrentProcess(),&after)&&before==after);
 us_resolver_clear(&r);assert(!r.handle_count&&!r.declarations.count&&!r.generation);us_resolver_clear(&r);us_bindings_clear(&in);
 assert(!GetModuleHandleA(argv[1])&&!GetModuleHandleA(argv[2])&&opened==closed);
 puts("Windows native resolver: 11 actual function/data candidates; process order and owned exclusion; missing/unsupported/duplicate/load failure/generation; 100 snapshot cleanup and module unload controls pass");return 0;
}
#endif
