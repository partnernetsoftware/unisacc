#define US_CALLABLES_IMPLEMENTATION
#include "librarycarrierplan.h"
#include <inttypes.h>
static uint64_t take64(const unsigned char *b){
    uint64_t n=0;for(unsigned i=0;i<8;i++)n|=(uint64_t)b[i]<<(8*i);return n;
}
/* This SCRIPT hook exercises original aggregate frames, not source compilation. */
union Value { double d; uint64_t bits; };
static union Value native_flip(union Value v){v.bits^=UINT64_C(0x8877665544332211);return v;}
static int native_call(void *owner,ffi_cif *cif,uintptr_t raw,void *out,void **in){
    (*(unsigned*)owner)++;ffi_call(cif,FFI_FN((void*)raw),out,in);return 0;
}
static int script_frame(void *owner,const void *raw,const us_export_frame *f){
    (void)owner;(void)raw;
    if(f->count!=1||f->result_kind!=5||f->result_bytes!=8)return 1;
    union Value v;memcpy(&v,(const void*)(uintptr_t)f->slots[0],8);
    v.bits^=UINT64_C(0x0102030405060708);memcpy(f->result,&v,8);return 0;
}
static int exercise(const us_carrier_certificate *plan){
    us_export_signature original;us_callables registry;unsigned calls=0;uint64_t nh=0,sh=0;void *entry=NULL;char error[256];int rc=1;
    if(us_callable_export_signature(plan->original.items,&original))return 1;
    us_callables_init(&registry,&calls,1,script_frame,native_call,NULL);
    if(us_carrier_certificate_make(plan,&registry,US_CALLABLE_NATIVE,&original,(uintptr_t)native_flip,&nh,error,sizeof error)||
       us_carrier_certificate_make(plan,&registry,US_CALLABLE_SCRIPT,&original,1,&sh,error,sizeof error)||
       us_callable_pointer(&registry,sh,&original,&entry,error,sizeof error))goto done;
    for(unsigned i=0;i<100;i++){
        union Value input;input.bits=UINT64_C(0x7ff8aa5500000000)+i;
        struct {uint64_t before;union Value result;uint64_t after;} out={11,{.bits=0},22};
        uint64_t slot=(uintptr_t)&input;
        if(us_callable_call(&registry,nh,&original,&slot,&out.result,1,error,sizeof error)||
           out.result.bits!=(input.bits^UINT64_C(0x8877665544332211))||out.before!=11||out.after!=22)goto done;
        union Value v=((union Value(*)(union Value))entry)(input);
        if(v.bits!=(input.bits^UINT64_C(0x0102030405060708))||input.bits!=UINT64_C(0x7ff8aa5500000000)+i)goto done;
    }
    rc=calls!=100;
done:us_callables_clear(&registry);return rc;
}
int main(int argc,char **argv){
    if(argc!=3)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END))return 2;long z=ftell(f);
    if(z<8||z>33554432||fseek(f,0,SEEK_SET))return 2;
    unsigned char *b=malloc((size_t)z);if(!b)return 2;
    if(fread(b,1,(size_t)z,f)!=(size_t)z||fclose(f)){free(b);return 2;}
    uint64_t count=take64(b);size_t at=8;unsigned good=0,bad=0;
    us_carrier_certificate plan={0};char error[256];
    if(!count||count>4096)return 2;
    for(uint64_t i=0;i<count;i++){
        if(at>(size_t)z||(size_t)z-at<16)return 2;
        uint64_t want=take64(b+at),n=take64(b+at+8);at+=16;
        if(n>(size_t)z-at||want>1)return 2;
        us_export *old=plan.original.items,*oldcarrier=plan.carrier.items;
        error[0]=0;int rc=us_carrier_certificate_load(&plan,argv[2],b+at,(size_t)n,error,sizeof error);
        at+=(size_t)n;
        if((rc==0)!=(want==0)){fprintf(stderr,"case %" PRIu64 " unexpected rc%d: %s\n",i,rc,error);return 1;}
        if(rc){
            if(!error[0]||plan.original.items!=old||plan.carrier.items!=oldcarrier)return 1;
            bad++;
        }else{
            us_export_signature original,carrier;
            if(us_callable_export_signature(plan.original.items,&original)||
               us_callable_export_signature(plan.carrier.items,&carrier)||
               original.result.kind!=5||original.result.tag!=2||
               carrier.result.kind!=1||carrier.result.uns!=1||
               original.count!=1||carrier.count!=1)return 1;
            /* Owned single-record wires must round-trip without ABI rewriting. */
            us_exports replay={0};
            us_export *x=plan.original.items;
            if(!x->wire||us_exports_load_bridge(&replay,x->wire,x->wire_length,error,sizeof error))return 1;
            us_export_signature replay_view;
            if(replay.count!=1||strcmp(replay.items[0].name,x->name)||
               us_callable_export_signature(replay.items,&replay_view)||
               !us_callable_signature_equal(&original,&replay_view)||
               replay.items[0].wire_length!=x->wire_length||
               memcmp(replay.items[0].wire,x->wire,x->wire_length))return 1;
            us_exports_clear(&replay);
            if(exercise(&plan))return 1;
            good++;
        }
    }
    us_carrier_certificate_clear(&plan);free(b);
    if(at!=(size_t)z||!good||!bad)return 1;
    printf("carrier framing: %u accepted, %u rejected; transaction preserved; each valid plan native/closure x100\n",good,bad);return 0;
}
