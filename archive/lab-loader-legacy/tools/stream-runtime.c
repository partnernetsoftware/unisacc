/* Single-stream developer shell over the production executor.
 * Default step budget stays 2e8, identical to exec/c/run.c.
 * Build with the product; package material is supplied separately. */
#define main stream_executor_main
#define UNISA_CX_PACKAGEFOOTER
#define UNISA_CX_ATTRIBUTE_FRAME
#define UNISA_CX_MODEL_NUMBER
#include "../exec/c/run.c"
#undef UNISA_CX_MODEL_NUMBER
#undef UNISA_CX_ATTRIBUTE_FRAME
#undef main
#undef UNISA_CX_PACKAGEFOOTER
#include <unistd.h>
#include <fcntl.h>
int cxs_write(int,const void *,long);
int cxs_attributes(const char *,const void *,long);
int cxsr_parse(const char *,unsigned char *,long,const char **,int *);
int cxsa_valid(const unsigned char *,long);
int main(int argc,char **argv){
 const char *input=argc>1?argv[1]:"-",*source=argc>2?argv[2]:"stdin.c",*ap;unsigned char *bytes,*owned=0;int n,rc,framed;Buf in={0},out={0};static unsigned char one=1;enum{NRMAX=64};static ResourceInput ri[NRMAX];static unsigned char rname[256];static unsigned char kb[NRMAX][256];
 if(argc>4){fprintf(stderr,"usage: stream.com [INPUT|-] [SOURCE_LABEL] [INCLUDE_DIR]\n");return 2;}
 if(argc>3)INCDIR=argv[3];
 package(argv[0]);if(PM!=1||PS!=1||strcmp(STAGES[0].route,"stream")||STAGES[0].model!=0)die("requires exactly one stream model");
 /* Defaults first, then the caller's list overrides them: cli/source and
    cli/target come from argv, cli/f1 and cli/attributes are the two fixed
    slots; every other cli/* key must come from UNISA_STREAM_RESOURCES. */
 ri[0].name=(unsigned char*)"\0cli/f1";ri[0].n=7;ri[0].data=&one;ri[0].len=1;
 ri[1].name=(unsigned char*)"\0cli/attributes";ri[1].n=15;
 ri[2].name=(unsigned char*)"\0cli/source";ri[2].n=11;ri[2].data=(unsigned char*)source;ri[2].len=strlen(source);
 ri[3].name=(unsigned char*)"\0cli/target";ri[3].n=11;ri[3].data=(unsigned char*)"lnx/x86_64";ri[3].len=10;
 RI=ri;NRI=4;ATTR_SLOT=1;
 ri[2].name=(unsigned char*)"\0cli/source";ri[2].n=11;ri[2].data=(unsigned char*)source;ri[2].len=strlen(source);ri[3].name=(unsigned char*)"\0cli/target";ri[3].n=11;ri[3].data=(unsigned char*)"lnx/x86_64";ri[3].len=10;RI=ri;NRI=4;ATTR_SLOT=1;
 /* cc 2026-10-28: optional UNISA_STREAM_RESOURCE=NAME=PATH adds one resource "\0NAME" with PATH's bytes (library/symbols,
    library/signatures); absent or empty, behaviour and bytes are unchanged */
 /* cli/* resources are supplied by the caller, not by the package (17 of the
    product's e3 reads are present; 13 are cli/*). Read them from a list file,
    one "hex-key<TAB>path" per line, so the runner carries no second table:
    the same list the product's capture writes (resources.tsv). */
 { const char *lf = getenv("UNISA_STREAM_RESOURCES");
   if (lf && *lf) {
     char *tb = 0; size_t tn = 0; size_t at = 0;
     tb = (char *)readfile(lf, (int *)&tn, 0);
     if (!tb) die("UNISA_STREAM_RESOURCES list");
     while (at < tn && NRI < NRMAX) {
       char *line = tb + at; size_t z = 0, k = 0;
       while (at + z < tn && line[z] != '\n') z++;
       line[z] = 0;
       if (z && line[0] != '#') {
         char *tab = 0;
         for (k = 0; k < z; k++) if (line[k] == '\t') { tab = line + k; break; }
         if (tab) {
           int hn = (int)(tab - line), i2, rl2 = 0; unsigned char *rb;
           *tab = 0;
           if (hn < 2 || hn >= (int)sizeof kb[0] || (hn & 1)) die("resource key hex");
           for (i2 = 0; i2 < hn / 2; i2++) {
             int hi = line[2*i2], lo = line[2*i2+1];
             hi = hi >= '0' && hi <= '9' ? hi - '0' : (hi | 32) - 'a' + 10;
             lo = lo >= '0' && lo <= '9' ? lo - '0' : (lo | 32) - 'a' + 10;
             if (hi < 0 || hi > 15 || lo < 0 || lo > 15) die("resource key hex");
             kb[NRI][i2] = (unsigned char)((hi << 4) | lo);
           }
           rb = readfile(tab + 1, &rl2, 0);
           if (!rb && rl2 != 0) die("resource bytes");
           ri[NRI].name = kb[NRI]; ri[NRI].n = hn / 2; ri[NRI].data = rb; ri[NRI].len = rl2;
           NRI = NRI + 1;
         }
       }
       at += z + 1;
     }
   } }
 in.b=!strcmp(input,"-")?readstream(0,"stdin",&in.n):readfile(input,&in.n,0);
 framed=in.n>=8&&!memcmp(in.b,"USLATTR1",8);
 if(framed){if(strcmp(STAGES[0].name,"e3")&&strcmp(STAGES[0].name,"units"))die("attribute frame requires e3 or units input");if(!attr_strip(&in))die("invalid attribute frame");}
 else {ap=getenv("UNISA_ATTRIBUTES");if(ap&&*ap){ATTRS.b=readfile(ap,&ATTRS.n,1);ATTRS.cap=ATTRS.n;if(ATTRS.b&&!cxsa_valid(ATTRS.b,ATTRS.n))die("malformed external attributes");ri[1].data=ATTRS.b;ri[1].len=ATTRS.n;}}
 bytes=PB+POFF[0];n=PLEN[0];if(PVER==3){owned=xrealloc(0,PRAW[0]);if(unisa_inflate(owned,PRAW[0],bytes,n)||crc((char*)owned,PRAW[0])!=PCRC[0])die("invalid compressed model");if(PRAW[0]<9||memcmp(owned,"UNINETB1N",9))die("package requires binary networks");bytes=owned;n=PRAW[0];}
 model_clear();MODEL_LOADING=1;loadbytes(bytes,n);free(owned);LB=0;MODEL_LOADING=0;
 rc=execute(in.b,in.n,source,&out);free(in.b);if(rc){unload();unpackage();return rc;}
 if(!strcmp(STAGES[0].name,"e1")&&out.n>=16&&!memcmp(out.b,"USLATTR1",8)){Buf copy=out;copy.b=xrealloc(0,out.n);memcpy(copy.b,out.b,out.n);attr_strip(&copy);free(copy.b);}
 rc=cxs_attributes(getenv("UNISA_ATTRIBUTES"),ATTRS.b,ATTRS.n);if(!rc)rc=cxs_write(1,out.b,out.n);free(out.b);unload();unpackage();if(rc)fprintf(stderr,"stream: output or attributes write failed\n");return rc;
}
