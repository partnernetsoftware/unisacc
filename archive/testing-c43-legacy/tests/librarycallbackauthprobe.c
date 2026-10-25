/* Narrow internal candidate-authentication component witness. Production readers,
   plan creation, catalogue loader, registry, frame guard and dispatch are reused.
   No host.h changes, signature fabrication, guest execution or callback ABI claim. */
#include "exec/c/libunisacc.c"
static long native_leaf(long x){return x;}
static void put64(unsigned char*p,uint64_t v){for(int i=0;i<8;i++)p[i]=(unsigned char)(v>>(8*i));}
int main(int argc,char**argv){
 if(argc!=5)return 2;
 size_t na=0,nb=0;FILE*fa=fopen(argv[2],"rb"),*fb=fopen(argv[3],"rb");if(!fa||!fb)return 2;
 unsigned char aa[4096],bb[4096];na=fread(aa,1,sizeof aa,fa);nb=fread(bb,1,sizeof bb,fb);fclose(fa);fclose(fb);
 us_context*c=us_new(argv[1]);if(!c)return 2;
 if(us_set_signature_version(c,3)){fprintf(stderr,"SETUP signature configuration: %s\n",c->error);return 2;}
 fprintf(stderr,"SETUP production generation=%llu\n",(unsigned long long)c->image_generation);
 LibraryCarrierOwner carrier_owner={c,"lnx/x86_64"};c->native_plans.carrier_provider=library_carrier_provider;c->native_plans.carrier_owner=&carrier_owner;
 uint64_t frozen=0;int plan_rc=us_native_plan_add(&c->native_plans,(uintptr_t)native_leaf,aa,na,&frozen,c->error,sizeof c->error);fprintf(stderr,"SETUP plan rc=%d handle_nonzero=%d sig_bytes=%zu target_nonzero=%d error=%s\n",plan_rc,!!frozen,na,!!(uintptr_t)native_leaf,c->error);if(plan_rc||!frozen)return 2;
 unsigned char catalog[8192];memcpy(catalog,"USLCALL1\n",9);put64(catalog+9,1);put64(catalog+17,1);put64(catalog+25,nb);memcpy(catalog+33,bb,nb);
 if(library_callable_catalog_load(c,catalog,33+nb)){fprintf(stderr,"SETUP catalog: %s\n",c->error);return 2;}
 us_callables_init(&c->callables,c,c->image_generation,library_invoke_frame,library_callable_native_hook,library_callable_failure_hook);
 c->callables.script_call=library_callable_script_hook;
 size_t page=library_page_size();unsigned char*stack=calloc(3,page);if(!stack)return 2;
 uint64_t*out=(uint64_t*)(stack+page);*out=0x123456789abcdef0ULL;
 ScriptFrame frame={0};us_call_outcome outcome={0};frame.owner=c;frame.stack_base=stack;frame.stack_size=3*page;frame.outcome=&outcome;
 active=c;script_frames=&frame;volatile int rc=0;
 if(setjmp(frame.returned))rc=1;
 else rc=(int)library_callable_make_dispatch(US_CALLABLE_NATIVE,1,(uintptr_t)native_leaf,(uintptr_t)out,0,0);
 int negative=!strcmp(argv[4],"negative");int good=negative ? rc!=0 && *out==0x123456789abcdef0ULL && !strcmp(c->error,"native callable lacks frozen candidate declaration") : rc==0 && *out!=0x123456789abcdef0ULL && *out!=0;
 fprintf(stderr,"candidate-auth rc=%d named=%s output_committed=%d frame_failed=%d\n",rc,c->error,*out!=0x123456789abcdef0ULL,frame.failed);
 script_frames=NULL;active=NULL;free(stack);us_free(c);
 if(!good)return 2;return negative?1:0;
}
