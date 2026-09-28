/* Actual incoming native aggregate copies forwarded across callback boundaries. */
#include "libunisacc.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
struct Pair {double d;int n;};
typedef struct Pair (*Leaf)(struct Pair);
typedef struct Pair (*Relay)(Leaf,struct Pair);
static Relay retained;
static int leaf_calls,take_calls,drive_calls,normal_returns,nesting,bad_native_copy;
static struct Pair native_leaf(struct Pair p){leaf_calls++;struct Pair r={p.d+2,p.n+5};p.d=999;p.n=999;return r;}
static struct Pair other_leaf(struct Pair p){struct Pair r=native_leaf(p);r.d+=1;r.n+=10;return r;}
static struct Pair host_take(Leaf leaf,struct Pair p){
 take_calls++;
 if(!nesting){nesting=1;struct Pair r=retained(leaf,p);nesting=0;
  if(p.d!=15||p.n!=35)bad_native_copy=1;return r;}
 return leaf(p);
}
static struct Pair host_drive(Relay relay,Leaf leaf,struct Pair p){
 drive_calls++;retained=relay;struct Pair r=relay(leaf,p);normal_returns++;
 if(p.d!=15||p.n!=35)bad_native_copy=1;return r;
}
static const char source[]=
"struct Pair{double d;int n;};"
"typedef struct Pair (*Leaf)(struct Pair);"
"typedef struct Pair (*Relay)(Leaf,struct Pair);"
"struct Pair host_drive(Relay,Leaf,struct Pair);"
"struct Pair host_take(Leaf,struct Pair);"
"struct Pair script_relay(Leaf leaf,struct Pair p){struct Pair result=host_take(leaf,p);p.n=999;return result;}"
"struct Pair entry(Leaf leaf,struct Pair p){return host_drive(script_relay,leaf,p);}";
static int bind(us_context *c,const char *dir,const char *name,void *target){
 char path[2048];unsigned char bytes[16384];snprintf(path,sizeof path,"%s/%s.sig",dir,name);FILE *f=fopen(path,"rb");if(!f)return 1;
 size_t n=fread(bytes,1,sizeof bytes,f);int bad=ferror(f)||!feof(f);fclose(f);return bad||us_add_symbol_typed(c,name,target,bytes,n);
}
static int fail(us_context *c,const char *step){fprintf(stderr,"%s: %s\n",step,us_error(c));retained=NULL;us_free(c);return 1;}
int main(int argc,char **argv){
 if(argc!=4)return 2;
 for(int opt=0;opt<3;opt++){
  us_context *c=us_new(argv[1]);if(!c)return 3;
  if(bind(c,argv[3],"host_drive",(void*)host_drive)||bind(c,argv[3],"host_take",(void*)host_take)||us_add_source(c,"forward.c",source)||us_compile(c,argv[2],opt)||us_relocate(c))return fail(c,"forward setup");
  Relay entry=(Relay)us_sym(c,"entry");if(!entry)return fail(c,"forward entry");
  int lc=leaf_calls,tc=take_calls,dc=drive_calls,nr=normal_returns;
  for(int i=0;i<100;i++){
   Leaf leaf=(i&1)?other_leaf:native_leaf;double d=(i&1)?18:17;int n=(i&1)?50:40;
   struct{uint64_t before;struct Pair input,output;uint64_t after;}guard={UINT64_C(0x0123456789abcdef),{15,35},{0,0},UINT64_C(0xfedcba9876543210)};
   guard.output=entry(leaf,guard.input);
   if(guard.output.d!=d||guard.output.n!=n||guard.input.d!=15||guard.input.n!=35||bad_native_copy||guard.before!=UINT64_C(0x0123456789abcdef)||guard.after!=UINT64_C(0xfedcba9876543210))return fail(c,"incoming Pair forwarded actual native ABI/canary");
   if(!retained)return fail(c,"retained Relay missing");
   struct Pair again=retained(leaf,guard.input);if(again.d!=d||again.n!=n||guard.input.d!=15||guard.input.n!=35||guard.output.d!=d||guard.output.n!=n||bad_native_copy||nesting)return fail(c,"retained Relay incoming aggregate/reentry copy");
   int status=-1;if(us_call_status(c,&status)||status||*us_error(c))return fail(c,"forward call status");
  }
  if(leaf_calls-lc!=200||take_calls-tc!=400||drive_calls-dc!=100||normal_returns-nr!=100)return fail(c,"forward boundary counts");
  retained=NULL;us_free(c);printf("O%d: incoming Pair forwarded entry/Relay->typed host_take, nested retained Relay reentry, native Leaf, two targets, caller copies/canaries, 100 repeats passed\n",opt);
 }
 return 0;
}
