/* tapebin v1 container reader for the model driver.  This is a byte-stream
   adapter: it validates the package and prints canonical tape records; E4,
   prune, lower and image decisions still run through the package's networks. */
#include "../../src/tapebin_shape.inc"

static const uint32_t tbc_k[64] = {
    1116352408u,1899447441u,3049323471u,3921009573u,961987163u,1508970993u,2453635748u,2870763221u,
    3624381080u,310598401u,607225278u,1426881987u,1925078388u,2162078206u,2614888103u,3248222580u,
    3835390401u,4022224774u,264347078u,604807628u,770255983u,1249150122u,1555081692u,1996064986u,
    2554220882u,2821834349u,2952996808u,3210313671u,3336571891u,3584528711u,113926993u,338241895u,
    666307205u,773529912u,1294757372u,1396182291u,1695183700u,1986661051u,2177026350u,2456956037u,
    2730485921u,2820302411u,3259730800u,3345764771u,3516065817u,3600352804u,4094571909u,275423344u,
    430227734u,506948616u,659060556u,883997877u,958139571u,1322822218u,1537002063u,1747873779u,
    1955562222u,2024104815u,2227730452u,2361852424u,2428436474u,2756734187u,3204031479u,3329325298u
};

static uint32_t tbc_rotr(uint32_t x,int n){return (x>>n)|(x<<(32-n));}
static void tbc_block(uint32_t h[8],const unsigned char p[64]){
    uint32_t w[64];
    for(int i=0;i<16;i++)w[i]=((uint32_t)p[4*i]<<24)|((uint32_t)p[4*i+1]<<16)|((uint32_t)p[4*i+2]<<8)|p[4*i+3];
    for(int i=16;i<64;i++)w[i]=w[i-16]+(tbc_rotr(w[i-15],7)^tbc_rotr(w[i-15],18)^(w[i-15]>>3))+
        w[i-7]+(tbc_rotr(w[i-2],17)^tbc_rotr(w[i-2],19)^(w[i-2]>>10));
    uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],x=h[7];
    for(int i=0;i<64;i++){
        uint32_t t=x+(tbc_rotr(e,6)^tbc_rotr(e,11)^tbc_rotr(e,25))+((e&f)^(~e&g))+tbc_k[i]+w[i];
        uint32_t u=(tbc_rotr(a,2)^tbc_rotr(a,13)^tbc_rotr(a,22))+((a&b)^(a&c)^(b&c));
        x=g;g=f;f=e;e=d+t;d=c;c=b;b=a;a=t+u;
    }
    h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=x;
}
static void tbc_sha(const unsigned char *p,size_t n,unsigned char out[32]){
    uint32_t h[8]={1779033703u,3144134277u,1013904242u,2773480762u,1359893119u,2600822924u,528734635u,1541459225u};
    size_t at=0;while(n-at>=64){tbc_block(h,p+at);at+=64;}
    unsigned char tail[128]={0};size_t rest=n-at;memcpy(tail,p+at,rest);tail[rest]=128;
    uint64_t bits=(uint64_t)n*8;size_t total=rest+9<=64?64:128;
    for(int i=0;i<8;i++)tail[total-1-i]=(unsigned char)(bits>>(8*i));
    tbc_block(h,tail);if(total==128)tbc_block(h,tail+64);
    for(int i=0;i<8;i++)for(int j=0;j<4;j++)out[4*i+j]=(unsigned char)(h[i]>>(24-8*j));
}
typedef struct {const unsigned char *p;size_t pos,end;int bad;} TbcReader;
static uint64_t tbc_le(const unsigned char *p,int n){uint64_t v=0;for(int i=n-1;i>=0;i--)v=(v<<8)|p[i];return v;}
static uint64_t tbc_u(TbcReader *r){
    uint64_t v=0;int k=0;
    for(;k<10;k++){
        if(r->pos>=r->end){r->bad=1;return 0;}
        int c=r->p[r->pos++];if(k==9&&c>1){r->bad=1;return 0;}
        v|=(uint64_t)(c&127)<<(7*k);
        if(c<128){if(k&&!c)r->bad=1;return v;}
    }r->bad=1;return 0;
}
static int64_t tbc_s(TbcReader *r){
    uint64_t v=0;int k=0,c=0;
    for(;k<10;k++){
        if(r->pos>=r->end){r->bad=1;return 0;}
        c=r->p[r->pos++];if(k==9&&c!=0&&c!=127){r->bad=1;return 0;}
        v|=(uint64_t)(c&127)<<(7*k);
        if(c<128){
            if(7*k+7<64&&(c&64))v|=UINT64_MAX<<(7*k+7);
            if(k&&((c==0&&!(r->p[r->pos-2]&64))||(c==127&&(r->p[r->pos-2]&64))))r->bad=1;
            return (int64_t)v;
        }
    }r->bad=1;return 0;
}
static void tbc_put(Buf *b,const char *s){while(*s)bput(b,(unsigned char)*s++,0);}
static void tbc_num(Buf *b,int64_t v){char tmp[32];snprintf(tmp,sizeof tmp,"%lld",(long long)v);tbc_put(b,tmp);}
static void tbc_unum(Buf *b,uint64_t v){char tmp[32];snprintf(tmp,sizeof tmp,"%llu",(unsigned long long)v);tbc_put(b,tmp);}
static void tbc_name(Buf *b,const unsigned char *p,size_t n){for(size_t i=0;i<n;i++)bput(b,p[i],0);}
static void tbc_quote(Buf *b,const unsigned char *p,size_t n){
    static const char hx[]="0123456789abcdef";bput(b,'"',0);
    for(size_t i=0;i<n;i++){
        int c=p[i];
        if(c==10)tbc_put(b,"\\n");else if(c==9)tbc_put(b,"\\t");
        else if(c==13)tbc_put(b,"\\r");else if(c==0)tbc_put(b,"\\0");
        else if(c==92)tbc_put(b,"\\\\");else if(c==34)tbc_put(b,"\\\"");
        else if(c>=32&&c<127)bput(b,c,0);
        else{tbc_put(b,"\\x");bput(b,hx[c>>4],0);bput(b,hx[c&15],0);}
    }bput(b,'"',0);
}
static void tbc_arg(Buf *b,char typ,int tag,uint64_t val,
                    const unsigned char **names,const size_t *lens,size_t nn){
    if(typ=='r'){bput(b,'r',0);bput(b,'0'+(int)val,0);}
    else if(tag==1&&typ!='i'){if(val<nn)tbc_name(b,names[val],lens[val]);}
    else if(tag==1)tbc_unum(b,val);
    else tbc_num(b,(int64_t)val);
}
static void tbc_mem(Buf*b,uint64_t reg,uint64_t off){
    bput(b,'[',0);bput(b,'r',0);bput(b,'0'+(int)reg,0);
    if((int64_t)off>=0)bput(b,'+',0);
    tbc_num(b,(int64_t)off);bput(b,']',0);
}
static void tbc_insn(Buf *b,int op,const uint64_t *v,const int *tag,
                     const unsigned char **names,const size_t *lens,size_t nn){
    const char *name=tb_opnames[op],*shape=tb_shapes[op];
    tbc_put(b,"  ");tbc_put(b,name);int pad=7-(int)strlen(name);while(pad-->0)bput(b,' ',0);
    if(*shape)bput(b,' ',0);
    if(op==30){tbc_mem(b,v[0],v[1]);} /* callm */
    else if(op==21||op==23){ /* load64, .ld */
        tbc_arg(b,'r',0,v[0],names,lens,nn);tbc_put(b,", ");tbc_mem(b,v[1],v[2]);
        if(op==23){tbc_put(b,", ");tbc_arg(b,'i',tag[3],v[3],names,lens,nn);}
    }else if(op==22||op==24){ /* store64, .st */
        tbc_mem(b,v[0],v[1]);tbc_put(b,", ");tbc_arg(b,'r',0,v[2],names,lens,nn);
        if(op==24){tbc_put(b,", ");tbc_arg(b,'i',tag[3],v[3],names,lens,nn);}
    }else if(op==26){ /* .zero */
        tbc_mem(b,v[0],v[1]);tbc_put(b,", ");tbc_arg(b,'i',tag[2],v[2],names,lens,nn);
    }else{
        for(int i=0;shape[i];i++){
            if(i)tbc_put(b,", ");tbc_arg(b,shape[i],tag[i],v[i],names,lens,nn);
        }
    }
    bput(b,'\n',0);
}
static int tbc_decode(const unsigned char *data,size_t size,int expected,int force,Buf *text){
    if(size<136||size>33554432||memcmp(data,"UTAPEBIN",8))return 1;
    if(tbc_le(data+8,2)!=1||tbc_le(data+10,2)||tbc_le(data+12,2)!=1||tbc_le(data+14,2)||
       tbc_le(data+16,4)||tbc_le(data+24,4)!=64)return 1;
    uint64_t count=tbc_le(data+20,4),origin=tbc_le(data+60,4);
    if(count<3||count>4||origin>6||64+24*count>size)return 1;
    if(!force&&expected&&origin&&origin!=(uint64_t)expected)return 1;
    unsigned char digest[32];tbc_sha(data+64,size-64,digest);
    if(memcmp(digest,data+28,32))return 1;
    size_t at[4]={0},length[4]={0},items[4]={0},end=64+24*count;
    for(size_t i=0;i<count;i++){
        const unsigned char *d=data+64+24*i;
        uint64_t kind=tbc_le(d,2),flags=tbc_le(d+2,2),off=tbc_le(d+4,8),n=tbc_le(d+12,8),c=tbc_le(d+20,4);
        size_t aligned=end+(-end&7);
        if(kind!=i+1||flags!=(kind<=3?1u:0u)||off!=aligned||off>size||n>size-off||c>size)return 1;
        for(size_t k=end;k<off;k++)if(data[k])return 1;
        at[i]=(size_t)off;length[i]=(size_t)n;items[i]=(size_t)c;end=(size_t)(off+n);
    }
    if(end!=size||(count==4&&(length[3]!=32||items[3]!=1)))return 1;
    const unsigned char **names=NULL,**consts=NULL;size_t *namelen=NULL,*constlen=NULL;
    int bad=1;
    if(items[0]>262144||items[1]>262144)return 1;
    names=xrealloc(0,(items[0]+1)*sizeof *names);namelen=xrealloc(0,(items[0]+1)*sizeof *namelen);
    consts=xrealloc(0,(items[1]+1)*sizeof *consts);constlen=xrealloc(0,(items[1]+1)*sizeof *constlen);
    for(int pool=0;pool<2;pool++){
        TbcReader r={data+at[pool],0,length[pool],0};uint64_t n=tbc_u(&r);
        if(r.bad||n!=items[pool])goto done;
        for(size_t i=0;i<n;i++){
            uint64_t len=tbc_u(&r);if(r.bad||len>r.end-r.pos||(pool==0&&!len))goto done;
            if(pool==0){
                for(size_t j=0;j<len;j++){int c=r.p[r.pos+j];if(!c||c==32||c==9||c==10||c==13||c==58)goto done;}
                names[i]=r.p+r.pos;namelen[i]=(size_t)len;
            }else{consts[i]=r.p+r.pos;constlen[i]=(size_t)len;}
            r.pos+=(size_t)len;
        }if(r.pos!=r.end)goto done;
    }
    TbcReader r={data+at[2],0,length[2],0};uint64_t nr=tbc_u(&r);
    if(r.bad||nr!=items[2]||nr>r.end)goto done;
    for(uint64_t row=0;row<nr;row++){
        if(r.pos>=r.end)goto done;int kind=r.p[r.pos++];
        if(kind==TB_RECORD_GLOBAL||kind==TB_RECORD_EXTERN){
            uint64_t id=tbc_u(&r);if(r.bad||id>=items[0])goto done;
            tbc_put(text,kind==TB_RECORD_GLOBAL?".global ":".extern ");tbc_name(text,names[id],namelen[id]);tbc_put(text,"\n");
        }else if(kind<=2){
            uint64_t id=tbc_u(&r);if(r.bad||id>=items[0])goto done;
            if(kind==0){tbc_name(text,names[id],namelen[id]);tbc_put(text,":\n");}
            else if(kind==1){
                uint64_t ci=tbc_u(&r);if(r.bad||ci>=items[1])goto done;
                tbc_put(text,".str ");tbc_name(text,names[id],namelen[id]);bput(text,' ',0);
                tbc_quote(text,consts[ci],constlen[ci]);bput(text,'\n',0);
            }else{
                uint64_t amount=tbc_u(&r);if(r.bad||amount>2147483647u)goto done;
                tbc_put(text,".bss ");tbc_name(text,names[id],namelen[id]);bput(text,' ',0);
                tbc_unum(text,amount);bput(text,'\n',0);
            }
        }else if(kind==3){
            if(r.pos>=r.end)goto done;int op=r.p[r.pos++];if(op>=TB_OPCOUNT)goto done;
            const char *shape=tb_shapes[op];int arity=(int)strlen(shape),nregs=0;
            if(arity>8)goto done;for(int i=0;i<arity;i++)if(shape[i]=='r')nregs++;
            if((size_t)((nregs+1)/2)>r.end-r.pos)goto done;
            const unsigned char *packed=r.p+r.pos;r.pos+=(nregs+1)/2;
            if((nregs&1)&&packed[nregs/2]>>4!=15)goto done;
            uint64_t v[8]={0};int tag[8]={0},ri=0;
            for(int i=0;i<arity;i++){
                if(shape[i]=='r'){
                    v[i]=(packed[ri/2]>>(4*(ri%2)))&15;ri++;
                    if(v[i]>7)goto done;
                }else{
                    if(r.pos>=r.end)goto done;tag[i]=r.p[r.pos++];if(tag[i]>1)goto done;
                    v[i]=tag[i]?tbc_u(&r):(uint64_t)tbc_s(&r);
                    if(r.bad||(tag[i]&&shape[i]!='i'&&v[i]>=items[0]))goto done;
                }
            }
            tbc_insn(text,op,v,tag,names,namelen,items[0]);
        }else goto done;
        if(text->n>33554432)goto done;
    }
    if(r.bad||r.pos!=r.end)goto done;
    bad=0;
done:
    free(names);free(namelen);free(consts);free(constlen);
    if(bad){free(text->b);free(text->at);memset(text,0,sizeof *text);}
    return bad;
}
