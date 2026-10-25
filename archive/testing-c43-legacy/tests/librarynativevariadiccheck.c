#include "exec/c/librarynative.h"
#include <stdarg.h>
struct Pair { double d; int n; };
static unsigned calls;
static double mixed(double first,int count,...) {
    va_list ap;va_start(ap,count);double r=first;
    for(int i=0;i<count;i++){int n=va_arg(ap,int);double d=va_arg(ap,double);r+=n+d;}
    va_end(ap);calls++;return r;
}
static struct Pair pair(int count,...) {
    va_list ap;va_start(ap,count);struct Pair p=va_arg(ap,struct Pair);
    p.d+=va_arg(ap,double);p.n+=va_arg(ap,int);va_end(ap);calls++;return p;
}
int main(int argc,char **argv) {
    if(argc!=4)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<1)return 2;
    unsigned char *sig=malloc((size_t)n);if(!sig)return 2;
    if(fread(sig,1,(size_t)n,f)!=(size_t)n||fclose(f))return 2;
    uint64_t fixed=strtoull(argv[3],NULL,10),handle=99;
    us_native_plans plans={0};char error[200]={0};
    int rc=us_native_plan_add_variadic(&plans,(uintptr_t)(!strcmp(argv[2],"pair") ? (void*)pair:(void*)mixed),sig,(size_t)n,fixed,&handle,error,sizeof error);
    free(sig);
    if(!strcmp(argv[2],"reject")){
        if(!rc||handle||plans.head){us_native_plans_clear(&plans);return 1;}
        puts(error);return 0;
    }
    if(rc||!handle||!plans.head){fprintf(stderr,"%s\n",error);return 1;}
    uint64_t slots[20]={0},word=0;double first=.5;
    struct Pair source={7.25,11},result={0};size_t count=plans.head->graph.items[0].count;
    if(!strcmp(argv[2],"pair")){
        slots[0]=1;slots[1]=(uintptr_t)&source;double d=2.5;memcpy(slots+2,&d,8);slots[3]=3;
    }else{
        memcpy(slots,&first,8);slots[1]=count==2 ? 0:9;
        for(size_t i=0;i<9&&count>2;i++){slots[2+2*i]=i+1;double d=(i+1)*.25;memcpy(slots+3+2*i,&d,8);}
    }
    for(unsigned i=0;i<100;i++){
        us_native_arena *a=NULL;
        if(us_native_prepare(plans.head,slots,count,!strcmp(argv[2],"pair") ? (void*)&result:(void*)&word,&a,error,sizeof error))return 1;
        if(us_native_invoke(a))return 1;us_native_arena_free(a);
    }
    if(calls!=100||source.d!=7.25||source.n!=11)return 1;
    if(!strcmp(argv[2],"pair")){if(result.d!=9.75||result.n!=14)return 1;}
    else{double value;memcpy(&value,&word,8);if(value!=(count==2 ? .5:56.75))return 1;}
    us_native_plans_clear(&plans);if(plans.head)return 1;
    puts("variadic native plan: 100 real calls, exact values and ownership passed");return 0;
}
