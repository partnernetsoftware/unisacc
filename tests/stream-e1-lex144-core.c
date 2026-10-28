/* Diagnostic boundary adapter over the actual deployed generic NN evaluator.
 * STREAM2 E1_TEXT_NET QUERIES. No lexer decisions or expected labels here.
 * Each query is: id state observation. Output includes actual actions.
 * Before answering, compare every state/observation and expanded action against
 * the formal same-generation text network. State IDs must therefore agree. */
#define main lex144_unused_main
#include "../exec/c/run.c"
#undef main
static int sequence_equal(const CoreModel *a,int x,const CoreModel *b,int y){
 if(a->qlen[x]!=b->qlen[y])return 0;int p=a->qoff[x],q=b->qoff[y];
 for(int i=0;i<a->qlen[x];i++){int op=(int)a->qa[p];if(op!=b->qa[q])return 0;for(int j=0;j<=ARITY[op];j++)if(a->qa[p+j]!=b->qa[q+j])return 0;p+=1+ARITY[op];q+=1+ARITY[op];}return 1;
}
int main(int argc,char **argv){
 if(argc!=4){fputs("lex144-core: usage STREAM2 E1_TEXT_NET QUERIES\n",stderr);return 2;}
 package(argv[1]);if(PM!=1||PS!=1||strcmp(STAGES[0].route,"stream")||strcmp(STAGES[0].name,"e1"))die("lex144-core: one-e1-required");
 unsigned char *bytes=PB+POFF[0],*owned=0;int n=PLEN[0];if(PVER==3){owned=xrealloc(0,PRAW[0]);if(unisa_inflate(owned,PRAW[0],bytes,n)||crc((char *)owned,PRAW[0])!=PCRC[0])die("lex144-core: model-codec");bytes=owned;n=PRAW[0];}
 model_clear();loadbytes(bytes,n);free(owned);CoreModel actual;core_model(&actual);Model held={0};model_capture(&held);model_clear();
 load(argv[2]);CoreModel source;core_model(&source);
 if(!actual.isnet||!source.isnet||actual.ns!=source.ns||actual.nrg!=source.nrg||actual.start!=source.start||actual.strl==0)die("lex144-core: source-model-shape");
 if(held.nstr!=NSTR||held.topmax!=TOPMAX)die("lex144-core: source-symbol-count");
 for(int i=0;i<NSTR;i++)if(held.strl[i]!=STRL[i]||memcmp(held.str[i],STR[i],(size_t)STRL[i]))die("lex144-core: source-symbol-bytes");
 long checked=0;
 for(int q=0;q<actual.ns;q++){
  if(actual.mode[q]!=source.mode[q]||actual.lo[q]!=source.lo[q]||actual.hi[q]!=source.hi[q])die("lex144-core: source-observation-domain");
  for(int key=actual.lo[q];key<=actual.hi[q];key++){int an,as,bn,bs;const char *a=core_transition(&actual,q,key,&an,&as),*b=core_transition(&source,q,key,&bn,&bs);if(a||b||an!=bn||!sequence_equal(&actual,as,&source,bs))die("lex144-core: source-model-transition-or-action");checked++;}
 }
 printf("BIND\tstates=%d\ttransitions=%ld\texpanded-actions=equal\n",actual.ns,checked);
 int qn=0;unsigned char *queries=readfile(argv[3],&qn,0);char *p=malloc((size_t)qn+1);if(!p)die("lex144-core: allocation");memcpy(p,queries,(size_t)qn);p[qn]=0;free(queries);char *cursor=p;
 while(*cursor){while(*cursor==' '||*cursor=='\t'||*cursor=='\n')cursor++;if(!*cursor)break;long v[3];char *end;for(int i=0;i<3;i++){v[i]=strtol(cursor,&end,10);if(end==cursor||(*end&&*end!=' '&&*end!='\t'&&*end!='\n'))die("lex144-core: query-number");cursor=end;while(*cursor==' '||*cursor=='\t')cursor++;}int id=(int)v[0],q=(int)v[1],key=(int)v[2];if(id<0||q<0||q>=actual.ns)die("lex144-core: query-bound");int nx,sq;const char *why=core_transition(&actual,q,key,&nx,&sq);if(why)die(why);printf("Q\t%d\t%d\t%d\t%d\t%d",id,q,key,nx,actual.qlen[sq]);int at=actual.qoff[sq];for(int i=0;i<actual.qlen[sq];i++){int op=(int)actual.qa[at];for(int j=0;j<=ARITY[op];j++)printf("\t%lld",(long long)actual.qa[at+j]);at+=1+ARITY[op];}putchar('\n');}
 free(p);unload();model_release(&held);unpackage();return 0;
}
