/* The v1 text encoder is shared with the classic reference.  This adapter
   supplies storage and SHA-256; it makes no tape or target decisions. */
#define MAXOUT 33554432
#define TB_MAXPOOL 262144
static char *out;
static int nout;
static char *tb_file;
static int tb_name_at[TB_MAXPOOL], tb_name_len[TB_MAXPOOL], tb_nn;
static int tb_const_at[TB_MAXPOOL], tb_const_len[TB_MAXPOOL], tb_nc;
static int tb_bad;
static int tb_fail(void) { tb_bad=1; return 0; }
static int bk_hex(int c) {
    if(c>=48&&c<=57)return c-48;
    c|=32; return c>=97&&c<=102?c-87:-1;
}
static void tb_hash(char *dst,char *bytes,int n) {
    tbc_sha((const unsigned char *)bytes,(size_t)n,(unsigned char *)dst);
}
#include "../../src/tapebin_encode.inc"
static int tbc_encode_product(Buf *tape,int origin) {
    if(tape->n<0||tape->n>=MAXOUT)return 1;
    out=(char *)tape->b; nout=tape->n;
    tb_file=malloc(MAXOUT);
    if(!tb_file)return 1;
    int length=tb_encode(nout,origin);
    if(length<=0){free(tb_file);tb_file=0;return 1;}
    free(tape->b);free(tape->at);
    tape->b=(unsigned char *)tb_file;tape->at=0;
    tape->n=length;tape->cap=MAXOUT;
    tb_file=0;
    return 0;
}
#undef MAXOUT
