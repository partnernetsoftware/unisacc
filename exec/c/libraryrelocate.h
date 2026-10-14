/* Protocol-only relocation adapter: ownership is a model decision. */
#ifndef UNISA_LIBRARY_RELOCATE_H
#define UNISA_LIBRARY_RELOCATE_H
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef int (*UsLibraryProvider)(void *,uint32_t,uint64_t *);
static uint32_t us_lr_u32(const unsigned char *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint64_t us_lr_u64(const unsigned char *p){return (uint64_t)us_lr_u32(p)|((uint64_t)us_lr_u32(p+4)<<32);}
static int us_lr_bad(char *e,size_t n){snprintf(e,n,"unisacc: relocate: malformed relocation");return 1;}
/* Success owns *out; failures always leave NULL/0. Provider cannot alter ownership. */
static int us_library_relocate(const unsigned char *f,size_t fn,const unsigned char *owners,size_t on,UsLibraryProvider provider,void *context,unsigned char **out,size_t *outn,char *error,size_t errorn){
 *out=NULL;*outn=0;
 if(!f||!owners||fn<20||on<12||memcmp(f,"UNILOW1\n",8)||memcmp(owners,"USLSEL1\n",8))return us_lr_bad(error,errorn);
 uint64_t tn64=us_lr_u64(f+8);uint32_t count=us_lr_u32(f+16),oc=us_lr_u32(owners+8);
 if(tn64>(uint64_t)(fn-20)||oc>4096||oc>(on-12)/5||on-12!=(size_t)oc*5)return us_lr_bad(error,errorn);
 size_t tn=(size_t)tn64,tail=fn-20-tn;
 if(count>tail/20||tail!=(size_t)count*20)return us_lr_bad(error,errorn);
 for(uint32_t i=0;i<oc;i++){
  const unsigned char *p=owners+12+5*(size_t)i;uint32_t id=us_lr_u32(p);
  if(id!=256+i||(p[4]!=1&&p[4]!=2))return us_lr_bad(error,errorn);
 }
 const unsigned char *text=f+20,*records=text+tn;uint64_t previous=0;
 for(uint32_t i=0;i<count;i++){
  const unsigned char *p=records+20*(size_t)i;uint64_t off=us_lr_u64(p);uint32_t id=us_lr_u32(p+12);
  if(us_lr_u32(p+8)!=1||off>=tn||(i&&off<previous)||text[(size_t)off]!='0'||p[16]||p[17]||p[18]||p[19])return us_lr_bad(error,errorn);
  previous=off+1;int found=0;
  for(uint32_t j=0;j<oc;j++){const unsigned char *q=owners+12+5*(size_t)j;if(us_lr_u32(q)==id){if(q[4]!=1)return us_lr_bad(error,errorn);found=1;break;}}
  if(!found)return us_lr_bad(error,errorn);
 }
 if((size_t)count>((size_t)-1)/sizeof(uint64_t))return us_lr_bad(error,errorn);
 uint64_t *addresses=count?malloc((size_t)count*sizeof(uint64_t)):NULL;
 if(count&&!addresses)return us_lr_bad(error,errorn);
 size_t total=tn;char digits[32];
 for(uint32_t i=0;i<count;i++){
  uint32_t id=us_lr_u32(records+20*(size_t)i+12);uint64_t address=0;
  int frozen=0;
  for(uint32_t j=0;j<i;j++)if(us_lr_u32(records+20*(size_t)j+12)==id){address=addresses[j];frozen=1;break;}
  if(!frozen&&(!provider||provider(context,id,&address)||!address)){free(addresses);snprintf(error,errorn,"unisacc: relocate: missing-provider symbol %u",id);return 1;}
  addresses[i]=address;int n=snprintf(digits,sizeof digits,"%llu",(unsigned long long)address);
  if(n<1||(size_t)n>=sizeof digits||total==(size_t)-1||(size_t)(n-1)>((size_t)-1)-total-1){free(addresses);return us_lr_bad(error,errorn);}total+=(size_t)n-1;
 }
 if(total==(size_t)-1){free(addresses);return us_lr_bad(error,errorn);}
 unsigned char *result=malloc(total+1);if(!result){free(addresses);return us_lr_bad(error,errorn);}memcpy(result,text,tn);size_t used=tn;
 /* Reverse replacement preserves all original offsets and non-span bytes. */
 for(uint32_t i=count;i;i--){size_t k=i-1,off=(size_t)us_lr_u64(records+20*k);int n=snprintf(digits,sizeof digits,"%llu",(unsigned long long)addresses[k]);memmove(result+off+n,result+off+1,used-off-1);memcpy(result+off,digits,(size_t)n);used+=(size_t)n-1;}
 result[used]=0;free(addresses);*out=result;*outn=used;return 0;
}
#endif
