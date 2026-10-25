/* API E3 observation stops at the actual input boundary and exports RI only.
   Independent native run owns model execution and capture. All resource adaptation, ledger parsing,
   identity hashing, route execution and model execution reuse production helpers. */
#define CORE_RUN_NAME observed_original_core_run
#include "exec/c/libunisacc.c"
static const char *witness_dir,*witness_model,*witness_contract,*witness_selection;
static void save(const char*name,const void*b,size_t n){char path[4096];snprintf(path,sizeof path,"%s/%s",witness_dir,name);FILE*f=fopen(path,"wb");if(!f||fwrite(b,1,n,f)!=n)exit(2);fclose(f);}
int __us_core_run(const CoreModel*m,unsigned char*input,int inputn,const char*src,I maxsteps,CoreResult*out){
 const char *stage=NULL;
 if(CACHE)for(int i=0;i<PS;i++)if(!strcmp(STAGES[i].route,"lnx/x86_64/tape/O0")&&STAGES[i].model<CACHE->nmodels&&CACHE->models[STAGES[i].model].qa==m->qa){stage=STAGES[i].name;break;}
 fprintf(stderr,"OBS route=lnx/x86_64/tape/O0 stage=%s input_bytes=%d\n",stage?stage:"UNKNOWN",inputn);
 if(!stage||strcmp(stage,"e3")){
  if(stage&&!strcmp(stage,"e1"))save("assembled-source.bytes",input,inputn);
  int rc=observed_original_core_run(m,input,inputn,src,maxsteps,out);
  if(stage&&!strcmp(stage,"e1"))save("e1-output.bytes",out->out.b,out->out.n);
  return rc;
 }
 save("input.bytes",input,inputn);save("attributes.bytes",ATTRS.b,ATTRS.n);save("source-label.bytes",src,strlen(src));
 int cn,sn;unsigned char*cbytes=readfile(witness_contract,&cn,0),*sbytes=readfile(witness_selection,&sn,0);static unsigned char one[8]={1};
 ResourceInput stage_resources[32];int n=0;
 stage_resources[n++]=(ResourceInput){(const unsigned char*)"\0library/contract",17,cbytes,cn};
 stage_resources[n++]=(ResourceInput){(const unsigned char*)"\0library/selection",18,sbytes,sn};
 stage_resources[n++]=(ResourceInput){(const unsigned char*)"\0library/typed-mode",19,one,8};
 for(int i=0;i<NRI;i++){
  if((RI[i].n==17&&!memcmp(RI[i].name,"\0library/contract",17))||
     (RI[i].n==18&&!memcmp(RI[i].name,"\0library/selection",18))||
     (RI[i].n==19&&!memcmp(RI[i].name,"\0library/typed-mode",19)))continue;
  if(n>=32)exit(2);stage_resources[n++]=RI[i];
 }
 char path[4096],err[512];snprintf(path,sizeof path,"%s/provider-snapshot",witness_dir);
 int rc=rf_snapshot_write_ri(path,"e3:lnx/x86_64/tape/O0",input,inputn,stage_resources,n,err,sizeof err);
 if(rc)fprintf(stderr,"snapshot export: %s\n",err);
 exit(rc);

}
int main(int n,char**v){if(n!=7)return 2;witness_dir=v[3];witness_model=v[4]; /* compatibility argument: independent consumer owns MODEL */witness_contract=v[5];witness_selection=v[6];if(getenv("UNISACC_RF_CAPTURE")){fprintf(stderr,"INTERFACE_ERROR snapshot-producer-capture-conflict\n");return 2;}setenv("UNISA_MAXSTEPS","400000000000",1);
 FILE*f=fopen(v[2],"rb");if(!f)return 2;char source[65536];size_t z=fread(source,1,sizeof(source)-1,f);fclose(f);source[z]=0;
 us_context*c=us_new(v[1]);if(!c||us_set_signature_version(c,3)||us_add_source(c,v[2],source))return 2;
 int rc=us_compile(c,"lnx/x86_64",0);fprintf(stderr,"E3 boundary not reached, rc=%d %s\n",rc,us_error(c));return 2;
}
