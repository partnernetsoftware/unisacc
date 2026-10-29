/* Native host ABI fixture; tests the private runtime, not installed APE APIs. */
#include "exec/c/libunisacc.c"
static int count,failed;
static void check(int ok,const char *name){count++;failed+=!ok;printf("%s\tfile.%s\n",ok?"PASS":"FAIL",name);}
static void put(const char *path,const char *s){FILE *f=fopen(path,"wx");if(!f||fputs(s,f)<0||fclose(f))exit(2);}
int main(int argc,char **argv){
 if(argc!=3)return 2;
 if(geteuid()==0){fputs("NOT_RUN permission oracle requires non-root\n",stderr);return 3;}
 if(mkdir(argv[2],0700))return 2;
 char good[4096],denied[4096],paths[4][4096];
 snprintf(good,sizeof good,"%s/good.c",argv[2]);snprintf(denied,sizeof denied,"%s/denied.c",argv[2]);
 const char *source="long answer(void){return 37;}\n";put(good,source);put(denied,source);if(chmod(denied,0000))return 2;
 snprintf(paths[0],sizeof paths[0],"%s/absent.c",argv[2]);snprintf(paths[1],sizeof paths[1],"%s/good.c/child.c",argv[2]);
 snprintf(paths[2],sizeof paths[2],"%s",denied);snprintf(paths[3],sizeof paths[3],"%s/dangling",argv[2]);if(symlink("absent.c",paths[3]))return 2;
 const char *want[]={"missing file","cannot open source file: not a directory","cannot open source file: permission denied","missing file"};
 const char *names[]={"ENOENT","ENOTDIR","EACCES","dangling-ENOENT"};
 us_context *a=us_new(argv[1]),*b=us_new(argv[1]);if(!a||!b)return 2;
 for(int i=0;i<4;i++){
  check(us_add_file(a,paths[i])!=0&&!strcmp(us_error(a),want[i]),names[i]);
  check(us_compile(a,"lnx/x86_64",0)!=0&&!strcmp(us_error(a),"no input"),"failure-added-no-input");
 }
 if(chmod(denied,0600))return 2;
 check(!us_add_file(a,good),"add-valid-after-refusals");check(!us_add_source(b,good,source),"add-same-source");
 int ca=us_compile(a,"lnx/x86_64",0),cb=us_compile(b,"lnx/x86_64",0);
 check(!ca&&!cb,"compile-both");
 size_t na=0,nb=0;const void *ta=us_tape(a,&na),*tb=us_tape(b,&nb);
 check(!ca&&!cb&&ta&&tb&&na&&na==nb&&!memcmp(ta,tb,na),"file-and-source-tape-equal");
 int relocated=!ca&&!us_relocate(a);long (*f)(void)=relocated?(long (*)(void))us_sym(a,"answer"):NULL;
 check(f&&f()==37,"recovery-call37");
 us_free(a);us_free(b);printf("LIBRARY_FILE_GUARD\tassertions=%d\tfailed=%d\n",count,failed);return failed?1:0;
}
