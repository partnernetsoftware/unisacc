/* Single-stream developer shell over the production executor.
 * Default step budget stays 2e8, identical to exec/c/run.c.
 * Build with the product; package material is supplied separately. */
#define main stream_executor_main
#define UNISA_CX_PACKAGEFOOTER
#include "../exec/c/run.c"
#undef main
#undef UNISA_CX_PACKAGEFOOTER
#include <unistd.h>
#include <fcntl.h>
int cxs_write(int,const void *,long);
int cxs_attributes(const char *,const void *,long);
int cxsr_parse(const char *,unsigned char *,long,const char **,int *);
int cxsa_valid(const unsigned char *,long);
int main(int argc,char **argv){
 const char *input=argc>1?argv[1]:"-",*source=argc>2?argv[2]:"stdin.c",*ap;unsigned char *bytes,*owned=0;int n,rc,framed;Buf in={0},out={0};static unsigned char one=1;static ResourceInput ri[5];static unsigned char rname[256];
 if(argc>4){fprintf(stderr,"usage: stream.com [INPUT|-] [SOURCE_LABEL] [INCLUDE_DIR]\n");return 2;}
 if(argc>3)INCDIR=argv[3];
 package(argv[0]);if(PM!=1||PS!=1||strcmp(STAGES[0].route,"stream")||STAGES[0].model!=0)die("requires exactly one stream model");
 ri[0].name=(unsigned char*)"\0cli/f1";ri[0].n=7;ri[0].data=&one;ri[0].len=1;
 ri[1].name=(unsigned char*)"\0cli/attributes";ri[1].n=15;
 ri[2].name=(unsigned char*)"\0cli/source";ri[2].n=11;ri[2].data=(unsigned char*)source;ri[2].len=strlen(source);ri[3].name=(unsigned char*)"\0cli/target";ri[3].n=11;ri[3].data=(unsigned char*)"lnx/x86_64";ri[3].len=10;RI=ri;NRI=4;ATTR_SLOT=1;
 /* cc 2026-10-28: optional UNISA_STREAM_RESOURCE=NAME=PATH adds one resource "\0NAME" with PATH's bytes (library/symbols,
    library/signatures); absent or empty, behaviour and bytes are unchanged */
 {const char *resource_path=0;int name_len=0;int parsed=cxsr_parse(getenv("UNISA_STREAM_RESOURCE"),rname,sizeof rname,&resource_path,&name_len);if(parsed==2)die("UNISA_STREAM_RESOURCE must be NAME=PATH");if(parsed==1){int rl=0;ri[4].name=rname;ri[4].n=name_len;ri[4].data=readfile(resource_path,&rl,0);if(!ri[4].data)die("cannot read UNISA_STREAM_RESOURCE file");ri[4].len=rl;NRI=5;}}
 in.b=!strcmp(input,"-")?readstream(0,"stdin",&in.n):readfile(input,&in.n,0);
 framed=in.n>=8&&!memcmp(in.b,"USLATTR1",8);
 if(framed){if(strcmp(STAGES[0].name,"e3"))die("attribute frame requires e3 input");if(!attr_strip(&in))die("invalid attribute frame");}
 else {ap=getenv("UNISA_ATTRIBUTES");if(ap&&*ap){ATTRS.b=readfile(ap,&ATTRS.n,1);ATTRS.cap=ATTRS.n;if(ATTRS.b&&!cxsa_valid(ATTRS.b,ATTRS.n))die("malformed external attributes");ri[1].data=ATTRS.b;ri[1].len=ATTRS.n;}}
 bytes=PB+POFF[0];n=PLEN[0];if(PVER==3){owned=xrealloc(0,PRAW[0]);if(unisa_inflate(owned,PRAW[0],bytes,n)||crc((char*)owned,PRAW[0])!=PCRC[0])die("invalid compressed model");if(PRAW[0]<9||memcmp(owned,"UNINETB1N",9))die("package requires binary networks");bytes=owned;n=PRAW[0];}
 model_clear();MODEL_LOADING=1;loadbytes(bytes,n);free(owned);LB=0;MODEL_LOADING=0;
 rc=execute(in.b,in.n,source,&out);free(in.b);if(rc){unload();unpackage();return rc;}
 if(!strcmp(STAGES[0].name,"e1")&&out.n>=16&&!memcmp(out.b,"USLATTR1",8)){Buf copy=out;copy.b=xrealloc(0,out.n);memcpy(copy.b,out.b,out.n);attr_strip(&copy);free(copy.b);}
 rc=cxs_attributes(getenv("UNISA_ATTRIBUTES"),ATTRS.b,ATTRS.n);if(!rc)rc=cxs_write(1,out.b,out.n);free(out.b);unload();unpackage();if(rc)fprintf(stderr,"stream: output or attributes write failed\n");return rc;
}
