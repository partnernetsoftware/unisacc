#ifndef UNISACC_FORWARD_SIGNATURE_H
#define UNISACC_FORWARD_SIGNATURE_H
/* Decode only the scalar/pointer subset needed by the macOS -run forwarding
   adapter.  USLSIG3 is produced by E3; this reader makes no C type decision. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    int kind;               /* fwd_stub: 0 integer, 1 pointer, 4 float, 8 double */
    int width, uns, ptr, depth, isvoid, structbyval;
    uint64_t base;           /* pointee width for a scalar pointer */
    const unsigned char *callback; /* borrowed USLSIG3 callable graph */
    size_t callback_len;
} USForwardType;
typedef struct {
    int variadic, structbyval, n;
    USForwardType ret, args[33];
} USForwardSig;

static int us_fw_u64(const unsigned char *b,size_t n,size_t *at,uint64_t *v) {
    if (*at>n || n-*at<8) return 1;
    *v=0;
    for (int i=0;i<8;i++) *v|=(uint64_t)b[*at+i]<<(8*i);
    *at+=8;
    return 0;
}
static int us_fw_type(const unsigned char *b,size_t n,size_t *at,USForwardType *t,int result) {
    uint64_t depth,base,shape,kind,width,uns,alignment,natural,payload;
    unsigned tag;
    if (us_fw_u64(b,n,at,&depth)||us_fw_u64(b,n,at,&base)||
        us_fw_u64(b,n,at,&shape)||us_fw_u64(b,n,at,&kind)||
        us_fw_u64(b,n,at,&width)||us_fw_u64(b,n,at,&uns)||
        us_fw_u64(b,n,at,&alignment)||n-*at<2) return 1;
    unsigned fp_rank=b[(*at)++],fp_format=b[(*at)++];
    if (us_fw_u64(b,n,at,&natural)||n-*at<4) return 1;
    unsigned flags=b[(*at)++],known=b[(*at)++],origin=b[(*at)++];
    tag=b[(*at)++];
    if (us_fw_u64(b,n,at,&payload)||payload>n-*at||payload>16777216||
        uns>1||depth>1024||kind>6||width>16777216||tag>5||
        fp_rank>3||fp_format>4||flags>3||known>3||origin>2||
        (flags&~known)||natural>65536||alignment>65536||
        (alignment && (alignment&(alignment-1)))||
        (natural && (natural&(natural-1)))) return 1;
    (void)shape;
    memset(t,0,sizeof *t);
    t->width=(int)width;t->uns=(int)uns;t->ptr=depth!=0;t->depth=(int)depth;t->base=base;
    if (kind==0 && result && !depth && !width && !payload) t->isvoid=1;
    else if (kind==1 && !depth && !payload && tag==0 &&
             (width==1||width==2||width==4||width==8)) t->kind=0;
    else if (kind==2 && depth && width==8 && (tag==0||tag==5) &&
             (tag==5||!payload)) t->kind=1;
    /* A function pointer has a callable identity graph in USLSIG3, but the
       host ABI passes its value in one pointer slot.  Keep the graph bytes in
       the validated enclosing signature; the forwarding stub needs its ABI kind. */
    else if (kind==4 && depth==1 && width==8 && tag==4 && payload>=20) {
        t->kind=1;t->callback=b+*at;t->callback_len=(size_t)payload;
    }
    else if (kind==3 && !depth && !payload && tag==0 &&
             (width==4||width==8)) t->kind=width==4 ? 4:8;
    else if (kind==5 && !depth && (tag==1||tag==2||tag==3)) t->structbyval=1;
    else return 1;
    *at+=(size_t)payload;
    return 0;
}
/* Decode the non-variadic, one-level callback subset into ABI types.  The
   product stub prints these as C declarators so its second compilation sees
   the same function-pointer declaration as the source unit. */
typedef struct { int n; USForwardType ret, args[8]; } USForwardCallback;
static int us_forward_callback_decode(const USForwardType *t,USForwardCallback *out) {
    if (!t || !t->callback || !out || t->callback_len<20) return 1;
    const unsigned char *b=t->callback;size_t n=t->callback_len,at=0;
    if (b[at++]!=1 || b[at++]!=0) return 1; /* graph definition, not a reference */
    uint64_t id,np,stored;
    if (us_fw_u64(b,n,&at,&id) || id==0 || id>1024 || n-at<2) return 1;
    unsigned variadic=b[at++],mode=b[at++];
    if (variadic || mode || us_fw_u64(b,n,&at,&np) || np>8 ||
        us_fw_type(b,n,&at,&out->ret,1) ||
        us_fw_u64(b,n,&at,&stored) || stored!=np) return 1;
    out->n=(int)np;
    for (int i=0;i<out->n;i++) if (us_fw_type(b,n,&at,&out->args[i],0)) return 1;
    return at+1==n && b[at]==0 ? 0 : 1;
}
/* A standalone single-record LX.signature blob.  The outer USLFW1 name must
   match this owned name; duplicate/mismatched declarations are never guessed. */
static int us_forward_sig_decode(const unsigned char *b,size_t n,
                                 const char *name,size_t namelen,USForwardSig *out) {
    size_t at=8;uint64_t count,inner_name,np,stored;
    if (!b||!name||!out||n<16||memcmp(b,"USLSIG3\n",8)||
        us_fw_u64(b,n,&at,&count)||count!=1||
        us_fw_u64(b,n,&at,&inner_name)||inner_name!=namelen||
        inner_name>n-at||memcmp(b+at,name,namelen)) return 1;
    at+=namelen;
    if (n-at<4 || b[at]!=0 || b[at+1]!=1 || b[at+2]>1 || b[at+3]>1) return 1;
    USForwardSig tmp={0};tmp.variadic=b[at+2];at+=4;
    if (us_fw_u64(b,n,&at,&np)||np>33||us_fw_type(b,n,&at,&tmp.ret,1)||
        us_fw_u64(b,n,&at,&stored)||stored!=np) return 1;
    tmp.n=(int)np;tmp.structbyval=tmp.ret.structbyval;
    for (int i=0;i<tmp.n;i++) {
        if (us_fw_type(b,n,&at,&tmp.args[i],0)) return 1;
        tmp.structbyval|=tmp.args[i].structbyval;
    }
    if (at>=n||b[at++]!=0||at!=n) return 1; /* V3 is never ABI-certified. */
    *out=tmp;
    return 0;
}
#endif
