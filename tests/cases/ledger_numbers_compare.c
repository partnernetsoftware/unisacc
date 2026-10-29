/* Compare legacy ledger numeric grammar against the CX implementation. */
#include "../../exec/c/resourceledger.h"
int cxsf_hex_digit(int);
int cxsf_length(const char *,int *);
int cxsf_ledger_identity(const char *,char *,size_t,char *);
int cxsf_ledger_key(const char *,size_t,unsigned char *);
int cxsf_ledger_metadata(const char *,const char *,const char *,const char *,int *,int *);
int cxsf_ledger_find(const void *,int,int,const unsigned char *,int);
int cxsf_ledger_next(const void *,int,int,int *,const unsigned char *,int,int *);
int cxsf_stage_label(const char *);
int cxsf_ledger_check_identity(const void *,const char *,const unsigned char *,int,char *,size_t,void (*)(const unsigned char *,int,char *));
typedef struct {unsigned char *key;int kn;int present;unsigned char *value;int vn;char blob[64];} CxLedgerRowCheck;
_Static_assert(sizeof(CxLedgerRowCheck)==sizeof(RLRow),"ledger row size");
_Static_assert(offsetof(CxLedgerRowCheck,blob)==offsetof(RLRow,blob),"ledger blob offset");
static int legacy_metadata(const char *presence,const char *blob,const char *length,const char *digest,int *p,int *n) {
    if (!strcmp(presence,"0")) *p=0; else if (!strcmp(presence,"1")) *p=1; else return 1;
    if (!rl_length(length,n)) return 2;
    if (!*p) { if (strcmp(blob,"-") || *n || strcmp(digest,"-")) return 3; }
    else if (!strcmp(blob,"-") || strchr(blob,'/') || !strcmp(blob,"..") || strlen(blob)>=64 || strlen(digest)!=64) return 4;
    return 0;
}
int main(void) {
    long checks=0,failed=0; char text[64];
    const char *cases[]={"", "0", "00", "01", "1", "2147483647", "2147483648", "999999999999999999999", "-1", "+1", " 1", "1 ", "1x", 0};
    for(int i=-1;i<=256;i++){checks++;if(rl_hexv(i)!=cxsf_hex_digit(i))failed++;}
    unsigned char ka[]={0,1},kb[]={0,2};RLRow rows[3]={{ka,2,1,0,0,{0}},{ka,2,0,0,0,{0}},{kb,2,1,0,0,{0}}};
    RLLedger ledger={0};ledger.rows=rows;ledger.n=3;
    for(int trace=0;trace<2;trace++)for(int q=0;q<3;q++){
        ledger.trace=trace;const unsigned char *key=q==0?ka:q==1?kb:(unsigned char *)"xx";
        const RLRow *r=rl_find(&ledger,key,2);int i=cxsf_ledger_find(rows,3,trace,key,2);
        checks++;if((r? (int)(r-rows):-1)!=i)failed++;
    }
    {const unsigned char data[]="abc";char a[512],b[512],good[65];rl_input_hex(data,3,good);
     for(int id=0;id<2;id++)for(int stage=0;stage<2;stage++)for(int digest=0;digest<2;digest++){
        ledger.has_id=id;strcpy(ledger.stage,stage?"right":"wrong");strcpy(ledger.input_hex,digest?good:"bad");a[0]=b[0]=0;
        int x=rl_check_identity(&ledger,"right",data,3,a,sizeof a),y=cxsf_ledger_check_identity(&ledger,"right",data,3,b,sizeof b,rl_input_hex);
        checks++;if(x!=y||strcmp(a,b))failed++;
     }}
    for(int a=0;a<256;a++)for(int b=0;b<256;b++){
        char key[2]={(char)a,(char)b};unsigned char out=0;int hi=rl_hexv(a),lo=rl_hexv(b);
        int ok=cxsf_ledger_key(key,2,&out);checks++;
        if(ok!=(hi>=0&&lo>=0)||(ok&&out!=(unsigned char)(hi*16+lo)))failed++;
    }
    for(int i=0;cases[i];i++){int a=99,b=99;int x=rl_length(cases[i],&a),y=cxsf_length(cases[i],&b);checks++;if(x!=y||a!=b)failed++;}
    const char *ps[]={"0","1","2",""}, *bs[]={"-","x","..","a/b",""};
    const char *ls[]={"0","1","00","2147483648"}, *ds[]={"-","0","0000000000000000000000000000000000000000000000000000000000000000"};
    for(int p=0;p<4;p++)for(int b=0;b<5;b++)for(int l=0;l<4;l++)for(int d=0;d<3;d++){
        int ap=9,an=9,bp=9,bn=9;int a=legacy_metadata(ps[p],bs[b],ls[l],ds[d],&ap,&an),z=cxsf_ledger_metadata(ps[p],bs[b],ls[l],ds[d],&bp,&bn);
        checks++;if(a!=z||(!a&&(ap!=bp||an!=bn)))failed++;
    }
    for(int i=0;i<10000;i++){int a=99,b=99;snprintf(text,sizeof text,"%u",(unsigned)i*429497u);int x=rl_length(text,&a),y=cxsf_length(text,&b);checks++;if(x!=y||a!=b)failed++;}
    const char *st[]={"e3","lower","source","bad"},*os[]={"lnx","osx","win","bad"},*arch[]={"x86_64","arm64","bad"},*mods[]={"","warn/","multi/","warn/multi/","multi/warn/"},*fmt[]={"tape","image","run","object","bad"};
    char label[160];
    for(int s=0;s<4;s++)for(int o=0;o<4;o++)for(int a=0;a<3;a++)for(int m=0;m<5;m++)for(int f=0;f<5;f++)for(int v=0;v<3;v++){
        snprintf(label,sizeof label,"%s:%s/%s/%s%s/O%d",st[s],os[o],arch[a],mods[m],fmt[f],v);
        checks++;if(rl_stage_label_ok(label)!=cxsf_stage_label(label))failed++;
    }
    char line[160], stage[128], hex[65];
    int n=snprintf(line,sizeof line,"#SC2\tstage=e3:lnx/x86_64\tinput_sha256=%064d",0);
    for(int i=5;i<n;i++){unsigned char save=(unsigned char)line[i];for(int c=0;c<256;c++){
        RLLedger ledger={0};line[i]=(char)c;
        int a=rl_parse_identity(line,&ledger),b=cxsf_ledger_identity(line,stage,sizeof stage,hex);
        checks++;if(a!=b||(a&&(strcmp(ledger.stage,stage)||strcmp(ledger.input_hex,hex))))failed++;
    }line[i]=(char)save;}
    printf("LEDGER_NUMBERS_COMPARE checks=%ld failed=%ld\n",checks,failed);return failed?1:0;
}
