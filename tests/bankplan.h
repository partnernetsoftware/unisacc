/* BNK1 plan decoding and execution against a register-bank frame (R12-1 ①).
   Mechanical only: bounds and consistency checks, byte moves, result capture.
   No ABI classification happens here; the plan says everything. */
#ifndef US_BANKPLAN_H
#define US_BANKPLAN_H
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

typedef struct { uint64_t gp[8]; unsigned char fp[8][16]; uint64_t hidden; uint64_t stack_bytes; unsigned char *stack_image;
                 uint64_t al; uint64_t rax, rdx; unsigned char xmm0[16], xmm1[16], v2[16], v3[16]; } us_bank_frame;   /* v2/v3: AAPCS64 HFA results */
typedef struct { const unsigned char *rec; uint32_t length; uint32_t proto_len; unsigned params, gp_used, fp_used, al, moves, results, hidden, profile, flags;
                 unsigned stack_bytes, scratch_bytes; const unsigned char *move, *result; } us_bank_plan;

static uint16_t us_bp_u16(const unsigned char *p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t us_bp_u32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint32_t us_bp_crc(const unsigned char *b,size_t n){uint32_t c=0xffffffffu;for(size_t i=0;i<n;i++){c^=b[i];for(int k=0;k<8;k++)c=(c>>1)^(0xedb88320u&(0u-(c&1)));}return ~c;}

/* Returns 0 on a well-formed plan; nonzero names the first violated rule. */
static int us_bank_plan_load(us_bank_plan *p,const unsigned char *rec,size_t n){
    memset(p,0,sizeof *p);
    if(n<32||memcmp(rec,"BNK1",4)||us_bp_u16(rec+4)!=1)return 1;
    p->rec=rec;p->profile=rec[6];p->flags=rec[7];p->length=us_bp_u32(rec+8);p->proto_len=us_bp_u32(rec+12);
    p->params=rec[16];p->gp_used=rec[17];p->fp_used=rec[18];p->al=rec[19];p->stack_bytes=us_bp_u16(rec+20);p->moves=us_bp_u16(rec+22);
    p->results=rec[24];p->hidden=rec[25];p->scratch_bytes=us_bp_u16(rec+26);
    if(p->length!=n||p->profile>5||(p->flags&~1u)||p->proto_len>4096||p->params>32||p->gp_used>8||p->fp_used>8||p->al>8)return 2;
    if(p->stack_bytes%16||p->stack_bytes>512||p->scratch_bytes%16||p->scratch_bytes>1024||p->moves>256||p->results>8)return 3;
    if(32u+p->proto_len+8u*p->moves+4u*p->results!=n)return 4;
    if(us_bp_crc(rec+32,n-32)!=us_bp_u32(rec+28))return 5;
    if(!(p->flags&1)!=(p->hidden==0xff))return 6;
    p->move=rec+32+p->proto_len;p->result=p->move+8u*p->moves;
    unsigned char gpmask[8][8]={{0}},fpmask[8][16]={{0}};unsigned last_stack=0,last_scr=0;
    for(unsigned i=0;i<p->moves;i++){
        const unsigned char *m=p->move+8*i;unsigned param=m[0],sk=m[1],dk=m[4],w=m[6];
        int pow2=(w==1||w==2||w==4||w==8||w==16);
        if(param>=p->params||sk>2||dk>4||w==0||w>16||(dk==0&&w>8)||((dk==1||dk==2)&&!pow2))return 10;   /* GP takes any 1..8 bytes (AAPCS64 small composites), FP lanes are 4/8/16 */
        if(dk==0){if(m[5]>=p->gp_used||w>8||m[7]>1)return 11;for(unsigned b=0;b<w;b++){if(gpmask[m[5]][b]++)return 12;}}
        else if(dk==1||dk==2){if(m[5]>=p->fp_used||(dk==2&&w>8))return 13;unsigned base=dk==2?8:0;for(unsigned b=0;b<w;b++){if(fpmask[m[5]][base+b]++)return 14;}}
        else{unsigned off=m[5]|(unsigned)m[7]<<8,lim=dk==3?p->stack_bytes:p->scratch_bytes;
             if(off+w>lim||(pow2&&off%(w<8?w:8)))return 15;
             unsigned *last=dk==3?&last_stack:&last_scr;if(i&&off<*last)return 16;*last=off+w;}
        if(sk==2&&dk==4)return 17;
    }
    for(unsigned i=0;i<p->results;i++){const unsigned char *r=p->result+4*i;if(r[0]>5||!(r[1]==1||r[1]==2||r[1]==4||r[1]==8))return 20;}
    if(p->hidden!=0xff&&p->hidden!=0&&p->hidden!=8)return 21;
    return 0;
}

/* slots: one 64-bit word per parameter (scalar value, or pointer to the argument bytes);
   sizes: byte size of each parameter's object. Fills the frame; caller provides stack/scratch storage. */
static int us_bank_plan_apply(const us_bank_plan *p,const uint64_t *slots,const size_t *sizes,us_bank_frame *f,
                              unsigned char *stack,unsigned char *scratch,void *result_memory){
    memset(f,0,sizeof *f);memset(stack,0,p->stack_bytes);if(scratch)memset(scratch,0,p->scratch_bytes);
    f->stack_bytes=p->stack_bytes;f->stack_image=stack;f->al=p->al;
    if(p->flags&1){if(!result_memory)return 30;f->hidden=(uint64_t)(uintptr_t)result_memory;}
    for(unsigned i=0;i<p->moves;i++){
        const unsigned char *m=p->move+8*i;unsigned param=m[0],sk=m[1],dk=m[4],w=m[6];unsigned so=us_bp_u16(m+2);
        unsigned char src[16]={0};
        if(sk==0){uint64_t v=slots[param];if(so||w>8)return 31;memcpy(src,&v,8);}
        else if(sk==1){const unsigned char *b=(const unsigned char*)(uintptr_t)slots[param];if(!b||so+w>sizes[param]+7)return 32;
                       size_t avail=sizes[param]>so?sizes[param]-so:0;memcpy(src,b+so,avail<w?avail:w);}
        else{if(!scratch||so+w>p->scratch_bytes)return 33;memcpy(src,scratch+so,w);}
        if(dk==0){uint64_t v=0;memcpy(&v,src,w);
                  if(m[7]==1&&w<8&&(src[w-1]&0x80))v|=~0ull<<(8*w);   /* sign-extend */
                  f->gp[m[5]]=v;}
        else if(dk==1)memcpy(f->fp[m[5]],src,w);
        else if(dk==2)memcpy(f->fp[m[5]]+8,src,w);
        else if(dk==3)memcpy(stack+(m[5]|(unsigned)m[7]<<8),src,w);
        else memcpy(scratch+(m[5]|(unsigned)m[7]<<8),src,w);
    }
    /* by-reference scratch objects: a later GP/STACK move with src kind 2 wants the ADDRESS of scratch+offset,
       encoded by the oracle as width 8 from scratch offset: patch those moves to carry the pointer. */
    for(unsigned i=0;i<p->moves;i++){
        const unsigned char *m=p->move+8*i;if(m[1]!=2)continue;unsigned so=us_bp_u16(m+2),dk=m[4];uint64_t addr=(uint64_t)(uintptr_t)(scratch+so);
        if(dk==0)f->gp[m[5]]=addr;else if(dk==3)memcpy(stack+(m[5]|(unsigned)m[7]<<8),&addr,8);
    }
    return 0;
}

static void us_bank_plan_capture(const us_bank_plan *p,const us_bank_frame *f,void *out){
    for(unsigned i=0;i<p->results;i++){const unsigned char *r=p->result+4*i;unsigned src=r[0],w=r[1],off=us_bp_u16(r+2);
        const void *from=src==0?(const void*)&f->rax:src==1?(const void*)&f->rdx:src==2?(const void*)f->xmm0:src==3?(const void*)f->xmm1:src==4?(const void*)f->v2:(const void*)f->v3;
        memcpy((unsigned char*)out+off,from,w);}
}
#endif
