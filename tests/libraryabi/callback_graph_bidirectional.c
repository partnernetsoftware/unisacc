/* Actual callback values, not a host global populated from us_sym. */
#include "libunisacc.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
struct Pair {double d;int n;};
typedef struct Pair (*Leaf)(int,double,float,int*,struct Pair,int,double,int,int);
typedef struct Pair (*Relay)(Leaf);
static Relay retained;
static int leaf_calls,drive_calls,normal_returns;
static struct Pair native_leaf(int a,double b,float c,int *p,struct Pair s,int e,double f,int g,int h){
 leaf_calls++;struct Pair r={b+c+s.d+f,a+*p+s.n+e+g+h};return r;
}
static struct Pair other_leaf(int a,double b,float c,int *p,struct Pair s,int e,double f,int g,int h){
 struct Pair r=native_leaf(a,b,c,p,s,e,f,g,h);r.d+=1;r.n+=10;return r;
}
static struct Pair host_drive(Relay relay,Leaf leaf){
 drive_calls++;retained=relay;struct Pair r=relay(leaf);normal_returns++;return r;
}
static const char source[]=
"struct Pair {double d;int n;};"
"typedef struct Pair (*Leaf)(int,double,float,int*,struct Pair,int,double,int,int);"
"typedef struct Pair (*Relay)(Leaf);"
"struct Pair host_drive(Relay,Leaf);"
"struct Pair script_relay(Leaf leaf){int n=12;struct Pair s={4.0,5};return leaf(2,1.5,2.5f,&n,s,6,9.0,7,8);}"
"struct Pair entry(Leaf leaf){return host_drive(script_relay,leaf);}";
static int fail(us_context *c,const char *step){fprintf(stderr,"%s: %s\n",step,us_error(c));us_free(c);return 1;}
static int bind(us_context *c,const char *dir){char path[2048];unsigned char bytes[16384];snprintf(path,sizeof path,"%s/host_drive.sig",dir);FILE *f=fopen(path,"rb");if(!f)return 1;size_t n=fread(bytes,1,sizeof bytes,f);int bad=ferror(f)||!feof(f);fclose(f);return bad||us_add_symbol_typed(c,"host_drive",(void*)host_drive,bytes,n);}
int main(int argc,char **argv){
 if(argc!=4)return 2;
 for(int opt=0;opt<3;opt++){
  us_context *c=us_new(argv[1]);if(!c)return 3;
  if(bind(c,argv[3]))return fail(c,"callback graph registration");
  if(us_add_source(c,"callbacks.c",source)||us_compile(c,argv[2],opt)||us_relocate(c))return fail(c,"callback graph compile/relocate");
  Relay entry=(Relay)us_sym(c,"entry");if(!entry)return fail(c,"entry typed closure");
  int before_leaf=leaf_calls,before_drive=drive_calls,before_normal=normal_returns;
  for(int i=0;i<100;i++){
   Leaf leaf=(i&1)?other_leaf:native_leaf;
   struct {uint64_t a;struct Pair p;uint64_t b;} guard={UINT64_C(0x123456789abcdef),{0,0},UINT64_C(0xfedcba9876543210)};
   guard.p=entry(leaf);double d=(i&1)?18.0:17.0;int n=(i&1)?50:40;
   if(guard.p.d!=d||guard.p.n!=n||guard.a!=UINT64_C(0x123456789abcdef)||guard.b!=UINT64_C(0xfedcba9876543210))return fail(c,"callback graph actual ABI/canary");
   if(!retained)return fail(c,"retained callback missing");struct Pair again=retained(leaf);
   if(again.d!=d||again.n!=n||guard.p.d!=d||guard.p.n!=n)return fail(c,"retained callback/by-value copy");
   int status=-1;if(us_call_status(c,&status)||status!=0)return fail(c,"callback status");
  }
  if(leaf_calls-before_leaf!=200||drive_calls-before_drive!=100||normal_returns-before_normal!=100)return fail(c,"real boundary counts");
  printf("O%d: native->script->native->script Relay->native Leaf9; two target addresses, Pair return, retained callback, 100 repeats: ok\n",opt);
  retained=NULL;us_free(c);
 }
 return 0;
}
