/* E2 consumer for a verified CX0 bundle. Language rules remain in the exact
 * carried E2 network; host resource lookup never falls back to the filesystem.
 * This standalone integration does not replace the installed product driver. */
#include "../include/cx.h"
#include "../cx/cx0/bundle.h"
#define UNISA_CORE_EXTERNAL
#define core_host_fetch cx0_unused_host_fetch
#define main cx0_unused_executor_main
#include "../exec/c/run.c"
#undef main
#undef core_host_fetch
static cxb_bundle cx0_bundle;
static cxb_unit *cx0_unit;
static unsigned char *cx0_includes;
static int cx0_includes_n;
static int cx0_resource(int kind,const unsigned char *name,int n,unsigned char **bytes,int *len){
 for(int i=0;i<cx0_bundle.resource_count;i++){cxb_resource *r=cx0_bundle.resources+i;
  if(r->kind==kind&&r->name_bytes==(size_t)n&&!memcmp(r->name,name,(size_t)n)){*bytes=(unsigned char *)r->data;*len=(int)r->bytes;return 1;}}
 return 0;
}
int core_host_fetch(const unsigned char *p,int n,unsigned char **bytes,int *len){
 static unsigned char one=1;*bytes=0;*len=0;
 /* Present-empty is the E2 ABI's default trimming request. An absent key
  * is not interchangeable with an empty declared resource. */
 if(n==18&&!memcmp(p,"\0cli/fno-trim-libc",18)){*bytes=&one;return 1;}
 if(n==7&&!memcmp(p,"\0cli/f1",7)){*bytes=&one;*len=1;return 1;}
 if(n==13&&!memcmp(p,"\0cli/includes",13)){*bytes=cx0_includes;*len=cx0_includes_n;return 1;}
 if(n==11&&!memcmp(p,"\0cli/source",11)){*bytes=(unsigned char *)cx0_unit->name;*len=(int)cx0_unit->name_bytes;return 1;}
 if(n==11&&!memcmp(p,"\0cli/target",11)){*bytes=(unsigned char *)cx0_bundle.target;*len=(int)strlen(cx0_bundle.target);return 1;}
 if(n>=5&&!memcmp(p,"\0hdr/",5))return cx0_resource(1,p+5,n-5,bytes,len);
 if(n>=7&&!memcmp(p,"\0inc/",5)&&(p[5]=='q'||p[5]=='a')&&p[6]=='/')return cx0_resource(2,p+7,n-7,bytes,len);
 /* The dependency scanner resolves cli/source through RI, too. Serve the
  * immutable source carried by the bundle, never a second filesystem read. */
 if(n&&p[0]){
  for(int i=0;i<cx0_bundle.unit_count;i++){cxb_unit *u=cx0_bundle.units+i;
   if(u->name_bytes==(size_t)n&&!memcmp(u->name,p,(size_t)n)){*bytes=(unsigned char *)u->source;*len=(int)u->source_bytes;return 1;}}
  return cx0_resource(2,p,n,bytes,len);
 }
 /* Non-file control resources belong to this pinned model package. Neither
  * absent headers nor arbitrary logical paths can reach rf_default_fetch. */
 for(int i=0;i<NR;i++)if(RES[i].n==n&&!memcmp(PB+RES[i].name,p,(size_t)n)){*bytes=PB+RES[i].data;*len=RES[i].len;return 1;}
 return 0;
}
#include "../exec/c/core.c"
static char *cx0_read_regular(const char *path,size_t *n){
 int fd=open(path,O_RDONLY|O_NONBLOCK);if(fd<0)die("cx0-e2: input-open");struct stat st;
 if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size<0||st.st_size>67108864){close(fd);die("cx0-e2: regular-input-size");}
 *n=(size_t)st.st_size;char *p=malloc(*n+1);if(!p){close(fd);die("allocation");}size_t at=0;
 while(at<*n){ssize_t z=read(fd,p+at,*n-at);if(z<0&&errno==EINTR)continue;if(z<=0){close(fd);free(p);die("cx0-e2: input-read");}at+=(size_t)z;}
 char tail;ssize_t z;do{z=read(fd,&tail,1);}while(z<0&&errno==EINTR);struct stat after;int bad=z||fstat(fd,&after)||st.st_size!=after.st_size;if(close(fd))bad=1;
 if(bad){free(p);die("cx0-e2: input-changed-or-close");}p[*n]=0;return p;
}
/* Parse precisely the bytes whose digest was checked. A second path open
 * could otherwise select a different model after the identity check. */
