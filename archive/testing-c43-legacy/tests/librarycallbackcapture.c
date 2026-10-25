/* Two existing owners: API capture private runtime vs standalone SC1 replay.
   Select only observation frontend; no new resource/cache/lifecycle mechanism. */
#ifdef US_CALLBACK_CAPTURE_API
/* Model witness only: real public compile/relocate adapter provides its RI.
   Replacement Tape/contract/selection/graph are accepted E3 observation inputs.
   No generated image or captured dispatch address is executed. */
#define CORE_RUN_NAME observed_original_core_run
#include "exec/c/libunisacc.c"
static const char *wdir,*wmodel,*wtape,*wcontract,*wselection,*wgraphs;
static void save(const char*n,const void*b,size_t z){char p[4096];snprintf(p,sizeof p,"%s/%s",wdir,n);FILE*f=fopen(p,"wb");if(!f||fwrite(b,1,z,f)!=z)exit(2);fclose(f);}
int __us_core_run(const CoreModel*m,unsigned char*in,int n,const char*src,I budget,CoreResult*out){
 const char*stage=NULL;
 if(CACHE)for(int i=0;i<PS;i++)if(!strcmp(STAGES[i].route,"lnx/x86_64/run/O0")&&STAGES[i].model<CACHE->nmodels&&CACHE->models[STAGES[i].model].qa==m->qa){stage=STAGES[i].name;break;}
 if(!stage||strcmp(stage,"lower")){
  return observed_original_core_run(m,in,n,src,budget,out);
 }
 save("product-lower-input.bytes",in,n);
 int tn,cn,sn,gn,mn;unsigned char*t=readfile(wtape,&tn,0),*c=readfile(wcontract,&cn,0),*s=readfile(wselection,&sn,0),*g=readfile(wgraphs,&gn,0);if(!t||!c||!s||!g)return 2;
 ResourceInput actual[64];int count=0;
 actual[count++]=(ResourceInput){(const unsigned char*)"\0library/contract",17,c,cn};
 actual[count++]=(ResourceInput){(const unsigned char*)"\0library/selection",18,s,sn};
 actual[count++]=(ResourceInput){(const unsigned char*)"\0library/callbackgraphs",23,g,gn};
 for(int i=0;i<NRI;i++){
  if((RI[i].n==17&&!memcmp(RI[i].name,"\0library/contract",17))||(RI[i].n==18&&!memcmp(RI[i].name,"\0library/selection",18)))continue;
  if(count==64)exit(2);actual[count++]=RI[i];
 }
 /* No capture state is touched in this API process. The next process owns
    its independent capture ledger and generic model execution. */
 save("consumer-input.bytes",t,tn);
 char path[4096],err[512];snprintf(path,sizeof path,"%s/provider-snapshot",wdir);
 int rc=rf_snapshot_write_ri(path,"lower:lnx/x86_64/run/O0",t,tn,actual,count,err,sizeof err);
 if(rc)fprintf(stderr,"snapshot export: %s\n",err);
 exit(rc);
}
static long real_hostcallback(long(*f)(long),long x){return f(x);}
int main(int n,char**v){if(n>1&&!strcmp(v[1],"--snapshot")){n--;v++;}else return 2;if(n!=10)return 2;wdir=v[3];wmodel=v[4];wtape=v[5];wcontract=v[6];wselection=v[7];wgraphs=v[8];if(getenv("UNISACC_RF_CAPTURE")){fprintf(stderr,"INTERFACE_ERROR snapshot-producer-capture-conflict\n");return 2;}
 us_context*c=us_new(v[1]);int z,sz;unsigned char*sig=readfile(v[9],&sz,0);unsigned char*source=readfile(v[2],&z,0);if(!c||!sig||!source)return 2;
 if(us_set_signature_version(c,3)||us_add_symbol_typed(c,"hostcallback",(void*)real_hostcallback,sig,sz)||us_add_source(c,v[2],(const char*)source)||us_compile(c,"lnx/x86_64",0)||us_relocate(c)){fprintf(stderr,"LOWER_BOUNDARY_NOT_REACHED %s\n",us_error(c));return 2;}return 2;
}

#else
#define main callback_standalone_hidden_main
#include "../exec/c/run.c"
#undef main
static const char*wdir;
static void save(const char*n,const void*b,size_t z){char p[4096];snprintf(p,sizeof p,"%s/%s",wdir,n);FILE*f=fopen(p,"wb");if(!f||fwrite(b,1,z,f)!=z)exit(2);fclose(f);}
static int callback_producer_selftest(const char*dir){
 char path[4096],err[512];unsigned char input[]={1,2,3},empty=0;RLLedger l;
 setenv("UNISACC_RF_CAPTURE",dir,1);snprintf(path,sizeof path,"%s/resources.tsv",dir);FILE*f=fopen(path,"wb");if(!f)return 2;
 rl_write_identity(f,"lower:lnx/x86_64/run/O0",input,3);fclose(f);
 rf_capture_record((const unsigned char*)"empty",5,1,&empty,0);
 rf_capture_record((const unsigned char*)"absent",6,0,0,0);
 if(rl_load(dir,&l,err,sizeof err))return 1;
 if(rl_check_identity(&l,"lower:lnx/x86_64/run/O0",input,3,err,sizeof err))return 1;
 if(!rl_check_identity(&l,"e3:lnx/x86_64/tape/O0",input,3,err,sizeof err))return 1;
 input[0]^=1;if(!rl_check_identity(&l,"lower:lnx/x86_64/run/O0",input,3,err,sizeof err))return 1;
 const RLRow*e=rl_find(&l,(const unsigned char*)"empty",5),*a=rl_find(&l,(const unsigned char*)"absent",6);
 if(!e||!e->present||e->vn||!a||a->present||rl_find(&l,(const unsigned char*)"missing",7))return 1;
 puts("producer selftest: identity stage/input NEG; EMPTY != ABSENT != MISSING; no model built or run");return 0;
}
static int callback_producer_replay(int n,char**v){
 if(n!=7)return 2;char err[512];RLLedger l;int z;unsigned char*in=readfile(v[3],&z,0);
 if(!in||rl_load(v[4],&l,err,sizeof err)){fprintf(stderr,"producer: ledger/input unavailable\n");return 2;}
 if(rl_check_identity(&l,v[5],in,z,err,sizeof err)){fprintf(stderr,"%s\n",err);return 2;}
 RF_LEDGER=l;RF_STRICT=1;RF_LOADED=1;wdir=v[6];int mn;unsigned char*model=readfile(v[2],&mn,0);if(!model)return 2;loadbytes(model,mn);Buf out={0};int rc=execute(in,z,"callback-model-observer",&out);
 save("output.bytes",out.b,out.n);return rc;
}
int main(int n,char**v){if(n==3&&!strcmp(v[1],"--selftest"))return callback_producer_selftest(v[2]);if(n>1&&!strcmp(v[1],"--replay"))return callback_producer_replay(n,v);fprintf(stderr,"usage: --selftest OUT | --replay MODEL INPUT LEDGER STAGE OUT\n");return 2;}

#endif
