/* Public native -> model script union -> typed native union. Expected red until
   an explicit model NativePlan backend exists; no substitute ffi union layout. */
#include "libunisacc.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
union U {double d;unsigned long long bits;};
_Static_assert(sizeof(union U)==8 && _Alignof(union U)==8,"fixture requires declared union8 ABI");
typedef union U (*Round)(union U);
static unsigned native_calls;
static us_context *callback_context;static Round callback_entry;static int callback_lookup_failed;
static const uint64_t script_mask=UINT64_C(0x0102030405060708);
static const uint64_t native_mask=UINT64_C(0x8877665544332211);
static union U host_flip(union U x){native_calls++;if(callback_context&&(Round)us_sym(callback_context,"round")!=callback_entry)callback_lookup_failed=1;x.bits^=native_mask;return x;}
static const char export_source[]=
"union U {double d;unsigned long long bits;};"
"union U round(union U x){x.bits^=0x0102030405060708ULL;return x;}";
static const char source[]=
"union U {double d;unsigned long long bits;};"
"union U host_flip(union U);"
"union U round(union U x){x.bits^=0x0102030405060708ULL;return host_flip(x);}";
static void quoted(const char *s){putchar('"');for(;*s;s++){unsigned char c=(unsigned char)*s;if(c=='"'||c=='\\'){putchar('\\');putchar(c);}else if(c<32)printf("\\u%04x",c);else putchar(c);}putchar('"');}
static int event(us_context *c,const char *stage,int opt,int rc){
 printf("{\"stage\":");quoted(stage);printf(",\"opt\":%d,\"rc\":%d,\"native_calls\":%u,\"error\":",opt,rc,native_calls);quoted(c?us_error(c):"context allocation failed");puts("}");fflush(stdout);return rc;
}
static int bind(us_context *c,const char *path,int opt){
 unsigned char sig[8192];FILE *f=fopen(path,"rb");if(!f){event(c,"signature_read",opt,1);return 1;}
 size_t n=fread(sig,1,sizeof sig,f);int bad=ferror(f)||!feof(f);fclose(f);
 if(bad)return event(c,"signature_read",opt,1);
 return event(c,"registration",opt,us_add_symbol_typed(c,"host_flip",(void*)host_flip,sig,n));
}
int main(int argc,char **argv){
 if(argc!=4&&argc!=5)return 2;
 int export_only=argc==5&&!strcmp(argv[4],"export-only");
 if(argc==5&&!export_only)return 2;
 /* An unused declaration still goes through the real resolver freeze. Its
    successful compile separates freeze/registration from referenced E3 refusal. */
 us_context *pre=us_new(argv[1]);if(!pre){event(NULL,"context",0,1);return 1;}
 if(bind(pre,argv[3],0)||event(pre,"preflight_add",0,us_add_source(pre,"unused-union.c","int control(void){return 7;}"))||
    event(pre,"unused_binding_compile",0,us_compile(pre,argv[2],0))){us_free(pre);return 1;}us_free(pre);
 for(int opt=0;opt<3;opt++){
  us_context *c=us_new(argv[1]);if(!c){event(NULL,"context",opt,1);return 1;}
  if(bind(c,argv[3],opt)||event(c,"source_add",opt,us_add_source(c,"union-chain.c",export_only?export_source:source))||
     event(c,"compile",opt,us_compile(c,argv[2],opt))||event(c,"relocate",opt,us_relocate(c))){us_free(c);return 1;}
  Round round=(Round)us_sym(c,"round");if(event(c,"us_sym",opt,round?0:1)){us_free(c);return 1;}
  if((void*)round!=us_sym(c,"round")){event(c,"stable_export",opt,1);us_free(c);return 1;}
  callback_context=c;callback_entry=round;callback_lookup_failed=0;
  for(unsigned i=0;i<100;i++){
   struct {uint64_t before;union U value;uint64_t after;} input={UINT64_C(0x123456789abcdef0),{.bits=UINT64_C(0x7ff8000000000001)+i},UINT64_C(0xfedcba9876543210)};
   struct {uint64_t before;union U value;uint64_t after;} output={UINT64_C(0xabcdef0123456789),{.bits=0},UINT64_C(0x9876543210fedcba)};
   uint64_t original=input.value.bits;unsigned before=native_calls;output.value=round(input.value);int status=-1;
   if(output.value.bits!=(original^script_mask^(export_only?0:native_mask))||input.value.bits!=original||native_calls!=before+(export_only?0:1)||
      input.before!=UINT64_C(0x123456789abcdef0)||input.after!=UINT64_C(0xfedcba9876543210)||
      output.before!=UINT64_C(0xabcdef0123456789)||output.after!=UINT64_C(0x9876543210fedcba)||callback_lookup_failed||us_call_status(c,&status)){
    event(c,"call_value_copy_canary_status",opt,1);us_free(c);return 1;
   }
  }
  event(c,"calls_100",opt,0);callback_context=NULL;callback_entry=NULL;us_free(c);
 }
 return 0;
}