static void cx0_load_model(const char *bytes,size_t n){
 char path[]="/tmp/cx0-e2-model-XXXXXX";int fd=mkstemp(path);if(fd<0)die("model-snapshot-create");
 if(unlink(path)){close(fd);die("model-snapshot-unlink");}size_t at=0;
 while(at<n){ssize_t z=write(fd,bytes+at,n-at);if(z<0&&errno==EINTR)continue;if(z<=0){close(fd);die("model-snapshot-write");}at+=(size_t)z;}
 if(lseek(fd,0,SEEK_SET)<0){close(fd);die("model-snapshot-seek");}
 package_parse(&CLIPKG,fd,"verified-cx0-e2-model");package_bind(&CLIPKG);
}
int main(int argc,char **argv){
 if(argc!=4){fputs("usage: E2_MODEL_CONTAINER CX0_BUNDLE UNIT_INDEX\n",stderr);return 2;}
 size_t n;char *data=cx0_read_regular(argv[2],&n);char why[256];if(cxb_read(data,n,&cx0_bundle,why,sizeof why)){fprintf(stderr,"cx0-e2: %s\n",why);free(data);return 2;}free(data);
 /* A logical source name has one meaning throughout this invocation. A
  * colliding file resource must contain exactly those same sealed bytes. */
 for(int i=0;i<cx0_bundle.unit_count;i++){cxb_unit *u=cx0_bundle.units+i;
  for(int j=0;j<i;j++){cxb_unit *v=cx0_bundle.units+j;
   if(v->name_bytes==u->name_bytes&&!memcmp(v->name,u->name,u->name_bytes)&&
    (v->source_bytes!=u->source_bytes||memcmp(v->source,u->source,u->source_bytes))){fputs("cx0-e2: conflicting-source-unit\n",stderr);cxb_clear(&cx0_bundle);return 2;}}
  for(int j=0;j<cx0_bundle.resource_count;j++){cxb_resource *r=cx0_bundle.resources+j;
   if(r->kind==2&&r->name_bytes==u->name_bytes&&!memcmp(r->name,u->name,u->name_bytes)&&
    (r->bytes!=u->source_bytes||memcmp(r->data,u->source,u->source_bytes))){fputs("cx0-e2: conflicting-source-resource\n",stderr);cxb_clear(&cx0_bundle);return 2;}}}
 char *end;long index=strtol(argv[3],&end,10);if(!*argv[3]||*end||index<0||index>=cx0_bundle.unit_count||strcmp(cx0_bundle.target,"lnx/x86_64")){fputs("cx0-e2: target-or-unit-unsupported\n",stderr);cxb_clear(&cx0_bundle);return 2;}
 data=cx0_read_regular(argv[1],&n);char hash[65];cx_sha256_hex(data,n,hash);if(strcmp(hash,cx0_bundle.model)){free(data);fputs("cx0-e2: model-identity-mismatch\n",stderr);cxb_clear(&cx0_bundle);return 2;}
 cx0_load_model(data,n);free(data);if(PM!=1||PS!=1||strcmp(STAGES[0].name,"e2")||strcmp(STAGES[0].route,"stream"))die("cx0-e2: exactly-one-e2-model-required");
 cx0_unit=cx0_bundle.units+index;size_t total=0;
 for(int i=0;i<cx0_unit->include_count;i++){unsigned id=attr_u32(cx0_unit->includes+4*i);total+=cx0_bundle.resources[id].name_bytes+1;}
 cx0_includes=malloc(total?total:1);if(!cx0_includes)die("allocation");cx0_includes_n=(int)total;size_t at=0;
 for(int i=0;i<cx0_unit->include_count;i++){unsigned id=attr_u32(cx0_unit->includes+4*i);cxb_resource *r=cx0_bundle.resources+id;memcpy(cx0_includes+at,r->name,r->name_bytes);at+=r->name_bytes;cx0_includes[at++]=0;}
 char source[4096];memcpy(source,cx0_unit->name,cx0_unit->name_bytes);source[cx0_unit->name_bytes]=0;
 Buf in={0};in.n=(int)cx0_unit->source_bytes;in.b=malloc(in.n?in.n:1);if(!in.b)die("allocation");if(in.n)memcpy(in.b,cx0_unit->source,(size_t)in.n);
 int rc=runroute("stream",&in,source);if(!rc&&in.n&&fwrite(in.b,1,(size_t)in.n,stdout)!=(size_t)in.n)rc=2;
 free(in.b);free(cx0_includes);unpackage();cxb_clear(&cx0_bundle);return rc;
}
