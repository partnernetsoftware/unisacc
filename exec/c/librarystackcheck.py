#!/usr/bin/env python3
"""Actual model-produced script ALL_STACK entries; not full native FFI exports.
All compile/run subprocesses are bounded. Outer caller must use bound 55.
"""
import pathlib,sys,tempfile
from librarycallcheck import command,labels
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from unisa.tape import parse
from unisa.lower import lower
from unisa.__main__ import _oracle
from unisa.assemble import assemble
from unisa.image.macho import HDRS
SOURCE="""
struct Pair { double d; int n; };
double aggregate9(int a,double b,float c,long *p,struct Pair s,int e,double f,int g,int h){
  s.n=s.n+13;return a+b+c+*p+s.d+s.n+e+f+g+h;
}
double mixed9(int a,double b,float c,long *p,long e,double f,int g,int h,int i){
  return a+b+c+*p+e+f+g+h+i;
}
float float9(float a,float b,float c,float d,float e,float f,float g,float h,float i){
  return a+b*2+c*3+d*4+e*5+f*6+g*7+h*8+i*9;
}
long sum17(long a,long b,long c,long d,long e,long f,long g,long h,long i,
           long j,long k,long l,long m,long n,long o,long p,long q){
  return a+b*2+c*3+d*4+e*5+f*6+g*7+h*8+i*9+j*10+k*11+l*12+m*13+n*14+o*15+p*16+q*17;
}
long nested9(long a,long b,long c,long d,long e,long f,long g,long h,long i){
  long local[3];local[0]=a;local[1]=e;local[2]=i;
  return sum17(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17)+local[0]+local[1]+local[2];
}
int main(void){long n=4;struct Pair pair;pair.d=5.5;pair.n=6;
  return (int)mixed9(1,2.5,3.25,&n,5,6.5,7,8,9)+(int)float9(1,2,3,4,5,6,7,8,9)
   +(int)sum17(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17)
   +(int)nested9(1,2,3,4,5,6,7,8,9)+(int)aggregate9(1,2.5,3.25,&n,pair,7,8.75,9,10);
}
"""
HARNESS=r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librarycall.h"
extern unsigned char script_blob[];
#if defined(__aarch64__)
#define READSP(v) __asm__ volatile("mov %0, sp":"=r"(v))
#else
#define READSP(v) __asm__ volatile("movq %%rsp, %0":"=r"(v))
#endif
static uint64_t dbits(double v){uint64_t b;memcpy(&b,&v,8);return b;}
static uint64_t fbits(float v){uint32_t b;memcpy(&b,&v,4);return b;}
static int run(unsigned off,uint64_t *slots,void *top,size_t count,uint64_t *out){
 return us_library_call_stack(script_blob+off,slots,top,65536,count,out);
}
int main(void){
 unsigned char *stack=malloc(65536+32);uint64_t a[1024],r;unsigned k,t;
 if(!stack)return 2;
 void *top=(void*)(((uintptr_t)stack+65536)&~(uintptr_t)15);
 memset(stack,0xa5,65536+32);
 for(t=0;t<100;t++){
  uintptr_t before,after;READSP(before);
  int64_t p=4;
  a[0]=1;a[1]=dbits(2.5);a[2]=fbits(3.25);a[3]=(uintptr_t)&p;
  a[4]=5;a[5]=dbits(6.5);a[6]=7;a[7]=8;a[8]=9;
  if(!run(OFF_mixed9,a,top,9,&r)||r!=dbits(46.25))return 3;
  struct {double d;int n;} pair={5.5,6};
  a[0]=1;a[1]=dbits(2.5);a[2]=fbits(3.25);a[3]=(uintptr_t)&p;
  a[4]=(uintptr_t)&pair;a[5]=7;a[6]=dbits(8.75);a[7]=9;a[8]=10;
  if(!run(OFF_aggregate9,a,top,9,&r)||r!=dbits(70.0)||pair.d!=5.5||pair.n!=6)return 14;
  for(k=0;k<9;k++)a[k]=fbits((float)(k+1));
  if(!run(OFF_float9,a,top,9,&r)||(uint32_t)r!=(uint32_t)fbits(285.0f))return 4;
  for(k=0;k<17;k++)a[k]=k+1;
  if(!run(OFF_sum17,a,top,17,&r)||r!=1785)return 5;
  if(!run(OFF_nested9,a,top,9,&r)||r!=1800)return 6;
  READSP(after);if(before!=after||(after&15))return 7;
  for(k=0;k<64;k++)if(stack[k]!=0xa5)return 8;
 }
 r=0x1234;
 if(us_library_call_stack(script_blob,a,top,65536,1025,&r)||r!=0x1234)return 9;
 if(us_library_call_stack(script_blob,a,top,103,9,&r)||r!=0x1234)return 10;
 if(us_library_call_stack(script_blob,a,(char*)top-1,65536,9,&r)||r!=0x1234)return 11;
 if(us_library_call_stack(script_blob,0,top,65536,9,&r)||r!=0x1234)return 12;
 if(us_library_call_stack(0,a,top,65536,9,&r)||r!=0x1234)return 13;
 free(stack);puts("ALL_STACK actual model entries: mixed9 raw FP/pointer, aggregate9 by-value copy, float9, weighted17, nested9, 100 repeats, SP+canary and guards: ok");
 return 0;
}
'''
def check(ua,arch):
 with tempfile.TemporaryDirectory(prefix='unisacc-librarystack-') as name:
  d=pathlib.Path(name);source=d/'p.c';source.write_text(SOURCE);image=d/'image'
  target='osx/'+arch
  tape=command(['/bin/sh',ua,'-nostdinc','-b',target,'-O0','-S',source])
  command(['/bin/sh',ua,'-nostdinc','-b',target,'-O0','-o',image,source])
  tp=lower(parse(tape.decode()),target,_oracle('built'),drive='built',prune_input=True)
  ref,stats=assemble(tp);assert stats['encoded']==stats['insns']
  machine=image.read_bytes()[HDRS(arch):HDRS(arch)+len(ref)]
  assert machine==ref,'actual model text differs from independently assembled tape'
  offsets=labels(tp);blob=d/'script.bin';blob.write_bytes(machine)
  (d/'blob.S').write_text('.text\n.p2align 4\n.globl _script_blob\n_script_blob:\n.incbin "'+str(blob)+'"\n')
  defines='\n'.join('#define OFF_'+n+' '+str(offsets[n]) for n in ('mixed9','aggregate9','float9','sum17','nested9'))
  (d/'h.c').write_text(defines+'\n'+HARNESS)
  command(['cc','-arch',arch,'-O2','-Wall','-Wextra','-I',ROOT/'exec/c',d/'h.c',d/'blob.S',ROOT/'exec/c'/('librarycall_'+arch+'.S'),'-o',d/'h'])
  out=command(['/usr/bin/arch','-x86_64',d/'h'] if arch=='x86_64' else [d/'h'])
  print(arch,out.decode().strip())
if __name__=='__main__':
 if len(sys.argv)!=3:sys.exit('usage: librarystackcheck.py PRIVATE_MODEL_COM ARCH')
 check(pathlib.Path(sys.argv[1]).resolve(),sys.argv[2])
