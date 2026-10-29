/* LB-1: typed framing/host adaptation only; Tape decisions stay in models. */
/* run.c's native branch has these; the product branch uses private syscall IO.
   Use the existing shipped headers, never a host-header search override. */
#if defined(__UNISA__) && (defined(__APPLE__) || defined(__linux__))
#include <sys/stat.h>
#include <unistd.h>
#endif
typedef struct { unsigned target, filter; int mode, native; const char *symbols;
 ResourceInput *cli; unsigned char *dl; } HostCompileContext;
static int host_stage_present(const char *route,const char *name){for(int i=0;i<PS;i++){const char *const rowr=STAGES[i].route,*const rown=STAGES[i].name;int p=bx_stage_present(route,name,&rowr,&rown,1);if(p==1)return 1;if(p==4)return 0;}return 0;}
static int host_filter_file(const char *path,unsigned target,unsigned *mask){
 int fr=bx_filter_none(path,target,mask);if(fr==0)return 0;if(fr==4)return 1;unsigned char bytes[13];char error[160];FILE*f=fopen(path,bx_file_mode(0));
 if(!f)return clierror("bind: cannot-open-provider file");size_t n=fread(bytes,1,sizeof bytes,f);int failed=ferror(f);if(fclose(f))failed=1;
 if(failed)return clierror("bind: cannot-read-provider file");
 if(bp_prov_rc(host_provider_filter(bytes,n,target,mask,error,sizeof error))){fprintf(stderr,"%s\n",error);return 1;}return 0;
}
#if defined(__APPLE__) || defined(__linux__)
static int host_path_key(const char *path,char *key,size_t cap){
 if(realpath(path,key))return 0;if(bp_errno_keep(errno,ENOENT))return 1;
 if(bx_key_len((int)strlen(path)))return 1;
 char parent[4096],base[4096];
 if(bx_key_split(path,parent,(int)sizeof parent,base,(int)sizeof base))return 1;
 char resolved[4096];if(bp_base_empty(base)||bp_parent_miss(realpath(parent,resolved)!=0))return 1;
 if(bx_key_fit((int)strlen(resolved),(int)strlen(base),(int)cap))return 1;
 return bp_wrote_over(snprintf(key,cap,"%s/%s",resolved,base),(int)cap);
}
static int host_same_output(const char*a,const char*b){
 if(!b||!strcmp(b,"-"))return !strcmp(a,"-");struct stat x,y;int ax=stat(a,&x),ae=errno,by=stat(b,&y),be=errno;
 if((ax&&ae!=ENOENT)||(by&&be!=ENOENT))return -1;
 if(!ax&&!by&&x.st_dev==y.st_dev&&x.st_ino==y.st_ino)return 1;
 char ka[4096],kb[4096];if(host_path_key(a,ka,sizeof ka)||host_path_key(b,kb,sizeof kb))return -1;return !strcmp(ka,kb);
}
static int host_side_write(const char*path,const Buf*b){
 char tmp[4096];if(bx_tmp_name(path,tmp,(int)sizeof tmp))return clierror(bx_tmp_why());
 int fd=mkstemp(tmp);if(fd<0)return clierror("bind: cannot-write-symbol-stream");FILE*f=fdopen(fd,"wb");
 if(!f){close(fd);unlink(tmp);return clierror("bind: cannot-write-symbol-stream");}
 int failed=fwrite(b->b,1,(size_t)b->n,f)!=(size_t)b->n;if(fclose(f))failed=1;
 if(failed){unlink(tmp);return clierror("bind: cannot-write-symbol-stream");}
 if(rename(tmp,path)){unlink(tmp);return clierror("bind: cannot-commit-symbol-stream");}return 0;
}
#else
static int host_same_output(const char*a,const char*b){(void)a;(void)b;return -1;}
static int host_side_write(const char*p,const Buf*b){(void)p;(void)b;return clierror("bind: symbol-stream target not supported");}
#endif
static int host_symbol_stream(const char *route,Buf*tape,const char*src,HostCompileContext*c,Buf*symbols,HostRequirements*r){
 /* Validate EXACT bytes, including optimizer/prune output, not a filename flag. */
 int rc=tape_domain(route,tape,src);if(rc)return rc;
 if(bx_sym_model(host_stage_present("host-symbols","symbols")))return clierror(bx_model_why());
 if(bx_sym_span(tape->n))return clierror("bind: symbol input too large");
 symbols->n=bx_sym_bytes(tape->n);symbols->b=xrealloc(0,(size_t)symbols->n);bx_sym_head((int)c->target,symbols->b);memcpy(symbols->b+10,tape->b,(size_t)tape->n);
 rc=runroute("host-symbols",symbols,src);if(rc)return rc;
 char error[160];if(host_requirements(symbols->b,(size_t)symbols->n,r,error,sizeof error)){fprintf(stderr,"%s\n",error);return 1;}return 0;
}
static int host_compile(const char*route,const char*first,Buf*in,const char*src,HostCompileContext*c){
 if(bx_host_skip((int)c->target,c->mode))return bx_host_from(first)?runroute_from(route,first,in,src):runroute(route,in,src);
 int rc=runroute_range(route,first,"prune",in,src);if(rc)return rc;
 Buf symbols={0};HostRequirements requirements;rc=host_symbol_stream(route,in,src,c,&symbols,&requirements);
 if(!rc)rc=runroute_range(route,"lower","lower",in,src);
 /* Receipt after lower success; subsequent binding refusal may retain it. */
 if(bx_host_side(rc,c->symbols))rc=host_side_write(c->symbols,&symbols);
 free(symbols.b);if(rc)return rc;
 HostProvider provider;memset(&provider,0,sizeof provider);provider.target=c->target;provider.kind=bx_host_kind(c->native);
 if(bx_host_fill(c->native))provider.available=15; /* existing target-loader import provider, not host addresses */
 else {for(int j=0;j<4;j++){int bit=0;provider.address[j]=(uint64_t)host_dl_slot(j);if(bx_host_bit(j,provider.address[j]!=0,&bit)==0)provider.available|=(unsigned)bit;}}
 HostBindPlan plan;char error[160];if(host_bind(&requirements,&provider,c->filter,&plan,error,sizeof error)){fprintf(stderr,"%s\n",error);return 1;}
 if(!bx_host_fill(c->native)){for(int j=0;j<mr_host_n();j++){lw_put(c->dl+mr_host_at(j),8,(unsigned long long)plan.address[j]);c->cli[mr_host_cli(j)].n=bx_host_keyn();}}
 /* run routes end at lower (memory writes the image); an image route without its elf
    stage is a package boundary error, never lower text written as the image */
 if(bx_host_elf(c->mode,host_stage_present(route,"elf")))return 0;
 return runroute_from(route,"elf",in,src);
}

