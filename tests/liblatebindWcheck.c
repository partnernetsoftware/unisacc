/* Native observation adapter; verdicts belong to liblatebindWcheck.cx.
 * W_FAKE is test-only. Logical kind7 deliberately shares data wire kind1. */
#if W_NEW && W_FAKE
#define US_LATE_EXTRA_CLASSES(X) X(7,1,4,1,"kind7 E3 unavailable","kind7 lower unavailable",us_declare_injected_data,w_kind7_verify)
#endif
#include "exec/c/libunisacc.c"
static unsigned hits;
#if W_NEW && W_FAKE
static int w_kind7_verify(void *ctx,const unsigned char *r,size_t n,uint32_t id,const char *name,uint64_t *address){
 int rc=us_late_data_provider(ctx,r,n,id,name,address);hits++;
 if(rc)return rc==2?us_late_data_error(ctx,"fake-kind7-missing-provider",id,name):rc;
 uint32_t token=0;memcpy(&token,(void *)(uintptr_t)(*address+4),4);
 if(token!=0x71371371)return us_late_data_error(ctx,"fake-kind7-provider-token-mismatch",id,name);
 return 0;
}
#endif
static long hostadd(long x,long y){return x+y+1000;}
static struct {int value;uint32_t token;} real_obj={11,0},fake_obj={22,0x71371371};
static const char *src="long hostadd(long,long); extern int real_value; extern int fake_value; long answer(long n){return hostadd(n,real_value)+fake_value;}";
static us_signature data(unsigned kind,unsigned width){us_signature s;memset(&s,0,sizeof s);s.kind=kind;s.result.kind=1;s.result.width=width;s.extent=8;s.writable=1;return s;}
static int save(const char *dir,const char *tag,unsigned i,const void *p,size_t n){char path[4096];snprintf(path,sizeof path,"%s/%s-%u.bin",dir,tag,i);FILE *f=fopen(path,"wb");if(!f)return 1;int rc=n&&(!p||fwrite(p,1,n,f)!=n);return fclose(f)||rc;}
static int views(us_context *c,const char *dir,const char *tag){
 const void *p;size_t n;int rc=save(dir,tag,0,c->late_contract,c->late_contract_n)|save(dir,tag,1,c->late_selection,c->late_selection_n);
 n=0;p=us_library_e3_fragment(c,&n);rc|=save(dir,tag,2,p,n);
 n=0;p=us_library_lower_unbound(c,&n);rc|=save(dir,tag,3,p,n);
 n=0;p=us_library_data_fragment(c,&n);rc|=save(dir,tag,4,p,n);
 n=0;p=us_library_data_lower_unbound(c,&n);rc|=save(dir,tag,5,p,n);return rc;
}
int main(int argc,char **argv){
 if(argc!=5)return 2;us_context *c=us_new(argv[1]);if(!c)return 3;
 us_type_descriptor args[2];memset(args,0,sizeof args);args[0].kind=args[1].kind=1;args[0].width=args[1].width=8;
 us_signature fn;memset(&fn,0,sizeof fn);fn.result=args[0];fn.args=args;fn.count=2;
 us_signature rd=data(1,4),fd=data(1,4),offer=data(1,4);int rc;
 if(us_add_symbol(c,"hostadd",(void *)hostadd,&fn)||us_declare_injected_data(c,"real_value",&rd))return 4;
#if W_NEW && W_FAKE
 fd.kind=7;rc=us_late_declare_kind(c,7,"fake_value",&fd);
#else
 rc=us_declare_injected_data(c,"fake_value",&fd);
#endif
 if(rc||us_register_data_provider(c,"real_value",&real_obj.value,&offer)||us_register_data_provider(c,"fake_value",&fake_obj.value,&offer)||us_add_source(c,"W.c",src))return 5;
 rc=us_compile(c,argv[2],1);printf("compile\t%d\t%s\n",rc,rc?us_error(c):"ACCEPT");if(rc)return 6;
 rc=us_relocate(c);printf("relocate\t%d\t%s\n",rc,rc?us_error(c):"ACCEPT");if(rc)return 7;
 long (*answer)(long)=(long(*)(long))us_sym(c,"answer");printf("answer\t%ld\n",answer?answer(5):-1L);
 if(views(c,argv[3],"success"))return 8;
#if W_NEW && W_FAKE
 printf("kind7-validator-hits\t%u\n",hits);
 fake_obj.token=0;us_register_data_provider(c,"fake_value",&fake_obj.value,&offer);
 rc=us_relocate(c);printf("fake-conflict\t%d\t%s\n",rc,us_error(c));
 printf("fake-export\t%d\n",us_sym(c,"answer")!=NULL);
 fake_obj.token=0x71371371;us_register_data_provider(c,"fake_value",&fake_obj.value,&offer);if(us_relocate(c))return 9;
#endif
 us_signature wrong=data(1,8);if(us_register_data_provider(c,"real_value",&real_obj.value,&wrong))return 10;
 if(views(c,argv[3],"before-failure"))return 11;
 rc=us_relocate(c);char diagnostic[4096];snprintf(diagnostic,sizeof diagnostic,"%s",us_error(c));
 printf("interleaved\t%d\t%s\n",rc,diagnostic);printf("failed-export\t%d\n",us_sym(c,"answer")!=NULL);
 if(views(c,argv[3],"after-failure"))return 12;
#if W_NEW && !W_FAKE
 fd.kind=7;rc=us_late_declare_kind(c,7,"new_kind",&fd);printf("macro-off\t%d\t%s\n",rc,us_error(c));
#endif
 printf("host-values\t%d\t%d\n",real_obj.value,fake_obj.value);us_free(c);
 const char *negative[2]={"extern int real_value; int read_value(void){return *(&real_value+1);}","extern int real_value; int read_value(void){real_value=4;return real_value;}"};
 for(int k=0;k<2;k++){
  c=us_new(argv[1]);us_signature ro=data(1,4);ro.writable=0;
  if(!c||us_declare_injected_data(c,"real_value",&ro)||us_register_data_provider(c,"real_value",&real_obj.value,&ro)||us_add_source(c,"W-negative.c",negative[k]))return 13;
  rc=us_compile(c,argv[2],1);if(!rc)rc=us_relocate(c);
  printf("refusal-%d\t%d\t%s\n",k,rc,us_error(c));
  printf("refusal-export-%d\t%d\n",k,us_sym(c,"read_value")!=NULL);us_free(c);
 }
 return 0;
}
