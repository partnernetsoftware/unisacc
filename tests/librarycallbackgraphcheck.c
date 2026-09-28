/* Graph decoder ownership/equality only: deliberately no executable callback ABI. */
#include "exec/c/librarycallplans.h"
static unsigned char *readfile(const char *path,size_t *length){
    FILE *f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);rewind(f);
    if(n<0){fclose(f);return NULL;}unsigned char *b=malloc(n ? (size_t)n:1);
    if(!b || fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return NULL;}
    fclose(f);*length=(size_t)n;return b;
}
int main(int argc,char **argv){
    if(argc<4)return 2;
    size_t n=0,m=0;unsigned char *b=readfile(argv[2],&n),*c=readfile(argv[3],&m);
    us_exports a={0},other={0};char error[160]={0};int rc=1;
    if(!b||!c||us_exports_load(&a,b,n,error,sizeof error)||us_exports_load(&other,c,m,error,sizeof error))goto done;
    if(a.count!=1||other.count!=1||us_export_supported(a.items))goto done;
    us_export *x=a.items;us_export_signature *s=x->argtypes[0].signature;
    if(!strcmp(argv[1],"self")){
        if(x->graph.count!=1||!s||s->count!=1||s->argtypes[0].signature!=s)goto done;
    }else if(!strcmp(argv[1],"shared")){
        if(x->graph.count!=1||!s||x->argtypes[1].signature!=s)goto done;
    }else if(!strcmp(argv[1],"mutual")){
        if(x->graph.count!=2||!s||!s->argtypes[0].signature||s->argtypes[0].signature->argtypes[0].signature!=s)goto done;
    }else if(!strcmp(argv[1],"nine")){
        if(x->graph.count!=2||!s||s->argtypes[0].signature->count!=9||s->argtypes[0].signature->argtypes[8].kind!=1)goto done;
    }else if(!strcmp(argv[1],"limit")){if(x->graph.count!=1024)goto done;}
    else if(strcmp(argv[1],"empty")&&strcmp(argv[1],"unequal")&&strcmp(argv[1],"depth")&&strcmp(argv[1],"equal"))goto done;
    if(x->argtypes[0].ffi || us_export_ffitype(&x->argtypes[0],0))goto done;
    uint64_t handle=99;us_native_plans plans={0};
    if(us_native_plan_add(&plans,1,b,n,&handle,error,sizeof error)||handle||plans.head)goto done;
    us_native_plans_clear(&plans);
    if(us_native_type_equal(&x->argtypes[0],&other.items[0].argtypes[0])==!strcmp(argv[1],"unequal"))goto done;
    us_export *old=a.items;
    for(size_t i=0;i<n;i+=(n<4096 ? 1:n/64))if(!us_exports_load(&a,b,i,error,sizeof error)||a.items!=old)goto done;
    for(int i=4;i<argc;i++){
        size_t z;unsigned char *bad=readfile(argv[i],&z);if(!bad)goto done;
        int failed=us_exports_load(&a,bad,z,error,sizeof error);free(bad);
        if(!failed||a.items!=old||a.count!=1)goto done;
    }
    rc=0;printf("callback graph %s: owned decode, bounded equality, unsupported execution, atomic rollback passed\n",argv[1]);
done:if(rc)fprintf(stderr,"callback graph failed: %s (%s)\n",argv[1],error);
    us_exports_clear(&a);us_exports_clear(&other);free(b);free(c);return rc;
}
