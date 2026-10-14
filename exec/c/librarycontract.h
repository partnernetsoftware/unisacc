#ifndef UNISACC_LIBRARYCONTRACT_H
#define UNISACC_LIBRARYCONTRACT_H
/* 2a protocol adapter: canonical declarations only, never provider state.
   The finite librarycontract/gen stage validates and produces USLCON1. */
static unsigned us_contract_target(const char *target) {
 if(!target)return 0;
 if(!strcmp(target,"lnx/x86_64"))return 1;if(!strcmp(target,"lnx/arm64"))return 2;
 if(!strcmp(target,"osx/x86_64"))return 3;if(!strcmp(target,"osx/arm64"))return 4;return 0;
}
static void us_contract_put32(unsigned char *p,unsigned value){for(int i=0;i<4;i++)p[i]=(unsigned char)(value>>(8*i));}
static void us_contract_type(unsigned char *p,const us_binding_type *t){
 us_binding_put64(p,t->depth);us_binding_put64(p+8,0);us_binding_put64(p+16,0);
 us_binding_put64(p+24,t->kind);us_binding_put64(p+32,t->width);us_binding_put64(p+40,t->uns);
}
static int us_contract_compare(const void *a,const void *b){
 const us_binding *const *x=a,*const *y=b;return strcmp((*x)->name,(*y)->name);
}
static int us_contract_input(const us_bindings *injected,const us_bindings *declared,unsigned target,
 unsigned char **out,size_t *length,char *error,size_t capacity){
 *out=NULL;*length=0;
 if(!target||target>4||!injected||!declared||injected->count>4096||declared->count>4096)
  return us_binding_error(error,capacity,"library contract: invalid declaration registry or target");
 size_t count=0;const us_binding **items=malloc((injected->count+declared->count+1)*sizeof *items);
 if(!items)return us_binding_error(error,capacity,"library contract: allocation failed");
 for(unsigned group=0;group<2;group++){
  const us_bindings *b=group?declared:injected;
  for(size_t i=0;i<b->count;i++){
   const us_binding *x=b->items+i;
   if(!x->name||!us_binding_name(x->name)||x->kind>1||x->count>1024||x->variadic>1||(x->count&&!x->args&&!x->typed))goto bad;
   size_t j;for(j=0;j<count;j++)if(!strcmp(items[j]->name,x->name))break;
   if(j<count){if(!group||!us_resolver_declaration_equal(items[j],x))goto bad;continue;}
   if(count==4096)goto bad;items[count++]=x;
  }
 }
 qsort(items,count,sizeof *items,us_contract_compare);
 size_t total=16;
 for(size_t i=0;i<count;i++){
  const us_binding *x=items[i];size_t sig=x->typed?x->signature_length:48*(1+x->count),n=strlen(x->name);
  if(sig>1048576||(x->typed&&(!x->signature||sig<16))||total>INT_MAX-36-n-sig)goto bad;
  total+=36+n+sig;
 }
 unsigned char *bytes=malloc(total);if(!bytes){free(items);return us_binding_error(error,capacity,"library contract: allocation failed");}
 memcpy(bytes,"USLDEC1\n",8);bytes[8]=(unsigned char)target;bytes[9]=1;bytes[10]=bytes[11]=0;us_contract_put32(bytes+12,(unsigned)count);size_t at=16;
 for(size_t i=0;i<count;i++){
  const us_binding *x=items[i];size_t n=strlen(x->name),sig=x->typed?x->signature_length:48*(1+x->count);
  unsigned callkind=x->variadic?2:0,sigv=0;
  if(x->typed){sigv=!memcmp(x->signature,"USLSIG3\n",8)?3:!memcmp(x->signature,"USLSIG2\n",8)?2:0;
   if(!sigv){free(bytes);goto bad;}if(!x->variadic)callkind=us_export_has_callbacks(x->typed->items)?3:1;}
  us_contract_put32(bytes+at,(unsigned)(32+n+sig));us_contract_put32(bytes+at+4,(unsigned)(256+i));
  bytes[at+8]=(unsigned char)x->kind;bytes[at+9]=(unsigned char)callkind;bytes[at+10]=(unsigned char)sigv;bytes[at+11]=(unsigned char)(x->kind&&x->writable?1:0);
  us_contract_put32(bytes+at+12,(unsigned)n);us_contract_put32(bytes+at+16,(unsigned)sig);
  us_binding_put64(bytes+at+20,x->kind?x->extent:0);us_contract_put32(bytes+at+28,(unsigned)x->count);
  bytes[at+32]=(unsigned char)x->variadic;bytes[at+33]=bytes[at+34]=bytes[at+35]=0;at+=36;
  memcpy(bytes+at,x->name,n);at+=n;
  if(x->typed){memcpy(bytes+at,x->signature,sig);at+=sig;}
  else {us_contract_type(bytes+at,&x->result);at+=48;for(size_t j=0;j<x->count;j++){us_contract_type(bytes+at,x->args+j);at+=48;}}
 }
 free(items);*out=bytes;*length=at;return 0;
bad:free(items);return us_binding_error(error,capacity,"library contract: malformed or over-limit declaration registry");
}
#endif
