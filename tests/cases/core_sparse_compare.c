/* Verify representation as well as values, including zero and overwrite. */
#include "../../exec/c/core.c"
#include <stdio.h>
I core_memory_get(const CoreMemory *,I);void core_memory_set(CoreMemory *,I,I);
void core_host_panic(const char *s){fprintf(stderr,"panic %s\n",s);abort();}
int main(void){
 CoreMemory a={0},b={0};int failed=0;long checks=0;
 for(int i=0;i<32768;i++){
  I key=(I)i*37-500000,value=(i%5)?(I)i:0;core_mset(&a,key,value);core_memory_set(&b,key,value);checks++;
  if(a.n!=b.n||a.cap!=b.cap||core_mget(&a,key)!=core_memory_get(&b,key))failed++;
 }
 core_mset(&a,-500000,0);core_memory_set(&b,-500000,0);checks++;
 if(a.n!=32768||b.n!=32768||a.cap!=131072||b.cap!=131072)failed++;
 if(memcmp(a.used,b.used,a.cap)||memcmp(a.keys,b.keys,a.cap*sizeof(I))||memcmp(a.values,b.values,a.cap*sizeof(I)))failed++;
 for(int i=0;i<32768;i++){I key=(I)i*37-500000;checks++;if(core_mget(&a,key)!=core_memory_get(&b,key))failed++;}
 checks++;if(core_mget(&a,INT64_MIN)!=core_memory_get(&b,INT64_MIN))failed++;
 free(a.keys);free(a.values);free(a.used);free(b.keys);free(b.values);free(b.used);
 printf("CORE_SPARSE_COMPARE checks=%ld failed=%d\n",checks,failed);return failed?1:0;
}
