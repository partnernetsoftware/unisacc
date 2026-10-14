/* LB-1: typed framing/host adaptation only; Tape decisions stay in models. */
/* run.c's native branch has these; the product branch uses private syscall IO.
   Use the existing shipped headers, never a host-header search override. */
#if defined(__UNISA__) && (defined(__APPLE__) || defined(__linux__))
#include <sys/stat.h>
#include <unistd.h>
#endif
typedef struct { unsigned target, filter; int mode, native; const char *symbols;
 ResourceInput *cli; unsigned char (*dl)[8]; } HostCompileContext;
static int host_stage_present(const char *route,const char *name){for(int i=0;i<PS;i++)if(!strcmp(STAGES[i].route,route)&&!strcmp(STAGES[i].name,name))return 1;return 0;}
static int host_filter_file(const char *path,unsigned target,unsigned *mask){
 if(!path){*mask=15;return 0;}unsigned char bytes[13];char error[160];FILE*f=fopen(path,"rb");
 if(!f)return clierror("bind: cannot-open-provider file");size_t n=fread(bytes,1,sizeof bytes,f);int failed=ferror(f);if(fclose(f))failed=1;
 if(failed)return clierror("bind: cannot-read-provider file");
 if(host_provider_filter(bytes,n,target,mask,error,sizeof error)){fprintf(stderr,"%s\n",error);return 1;}return 0;
}
#if defined(__APPLE__) || defined(__linux__)
static int host_path_key(const char *path,char *key,size_t cap){
 if(realpath(path,key))return 0;if(errno!=ENOENT)return 1;
 char parent[4096],base[4096];size_t n=strlen(path);if(!n||n>=sizeof parent)return 1;strcpy(parent,path);char *slash=strrchr(parent,'/');
 if(slash){snprintf(base,sizeof base,"%s",slash+1);if(slash==parent)slash[1]=0;else *slash=0;}else{snprintf(base,sizeof base,"%s",parent);strcpy(parent,".");}
 char resolved[4096];if(!*base||!realpath(parent,resolved))return 1;
 return snprintf(key,cap,"%s/%s",resolved,base)>=(int)cap;
}
static int host_same_output(const char*a,const char*b){
 if(!b||!strcmp(b,"-"))return !strcmp(a,"-");struct stat x,y;int ax=stat(a,&x),ae=errno,by=stat(b,&y),be=errno;
 if((ax&&ae!=ENOENT)||(by&&be!=ENOENT))return -1;
 if(!ax&&!by&&x.st_dev==y.st_dev&&x.st_ino==y.st_ino)return 1;
 char ka[4096],kb[4096];if(host_path_key(a,ka,sizeof ka)||host_path_key(b,kb,sizeof kb))return -1;return !strcmp(ka,kb);
}
static int host_side_write(const char*path,const Buf*b){
 char tmp[4096];if(snprintf(tmp,sizeof tmp,"%s.XXXXXX",path)>=(int)sizeof tmp)return clierror("bind: symbol-stream path too long");
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
 if(!host_stage_present("host-symbols","symbols"))return clierror("bind: required host-symbols model is missing");
 if(tape->n>2147483637)return clierror("bind: symbol input too large");
 symbols->n=tape->n+10;symbols->b=xrealloc(0,(size_t)symbols->n);memcpy(symbols->b,"USLSYM1\n",8);symbols->b[8]=(unsigned char)c->target;symbols->b[9]=0;memcpy(symbols->b+10,tape->b,(size_t)tape->n);
 rc=runroute("host-symbols",symbols,src);if(rc)return rc;
 char error[160];if(host_requirements(symbols->b,(size_t)symbols->n,r,error,sizeof error)){fprintf(stderr,"%s\n",error);return 1;}return 0;
}
static int host_compile(const char*route,const char*first,Buf*in,const char*src,HostCompileContext*c){
 if(!c->target||(c->mode!=0&&c->mode!=3))return first?runroute_from(route,first,in,src):runroute(route,in,src);
 int rc=runroute_range(route,first,"prune",in,src);if(rc)return rc;
 Buf symbols={0};HostRequirements requirements;rc=host_symbol_stream(route,in,src,c,&symbols,&requirements);
 if(!rc)rc=runroute_range(route,"lower","lower",in,src);
 /* Receipt after lower success; subsequent binding refusal may retain it. */
 if(!rc&&c->symbols)rc=host_side_write(c->symbols,&symbols);
 free(symbols.b);if(rc)return rc;
 HostProvider provider;memset(&provider,0,sizeof provider);provider.target=c->target;provider.kind=c->native?2:1;
 if(c->native){for(int j=0;j<4;j++){provider.address[j]=(uint64_t)host_dl_slot(j);if(provider.address[j])provider.available|=1u<<j;}}
 else provider.available=15; /* existing target-loader import provider, not host addresses */
 HostBindPlan plan;char error[160];if(host_bind(&requirements,&provider,c->filter,&plan,error,sizeof error)){fprintf(stderr,"%s\n",error);return 1;}
 if(c->native){for(int j=0;j<4;j++){resource_u64(c->dl[j],plan.address[j]);c->cli[9+j].n=13;}}
 return host_stage_present(route,"elf")?runroute_from(route,"elf",in,src):0;
}

