/* Generic one-stage package adapter. No compiler driver or language rules.
 * PRODUCT ROUTE STAGE INPUT SOURCE_LABEL ATTRIBUTE_FILE
 * The CX driver selects boundaries explicitly, including units and tapecheck. */
#define main pipeline_unused_main
#include "../exec/c/run.c"
#undef main
static int pipeline_open(const char *path){int fd=open(path,O_RDONLY|O_NONBLOCK);struct stat st;if(fd<0)die("pipeline-stage: input-open");if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size<0||st.st_size>67108864){close(fd);die("pipeline-stage: bounded-regular-input");}return fd;}
static unsigned char *pipeline_read(const char *path,int *n){int fd=pipeline_open(path);unsigned char *p=readstream(fd,path,n);if(close(fd))die("pipeline-stage: input-close");if(*n>67108864){free(p);die("pipeline-stage: input-size");}return p;}
static int pipeline_verify(const char *product,const char *directory){
 Package whole={0};package_parse(&whole,pipeline_open(product),product);
 const char *names[]={"e2","e1","e3","e4","prune","lower","elf"};
 for(int i=0;i<7;i++){
  char path[4400];int z=snprintf(path,sizeof path,"%s/stream%d",directory,i+1);if(z<0||(size_t)z>=sizeof path)die("pipeline-stage: material-path-bound");
  Package one={0};package_parse(&one,pipeline_open(path),path);int match=-1;
  for(int j=0;j<whole.s;j++)if(!strcmp(whole.stages[j].route,"lnx/x86_64/image/O2")&&!strcmp(whole.stages[j].name,names[i])){if(match>=0)die("pipeline-stage: duplicate-stage");match=j;}
  if(match<0||one.m!=1||one.s!=1||strcmp(one.stages[0].route,"stream")||strcmp(one.stages[0].name,names[i])||one.stages[0].model||one.ver!=whole.ver)die("pipeline-stage: material-stage-mismatch");
  Stage *stage=whole.stages+match;int m=stage->model;
  if(strcmp(one.stages[0].in,stage->in)||strcmp(one.stages[0].out,stage->out)||one.len[0]!=whole.len[m]||one.raw[0]!=whole.raw[m]||one.crc[0]!=whole.crc[m]||memcmp(one.b+one.off[0],whole.b+whole.off[m],(size_t)one.len[0])||one.nr!=whole.nr)die("pipeline-stage: material-model-mismatch");
  for(int j=0;j<one.nr;j++){Resource *a=one.res+j,*b=whole.res+j;if(a->n!=b->n||a->len!=b->len||memcmp(one.b+a->name,whole.b+b->name,(size_t)a->n)||memcmp(one.b+a->data,whole.b+b->data,(size_t)a->len))die("pipeline-stage: material-resource-mismatch");}
  package_release(&one);
 }
 package_release(&whole);fputs("VERIFIED\tseven-streams-same-package\n",stdout);return 0;
}
int main(int argc,char **argv){
 if(argc==4&&!strcmp(argv[1],"--verify"))return pipeline_verify(argv[2],argv[3]);
 if(argc!=7){fputs("pipeline-stage: usage PRODUCT ROUTE STAGE INPUT SOURCE ATTRIBUTES\n",stderr);return 2;}
 if(strcmp(argv[3],"units")&&strcmp(argv[3],"tapecheck")){fputs("pipeline-stage: unsupported-boundary\n",stderr);return 2;}
 package_parse(&CLIPKG,pipeline_open(argv[1]),argv[1]);package_bind(&CLIPKG);int found=0;
 for(int i=0;i<PS;i++)if(!strcmp(STAGES[i].route,argv[2])&&!strcmp(STAGES[i].name,argv[3]))found++;
 if(found!=1){fputs("pipeline-stage: missing-or-duplicate-boundary\n",stderr);unpackage();return 2;}
 static ResourceInput ri[2];ri[0].name=(const unsigned char *)"\0cli/source";ri[0].n=11;ri[0].data=(const unsigned char *)argv[5];ri[0].len=strlen(argv[5]);
 ri[1].name=(const unsigned char *)"\0cli/attributes";ri[1].n=15;ri[1].data=pipeline_read(argv[6],&ri[1].len);RI=ri;NRI=2;
 Buf in={0};in.b=pipeline_read(argv[4],&in.n);int rc=runroute_range(argv[2],argv[3],argv[3],&in,argv[5]);
 if(!rc&&in.n&&fwrite(in.b,1,(size_t)in.n,stdout)!=(size_t)in.n)rc=2;
 free((void *)ri[1].data);free(in.b);unpackage();return rc;
}
