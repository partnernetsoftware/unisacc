/* LB-1: host adapter only. No Tape/C parser, no model decision or ABI claim. */
#ifndef UNISA_HOST_BIND_H
#define UNISA_HOST_BIND_H
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef struct { unsigned target, count, indirect, mask; } HostRequirements;
typedef struct { unsigned target, kind, available; uint64_t address[4]; } HostProvider;
typedef struct { unsigned target, kind, mask; uint64_t address[4]; } HostBindPlan;
static int host_bind_error(char *error,size_t cap,const char *reason){snprintf(error,cap,"unisacc: bind: %s",reason);return 1;}
static unsigned host_target_id(const char *target){
 if(!strcmp(target,"lnx/x86_64"))return 1;if(!strcmp(target,"lnx/arm64"))return 2;
 if(!strcmp(target,"osx/x86_64"))return 3;if(!strcmp(target,"osx/arm64"))return 4;return 0;
}
static int host_requirements(const unsigned char *b,size_t n,HostRequirements *r,char *error,size_t cap){
 memset(r,0,sizeof *r);
 if(n<12||memcmp(b,"UNISYM1\n",8)||b[8]<1||b[8]>4||b[9]||b[10]>4||b[11]>1||n!=12+8*(size_t)b[10])return host_bind_error(error,cap,"invalid-contract framing");
 r->target=b[8];r->count=b[10];r->indirect=b[11];int previous=-1;
 for(unsigned i=0;i<r->count;i++){
  const unsigned char *p=b+12+8*i;unsigned slot=p[4];
  if(slot>3||(int)slot<=previous||p[0]!=slot+1||p[1]||p[2]||p[3]||p[5]!=1||p[6]||p[7])return host_bind_error(error,cap,"invalid-contract record");
  previous=(int)slot;r->mask|=1u<<slot;
 }
 return 0;
}
/* Exact 12B capability filter. Cannot manufacture capability or an address. */
static int host_provider_filter(const unsigned char*b,size_t n,unsigned target,unsigned *mask,char*error,size_t cap){
 if(n!=12||memcmp(b,"UHPROV1\n",8))return host_bind_error(error,cap,"invalid-provider framing");
 if(b[8]!=target)return host_bind_error(error,cap,"provider-target differs");
 if(b[9]&240)return host_bind_error(error,cap,"invalid-provider mask");
 if(b[10]||b[11])return host_bind_error(error,cap,"provider-reserved bits are nonzero");
 *mask=b[9];return 0;
}
static int host_bind(const HostRequirements*r,const HostProvider*p,unsigned filter,HostBindPlan*out,char*error,size_t cap){
 memset(out,0,sizeof *out);
 if(!r->target||p->target!=r->target||p->kind<1||p->kind>2||(p->available&~15u)||(filter&~15u))return host_bind_error(error,cap,"invalid-provider backend");
 unsigned available=p->available&filter;
 for(unsigned slot=0;slot<4;slot++)if(r->mask&(1u<<slot)){
  if(!(available&(1u<<slot))||(p->kind==2&&!p->address[slot])){snprintf(error,cap,"unisacc: bind: missing-provider symbol %u slot %u",slot+1,slot);return 1;}
 }
 out->target=r->target;out->kind=p->kind;out->mask=r->mask;
 for(unsigned slot=0;slot<4;slot++)out->address[slot]=p->address[slot];
 return 0;
}
#endif
