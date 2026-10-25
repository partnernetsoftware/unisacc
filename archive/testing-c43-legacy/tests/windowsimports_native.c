/* Actual Windows OS named-export resource enumeration. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {const unsigned char *name;int n;const unsigned char *data;int len;} ResourceInput;
#include "../exec/c/librarywinimports.h"
int main(void){LibraryWinImports x={0};if(library_winimports_init(&x))return 1;
 if(!x.count)return 2;
 static const char prefix[]="\0process/import/kernel32.dll/";
 HMODULE module=GetModuleHandleA("kernel32.dll");
 for(int i=0;i<x.count;i++){
  ResourceInput *r=x.rows+i;int at=sizeof(prefix)-1;
  if(r->n<=at||memcmp(r->name,prefix,at)||r->len!=8)return 3;
  char *name=malloc(r->n-at+1);if(!name)return 4;
  memcpy(name,r->name+at,r->n-at);name[r->n-at]=0;
  uint64_t value=0;for(int k=0;k<8;k++)value|=(uint64_t)r->data[k]<<(k*8);
  uintptr_t actual=(uintptr_t)GetProcAddress(module,name);free(name);
  if(!actual||value!=(uint64_t)actual)return 5;
 }
 printf("Windows OS exports: %d exact 64-bit resources\n",x.count);
 library_winimports_free(&x);if(x.rows||x.count)return 6;return 0;
}
