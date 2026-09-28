/* Actual model-produced indirect concrete sites, not a mock script frame. */
#include "libunisacc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
struct Pair {double d;int n;};
typedef struct Pair (*Leaf)(double,int,struct Pair);
typedef struct Pair (*Var)(double,int,...);
static int mixed,empty,normal_returns;
static struct Pair host_exchange(double x,int mode,...){
 if(!mode){empty++;normal_returns++;return (struct Pair){x,0};}
 va_list ap;va_start(ap,mode);double y=va_arg(ap,double);int k=va_arg(ap,int);
 struct Pair p=va_arg(ap,struct Pair);Leaf leaf=va_arg(ap,Leaf);
 for(int i=0;i<7;i++)y+=va_arg(ap,double);va_end(ap);
 struct Pair out=leaf(x+y,k,p);mixed++;normal_returns++;return out;
}
static struct Pair other_exchange(double x,int mode,...){
 if(!mode){empty++;normal_returns++;return (struct Pair){x,0};}
 va_list ap;va_start(ap,mode);double y=va_arg(ap,double);int k=va_arg(ap,int);
 struct Pair p=va_arg(ap,struct Pair);Leaf leaf=va_arg(ap,Leaf);
 for(int i=0;i<7;i++)y+=va_arg(ap,double);va_end(ap);
 struct Pair out=leaf(x+y,k,p);out.d+=1;out.n+=10;mixed++;normal_returns++;return out;
}
static const char source[]=
 "#include <stdlib.h>\nstruct Pair{double d;int n;};"
 "typedef struct Pair (*Leaf)(double,int,struct Pair);"
 "typedef struct Pair (*Var)(double,int,...);"
 "struct Pair host_exchange(double,int,...);"
 "struct Pair leaf(double x,int k,struct Pair p){struct Pair r={x+p.d,k+p.n};return r;}"
 "float tailvalue(Var f){struct Pair z=f(2.5,0);return z.d;}"
 "struct Pair entry(Var f){struct Pair p={4.0,5};signed char k=7;"
 "return f(1.5,1,tailvalue(f),k,p,leaf,1.0,2.0,3.0,4.0,5.0,6.0,7.0);}"
 "struct Pair native_case(void){Var f=host_exchange;return entry(f);}"
 "struct Pair leaf_fail(double x,int k,struct Pair p){exit(23);return p;}"
 "struct Pair fail_case(Var f){struct Pair p={4.0,5};return f(1.5,1,2.5,7,p,leaf_fail,1.0,2.0,3.0,4.0,5.0,6.0,7.0);}"
 "struct Pair scriptvar(double x,int n,...){struct Pair p={x+1.0,n+2};return p;}"
 "struct Pair script_case(void){Var f=scriptvar;return f(4.25,3,1.0);}"
 "struct Pair nested_script_case(void){Var f=scriptvar;return f(tailvalue(f),3,1.0);}";
static int bind(us_context *c,const char *dir){
 char path[2048];unsigned char bytes[16384];snprintf(path,sizeof path,"%s/host_exchange.sig",dir);
 FILE *f=fopen(path,"rb");if(!f)return 1;size_t n=fread(bytes,1,sizeof bytes,f);int bad=ferror(f)||!feof(f);fclose(f);
 return bad||us_add_symbol_typed(c,"host_exchange",(void*)host_exchange,bytes,n);
}
static int fail(us_context *c,const char *step){fprintf(stderr,"%s: %s\n",step,us_error(c));us_free(c);return 1;}
int main(int argc,char **argv){
 if(argc!=4)return 2;
 for(int opt=0;opt<3;opt++){
  us_context *c=us_new(argv[1]);if(!c)return 3;
  if(bind(c,argv[3])||us_add_source(c,"callable-var.c",source)||us_compile(c,argv[2],opt)||us_relocate(c))return fail(c,"callable var setup");
  struct Pair (*entry)(Var)=(struct Pair(*)(Var))us_sym(c,"entry");
  struct Pair (*native_case)(void)=(struct Pair(*)(void))us_sym(c,"native_case");
  struct Pair (*script_case)(void)=(struct Pair(*)(void))us_sym(c,"script_case");
  struct Pair (*nested)(void)=(struct Pair(*)(void))us_sym(c,"nested_script_case");
  struct Pair (*failure_case)(Var)=(struct Pair(*)(Var))us_sym(c,"fail_case");
  if(!entry||!native_case||!script_case||!nested||!failure_case)return fail(c,"callable var symbols");
  int m=mixed,e=empty,r=normal_returns;
  for(int i=0;i<100;i++){
   struct {uint64_t a;struct Pair p;uint64_t z;} g={123,{0,0},456};
   g.p=entry((i&1)?other_exchange:host_exchange);
   if(g.p.d!=36+(i&1)||g.p.n!=12+10*(i&1)||g.a!=123||g.z!=456)return fail(c,"incoming var pointer/promoted tail/Pair/fixed script callback");
   g.p=native_case();if(g.p.d!=36||g.p.n!=12)return fail(c,"frozen native template callable introduction");
   g.p=script_case();if(g.p.d!=5.25||g.p.n!=5)return fail(c,"concrete script var handle");
   g.p=nested();if(g.p.d!=4.5||g.p.n!=5)return fail(c,"nested concrete script sites");
   int status=-1;if(us_call_status(c,&status)||status||*us_error(c))return fail(c,"callable var status");
  }
  if(mixed-m!=200||empty-e!=200||normal_returns-r!=400)return fail(c,"actual native boundary counts");
  struct {uint64_t a;struct Pair p;uint64_t z;} guard={123,{99,88},456};
  int before=normal_returns;guard.p=failure_case(host_exchange);int status=-1;
  if(!us_call_status(c,&status)||status!=23||guard.p.d!=0||guard.p.n!=0||guard.a!=123||guard.z!=456||normal_returns!=before+1)return fail(c,"variadic callback failure normal native return/ABI zero/canary");
  guard.p=native_case();if(guard.p.d!=36||guard.p.n!=12||us_call_status(c,&status)||status||*us_error(c))return fail(c,"independent variadic recovery after callback failure");
  us_free(c);printf("O%d: concrete indirect native/script varargs, nested sites, promoted float/char, Pair and fixed callback, two native targets, 100 repeats passed\n",opt);
 }
 return 0;
}
