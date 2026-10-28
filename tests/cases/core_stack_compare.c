/* Compare ownership-independent stack and frame behavior to the C core. */
#include "../../exec/c/core.c"
#include <stdio.h>
void core_stack_push(CoreStack *,int);void core_stack_pop(CoreStack *);
void core_frame_push(CoreFrames *,const CoreFrame *);void core_frame_pop(CoreFrames *);
void core_host_panic(const char *s){fprintf(stderr,"panic %s\n",s);abort();}
int main(void){
 CoreStack a={0},b={0};CoreFrames x={0},y={0};unsigned char input[]={1,2};I attr[]={3,4};CoreFrame f={input,attr,0,2};int failed=0;
 for(int i=0;i<2050;i++){stack_push(&a,i);core_stack_push(&b,i);if(a.n!=b.n||a.cap!=b.cap||a.entries[i]!=b.entries[i])failed++;}
 for(int i=0;i<2050;i++){stack_pop(&a);core_stack_pop(&b);if(a.n!=b.n)failed++;}
 for(int i=0;i<33;i++){f.i=i;frame_push(&x,&f);core_frame_push(&y,&f);if(x.n!=y.n||x.cap!=y.cap||memcmp(&x.entries[i],&y.entries[i],sizeof f))failed++;}
 for(int i=0;i<40;i++){frame_pop(&x);core_frame_pop(&y);if(x.n!=y.n)failed++;}
 free(a.entries);free(b.entries);free(x.entries);free(y.entries);
 printf("CORE_STACK_COMPARE checks=4173 failed=%d\n",failed);return failed?1:0;
}
