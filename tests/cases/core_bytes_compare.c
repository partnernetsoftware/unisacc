/* Compare the original C primitive and the separately linked CX unit. */
#include "../../exec/c/core.c"
#include <stdio.h>
int core_blob_add(CoreBlobs *,const unsigned char *,int);
I core_string_intern(CoreIntern *,const unsigned char *,int);
int core_resource_find(CoreResources *,CoreBlobs *,const unsigned char *,int);
void core_host_panic(const char *s){fprintf(stderr,"panic %s\n",s);abort();}
int core_host_fetch(const unsigned char *key,int n,unsigned char **bytes,int *len){
 static unsigned char borrowed[]={9,0,7};*bytes=0;*len=0;
 if(n==1&&key[0]=='b'){*bytes=borrowed;*len=3;return 1;}
 if(n==1&&key[0]=='e'){*bytes=borrowed;return 1;}
 if(n==1&&key[0]=='o'){*bytes=malloc(2);(*bytes)[0]=5;(*bytes)[1]=6;*len=2;return 2;}return 0;
}
static void clear(CoreResources *r,CoreBlobs *b,CoreIntern *t){
 for(int i=0;i<r->n;i++)free(r->entries[i].p);free(r->entries);
 for(int i=0;i<b->n;i++)free(b->entries[i].b);free(b->entries);
 for(size_t i=0;i<t->cap;i++)free(t->entries[i].b);free(t->entries);
}
int main(void){
 CoreBlobs a={0},b={0};CoreResources x={0},y={0};CoreIntern i={0},j={0};int failed=0,checks=0;unsigned char empty[]={0};
 int za=core_badd(&a,empty,0),zb=core_blob_add(&b,empty,0);checks++;if(za!=0||zb!=0||!a.entries[0].b||!b.entries[0].b)failed++;
 const unsigned char *keys[]={(const unsigned char *)"b",(const unsigned char *)"e",(const unsigned char *)"o",(const unsigned char *)"x",(const unsigned char *)"b"};
 for(int k=0;k<5;k++){
  int u=core_rfind(&x,&a,keys[k],1),v=core_resource_find(&y,&b,keys[k],1);checks++;
  if(u!=v||x.n!=y.n||a.n!=b.n||(u&&a.entries[u].n!=b.entries[v].n)||(u&&memcmp(a.entries[u].b,b.entries[v].b,a.entries[u].n)))failed++;
 }
 for(int k=0;k<600;k++){unsigned char key[2]={(unsigned char)k,(unsigned char)(k>>8)};I u=core_intern(&i,key,2),v=core_string_intern(&j,key,2);checks++;if(u!=v||i.n!=j.n||i.cap!=j.cap)failed++;}
 I u=core_intern(&i,empty,0),v=core_string_intern(&j,empty,0);checks++;if(u!=v||i.n!=j.n||i.cap!=j.cap)failed++;
 clear(&x,&a,&i);clear(&y,&b,&j);printf("CORE_BYTES_COMPARE checks=%d failed=%d\n",checks,failed);return failed?1:0;
}
