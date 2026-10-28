/* Host ABI fixture; private runtime + explicit package, not installed library qualification. */
#include "exec/c/libunisacc.c"
static int assertions,failures;
static void check(int ok,const char *what,int which){assertions++;failures+=!ok;printf("%s\toptions.%d.%s\n",ok?"PASS":"FAIL",which,what);}
int main(int argc,char **argv){
 if(argc!=2)return 2;
 const char *targets[]={NULL,"","unknown","lnx","lnx/bogus","lnx/x86_64/extra","lnx/x86_64","lnx/x86_64"};
 for(int i=0;i<8;i++){
  us_context *c=us_new(argv[1]);if(!c)return 2;
  if(us_add_source(c,"guard.c","long answer(void){return 37;}\n")||us_compile(c,"lnx/x86_64",0)||us_relocate(c)){fprintf(stderr,"fixture setup: %s\n",us_error(c));return 2;}
  long (*before)(void)=(long (*)(void))us_sym(c,"answer");
  if(!before||before()!=37)return 2;
  size_t n;const void *tape=us_tape(c,&n);void *copy=malloc(n?n:1);if(!copy)return 2;memcpy(copy,tape,n);
  int rc=us_compile(c,targets[i],i==6?-1:i==7?3:0);
  check(rc!=0&&!strcmp(us_error(c),"invalid compilation options"),"named-refusal",i);
  size_t after_n;const void *after_tape=us_tape(c,&after_n);
  int same=after_n==n&&after_tape&&!memcmp(copy,after_tape,n);
  check(same,"prior-tape",i);
  long (*after)(void)=(long (*)(void))us_sym(c,"answer");
  check(after==before&&same&&after&&after()==37,"prior-export-callable",i);
  free(copy);us_free(c);
 }
 const char *valid[]={"lnx/x86_64","lnx/arm64","osx/x86_64","osx/arm64","win/x86_64","win/arm64"};
 for(int i=0;i<6;i++){
  us_context *c=us_new(argv[1]);const char *t="_start:\n  imm r0, 7\n  ret\n";
  if(!c||us_add_tape(c,t,strlen(t)))return 2;
  check(!us_compile(c,valid[i],1),"valid-target-tape-options-only",i);
  check(us_compile(c,"invalid/target",1)!=0&&!strcmp(us_error(c),"invalid compilation options"),"tape-unknown-target-refused",i);
  size_t n;const void *after=us_tape(c,&n);check(n==strlen(t)&&after&&!memcmp(after,t,n),"tape-preserved",i);us_free(c);
 }
 for(int level=0;level<3;level++){
  us_context *c=us_new(argv[1]);if(!c||us_add_source(c,"level.c","long answer(void){return 37;}\n"))return 2;
  int ok=!us_compile(c,"lnx/x86_64",level)&&!us_relocate(c);long (*f)(void)=ok?(long (*)(void))us_sym(c,"answer"):NULL;
  check(ok&&f&&f()==37,"valid-optimisation-callable",level);us_free(c);
 }
 printf("LIBRARY_OPTION_GUARD\tassertions=%d\tfailed=%d\n",assertions,failures);return failures?1:0;
}
