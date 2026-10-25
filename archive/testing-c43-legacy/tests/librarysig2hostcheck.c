/* Native closure calls through model-declared raw frames; no source parsing. */
#include "exec/c/libraryexports.h"
struct Pair { double d; int n; };
static int calls;
static int lookup(void *o,const char *n,const void **p,int *k){(void)o;(void)n;*p=(void*)1;*k=0;return 0;}
static int invoke(void *o,const void *raw,const us_export_frame *f){
    (void)o;(void)raw;calls++;
    if(f->result_kind==1){uint64_t sum=0;for(size_t i=0;i<(size_t)f->count;i++)sum+=f->slots[i];memcpy(f->result,&sum,8);return 0;}
    if(f->count!=9||f->mode!=1||f->result_kind!=5||f->result_bytes!=sizeof(struct Pair))return 1;
    double b,d;float c;memcpy(&b,&f->slots[1],8);memcpy(&c,&f->slots[2],4);memcpy(&d,&f->slots[6],8);
    struct Pair *s=(void*)(uintptr_t)f->slots[4];
    if(s->d!=5.0||s->n!=6)return 1;
    struct Pair r={b+c+s->d+d,(int)f->slots[0]+*(int*)(uintptr_t)f->slots[3]+s->n+(int)f->slots[5]+(int)f->slots[7]+(int)f->slots[8]};
    s->n=999; /* mutable owned argument must not affect native caller */
    memcpy(f->result,&r,sizeof r);return 0;
}
static int legacy(void *o,const void *r,const uint64_t slots[6],uint64_t *out){(void)o;(void)r;calls++;*out=slots[0];return 0;}
int main(int argc,char **argv){
    if(argc!=2 && argc!=3)return 2;FILE *fp=fopen(argv[1],"rb");if(!fp)return 2;
    fseek(fp,0,SEEK_END);long z=ftell(fp);rewind(fp);unsigned char *b=malloc((size_t)z);if(!b)return 2;
    if(fread(b,1,(size_t)z,fp)!=(size_t)z)return 2;fclose(fp);
    char err[160];us_exports set={0};
    if(us_exports_load(&set,b,(size_t)z,err,sizeof err)){fprintf(stderr,"%s\n",err);return 1;}
    if(argc==3 && !strcmp(argv[2],"decode")){us_exports_clear(&set);free(b);puts("sig2 nested layout decoded");return 0;}
    void *code=argc==3 && !strcmp(argv[2],"legacy") ? us_exports_symbol(&set,"exchange",NULL,lookup,legacy,err,sizeof err) : us_exports_symbol_frame(&set,"exchange",NULL,lookup,invoke,err,sizeof err);
    if(!code){fprintf(stderr,"%s\n",err);return 1;}
    if(argc==3){
        us_export *x=&set.items[0];size_t n=(size_t)x->count;
        int *values=calloc(n,sizeof *values);void **ptrs=calloc(n,sizeof *ptrs);ffi_arg answer=0;
        for(size_t i=0;i<n;i++){values[i]=1;ptrs[i]=&values[i];}
        ffi_call(&x->cif,FFI_FN(code),&answer,ptrs);
        if(answer!=(ffi_arg)n||calls!=1)return 1;
        free(values);free(ptrs);us_exports_clear(&set);free(b);printf("signature %zu parameters: actual ffi call passed\n",n);return 0;
    }
    typedef struct Pair (*Fn)(int,double,float,int*,struct Pair,int,double,int,int);Fn fn;memcpy(&fn,&code,sizeof fn);
    int p=4;struct Pair s={5,6};struct Pair r=fn(1,2,3,&p,s,7,7,8,14);
    if(r.d!=17||r.n!=40||s.n!=6||calls!=1)return 1;
    for(size_t i=0;i<(size_t)z;i++){
        us_export *old=set.items;size_t count=set.count;
        if(!us_exports_load(&set,b,i,err,sizeof err)||set.items!=old||set.count!=count)return 1;
    }
    us_exports_clear(&set);free(b);puts("sig2 nine mixed: 17.00 40; truncations atomic");return 0;
}
