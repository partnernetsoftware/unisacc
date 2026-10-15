/* Test witness adapter: preserve product route framing; record/replay opaque resource responses.
   Build with exec/c/core.c. No product source edits, no result reuse authority. */
#define UNISA_RUNTIME_LIBRARY
#define UNISA_CORE_EXTERNAL
#define core_host_fetch middle_original_fetch
#include "../exec/c/run.c"
#include "../exec/c/resourceledger.h"
#undef core_host_fetch
#include "../include/cx.h"
#define MF_LIMIT 256
static int replay_mode, logging, nr_seen;
static char *outdir;
typedef struct {unsigned char *key,*value; int kn,vn,present;} MFResource;
static MFResource seen[MF_LIMIT];
static void keyhex(const unsigned char*p,int n,char*h){int i;for(i=0;i<n;i++){h[2*i]="0123456789abcdef"[p[i]>>4];h[2*i+1]="0123456789abcdef"[p[i]&15];}h[2*n]=0;}
int core_host_fetch(const unsigned char*p,int n,unsigned char**bytes,int*len){int i,rc;
 if(replay_mode){for(i=0;i<nr_seen;i++)if(n==seen[i].kn&&!memcmp(p,seen[i].key,n)){char th[1025];if(getenv("MF_TRACE")&&n<=512){keyhex(p,n,th);fprintf(stderr,"TRACE resource %s present %d len %d\n",th,seen[i].present,seen[i].vn);}*bytes=seen[i].value;*len=seen[i].vn;return seen[i].present;}char h[1025];if(n>512)die("resource key too long");keyhex(p,n,h);fprintf(stderr,"MISSING_EXPLICIT_RESOURCE %s\n",h);exit(2);}
 rc=middle_original_fetch(p,n,bytes,len);
 if(logging){if(nr_seen==MF_LIMIT)die("resource witness capacity");MFResource*r=&seen[nr_seen++];r->key=malloc(n?n:1);memcpy(r->key,p,n);r->kn=n;r->present=rc!=0;r->vn=rc?*len:0;r->value=malloc(r->vn?r->vn:1);if(r->vn)memcpy(r->value,*bytes,r->vn);}
 return rc;
}
static void writeblob(const char*name,const void*b,size_t n){char path[4096];snprintf(path,sizeof path,"%s/%s",outdir,name);if(cx_write(path,b,n))die("write witness failed");}
static const char *mf_stage="e3:lnx/x86_64/tape/O0"; static char mf_sha[65];
static void resources_save(void){char path[4096],key[1025],sha[65],name[50];int i;FILE*f;snprintf(path,sizeof path,"%s/resources.tsv",outdir);f=fopen(path,"wb");if(!f)die("resource ledger output");rl_write_identity_hex(f,mf_stage,mf_sha);for(i=0;i<nr_seen;i++){MFResource*r=&seen[i];if(r->kn>512)die("resource key size");keyhex(r->key,r->kn,key);snprintf(name,sizeof name,"resource-%03d.bytes",i);if(r->present){writeblob(name,r->value,r->vn);cx_sha256_hex(r->value,r->vn,sha);}else{strcpy(name,"-");strcpy(sha,"-");}fprintf(f,"%s\t%d\t%s\t%d\t%s\n",key,r->present,name,r->vn,sha);}fclose(f);}
static int digit(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;die("hex key malformed");return 0;}
/* the ledger parser is the shared one (exec/c/resourceledger.h): strict rows, named failures, no second implementation */
static void resources_load(const char*dir){
 RLLedger L; char err[512];
 if(rl_load(dir,&L,err,sizeof err)){fprintf(stderr,"%s\n",err);exit(2);}
 for(int i=0;i<L.n;i++){
  if(nr_seen==MF_LIMIT)die("resource witness capacity");
  MFResource*r=&seen[nr_seen++];
  r->key=L.rows[i].key; r->kn=L.rows[i].kn; r->present=L.rows[i].present; r->value=L.rows[i].value; r->vn=L.rows[i].vn;
 }
}
static void defaults(const char*label){static ResourceInput cli[32];static unsigned char one=1;static const char*keys[]={"\0cli/defines","\0cli/undefines","\0cli/includes","\0cli/include-dir","\0cli/nostdinc","\0cli/source","\0cli/fno-trim-libc","\0cli/werror","\0cli/error-limit","\0cli/f1","\0cli/attributes","\0cli/target"};int i;RI=cli;NRI=12;for(i=0;i<12;i++){cli[i].name=(const unsigned char*)keys[i];cli[i].n=1+strlen(keys[i]+1);cli[i].data=(const unsigned char*)"";cli[i].len=0;}cli[5].data=(const unsigned char*)label;cli[5].len=strlen(label)+1;cli[9].data=&one;cli[9].len=1;ATTR_SLOT=10;cli[11].data=(const unsigned char*)"lnx/x86_64";cli[11].len=10;}
int main(int argc,char**argv){const char*label="leaf/middle.c";char path[4096],*input;size_t n;Buf in={0},out={0};int rc,expected;
 setenv("UNISA_MAXSTEPS","400000000000",1); /* explicit execution budget, never ambient */
 if(argc==6&&(!strcmp(argv[1],"capture")||!strcmp(argv[1],"capture-lower")||!strcmp(argv[1],"capture-lower-refuse"))){outdir=argv[4];expected=atoi(argv[5]);defaults(label);package(argv[2]);input=cx_read(argv[3],&n);if(!input)die("source absent");in.b=(unsigned char*)input;in.n=n;rc=runroute_range("lnx/x86_64/tape/O0",0,"e1",&in,label);if(rc)return rc;if(in.n<8||memcmp(in.b,"UNITOK",6))die("product E1 not a UNITOK frame");if(strcmp(argv[1],"capture")){rc=runroute_range("lnx/x86_64/image/O0","e3","prune",&in,label);if(rc)return rc;if(!strcmp(argv[1],"capture-lower-refuse")){free(in.b);in.b=(unsigned char*)strdup("?\n");in.n=2;}}
 mf_stage=!strcmp(argv[1],"capture")?"e3:lnx/x86_64/tape/O0":"lower:lnx/x86_64/image/O0";rl_input_hex(in.b,in.n,mf_sha);writeblob("input.bytes",in.b,in.n);writeblob("attributes.bytes",ATTRS.b,ATTRS.n);writeblob("source-label.bytes",label,strlen(label));logging=1;rc=!strcmp(argv[1],"capture")?runroute_range("lnx/x86_64/tape/O0","e3","e3",&in,label):runroute_range("lnx/x86_64/image/O0","lower","lower",&in,label);logging=0;resources_save();writeblob("output.bytes",in.b,in.n);snprintf(path,sizeof path,"%s/status.txt",outdir);char status[20];snprintf(status,sizeof status,"%d\n",rc);cx_write(path,status,strlen(status));if(rc!=expected){fprintf(stderr,"unexpected stage verdict %d wanted %d\n",rc,expected);return 1;}puts("capture: expected stage verdict and full input/resource witness saved");return 0;}
 if(argc==5&&!strcmp(argv[1],"replay")){snprintf(path,sizeof path,"%s.stderr",argv[4]);if(!freopen(path,"wb",stderr))return 2;resources_load(argv[3]);replay_mode=1;snprintf(path,sizeof path,"%s/source-label.bytes",argv[3]);input=cx_read(path,&n);if(!input)die("source-label absent");label=input;snprintf(path,sizeof path,"%s/input.bytes",argv[3]);int length=0;unsigned char*bytes=readfile(path,&length,0);{RLLedger IL;char ie[512];const char*stage=getenv("MF_STAGE_ID");if(!stage){fprintf(stderr,"replay refused: MF_STAGE_ID (the stage label of this replay) is required\n");return 2;}if(rl_load(argv[3],&IL,ie,sizeof ie)||rl_check_identity(&IL,stage,bytes,length,ie,sizeof ie)){fprintf(stderr,"%s\n",ie);return 2;}}int mn=0;unsigned char*mb=readfile(argv[2],&mn,0);{const char*want=getenv("MF_MODEL_SHA256");if(want&&mb){char got[65];cx_sha256_hex(mb,mn,got);if(strcmp(got,want)){fprintf(stderr,"MODEL_IDENTITY_MISMATCH %s\n",got);return 2;}}}loadbytes(mb,mn);free(mb);rc=execute(bytes,length,label,&out);if(out.n&&fwrite(out.b,1,out.n,stdout)!=(size_t)out.n)die("stdout write failed");return rc;}
 fprintf(stderr,"usage: middleflowrunner capture PACKAGE SOURCE OUT EXPECTED | replay MODEL DATA RESERVED\n");return 2;
}
