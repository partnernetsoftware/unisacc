/* GENERATED -- do not edit.  `python3 -m unisa emit-kernel` writes this
 * file; tests/kernel.sh fails when it differs from a fresh run.
 * The model: every stage's constructed weights, their dimensions, and the
 * vocabularies both sides key by.  Nothing here is typed by hand: the weights
 * are weights/built.json, which `unisa build-weights` CONSTRUCTS from the
 * gold tables in unisa/gold.py (no training); the vocabularies are the
 * tuples named at each string.  The kernel that reads this is unisa_core.c.
 * Inputs (sha256, first 12 hex digits):
 *   unisa/gold.py                25721a152d0d
 *   unisa/catalog.py             07ff7baa3166
 *   unisa/front/lex.py           67f2248c3a8a
 *   weights/built.json           8b10e43e2b3d
 *   unisa/ckernel.py             d54d70c049f3 */

#ifndef UNISA_MODEL_INC
#define UNISA_MODEL_INC

/* S_<stage>: the stage order of gold.ALL */
#define S_PP 0
#define S_LEX 1
#define S_PARSE 2
#define S_TYPE 3
#define S_SCOPE 4
#define S_IRSEL 5
#define S_ENC 6
#define S_RELOC 7
#define S_REGMAP 8
#define S_TYINFO 9
#define S_PFCONV 10
#define S_PEEP 11
#define S_OPINFO 12
#define S_PREC 13
#define S_BINSEL 14
#define S_ISEL 15
#define S_ABI 16
#define S_COMBO 17

/* MODEL: every stage's units, packed: per stage [nfields][field masks][nw2][(unit, class, w2)...]
 * -- the constructed weights of weights/built.json, in gold.ALL order */
char *MODEL =
#include "weight.pp.inc"
#include "weight.lex.inc"
#include "weight.parse.inc"
#include "weight.type.inc"
#include "weight.scope.inc"
#include "weight.irsel.inc"
#include "weight.enc.inc"
#include "weight.reloc.inc"
#include "weight.regmap.inc"
#include "weight.tyinfo.inc"
#include "weight.pfconv.inc"
#include "weight.peep.inc"
#include "weight.opinfo.inc"
#include "weight.prec.inc"
#include "weight.binsel.inc"
#include "weight.isel.inc"
#include "weight.abi.inc"
#include "weight.combo.inc"
;

#define NSTAGE 18

int STAGE_M[18];
int STAGE_H[18];
int STAGE_OFF[18];
int STAGE_NCLS[288];
int STAGE_MW[18];
int STAGE_VN[72];
int act[4096];
int z[512];

/* DENSE: each stage's answers for every key, head-minor, as the constructed net
 * computes them (intnet.predict over the full domain) -- the net compiled to a
 * table [J1] */
char *DENSE =
#include "dense.pp.inc"
#include "dense.lex.inc"
#include "dense.parse.inc"
#include "dense.type.inc"
#include "dense.scope.inc"
#include "dense.irsel.inc"
#include "dense.enc.inc"
#include "dense.reloc.inc"
#include "dense.regmap.inc"
#include "dense.tyinfo.inc"
#include "dense.pfconv.inc"
#include "dense.peep.inc"
#include "dense.opinfo.inc"
#include "dense.prec.inc"
#include "dense.binsel.inc"
#include "dense.isel.inc"
#include "dense.abi.inc"
#include "dense.combo.inc"
;
#define DENSE_LEN 20368
int STAGE_DOFF[18]; int STAGE_NH[18];

int model_dims(void) {
  STAGE_DOFF[0] = 0; STAGE_NH[0] = 1;
  STAGE_DOFF[1] = 18; STAGE_NH[1] = 1;
  STAGE_DOFF[2] = 162; STAGE_NH[2] = 1;
  STAGE_DOFF[3] = 502; STAGE_NH[3] = 1;
  STAGE_DOFF[4] = 4777; STAGE_NH[4] = 1;
  STAGE_DOFF[5] = 4807; STAGE_NH[5] = 1;
  STAGE_DOFF[6] = 5227; STAGE_NH[6] = 1;
  STAGE_DOFF[7] = 5671; STAGE_NH[7] = 1;
  STAGE_DOFF[8] = 5677; STAGE_NH[8] = 1;
  STAGE_DOFF[9] = 5693; STAGE_NH[9] = 3;
  STAGE_DOFF[10] = 5741; STAGE_NH[10] = 1;
  STAGE_DOFF[11] = 5750; STAGE_NH[11] = 1;
  STAGE_DOFF[12] = 7382; STAGE_NH[12] = 3;
  STAGE_DOFF[13] = 7589; STAGE_NH[13] = 1;
  STAGE_DOFF[14] = 7608; STAGE_NH[14] = 1;
  STAGE_DOFF[15] = 7640; STAGE_NH[15] = 2;
  STAGE_DOFF[16] = 7936; STAGE_NH[16] = 13;
  STAGE_DOFF[17] = 13708; STAGE_NH[17] = 15;
  STAGE_M[0] = 2; STAGE_H[0] = 6; STAGE_OFF[0] = 0;  STAGE_MW[0] = 1;
  STAGE_NCLS[0] = 4;
  STAGE_VN[0] = 9;
  STAGE_VN[1] = 2;
  STAGE_M[1] = 2; STAGE_H[1] = 11; STAGE_OFF[1] = 142;  STAGE_MW[1] = 1;
  STAGE_NCLS[16] = 10;
  STAGE_VN[4] = 12;
  STAGE_VN[5] = 12;
  STAGE_M[2] = 2; STAGE_H[2] = 34; STAGE_OFF[2] = 399;  STAGE_MW[2] = 2;
  STAGE_NCLS[32] = 36;
  STAGE_VN[8] = 5;
  STAGE_VN[9] = 68;
  STAGE_M[3] = 3; STAGE_H[3] = 62; STAGE_OFF[3] = 2751;  STAGE_MW[3] = 1;
  STAGE_NCLS[48] = 16;
  STAGE_VN[12] = 15;
  STAGE_VN[13] = 19;
  STAGE_VN[14] = 15;
  STAGE_M[4] = 2; STAGE_H[4] = 9; STAGE_OFF[4] = 4677;  STAGE_MW[4] = 1;
  STAGE_NCLS[64] = 7;
  STAGE_VN[16] = 6;
  STAGE_VN[17] = 5;
  STAGE_M[5] = 2; STAGE_H[5] = 75; STAGE_OFF[5] = 4888;  STAGE_MW[5] = 2;
  STAGE_NCLS[80] = 70;
  STAGE_VN[20] = 6;
  STAGE_VN[21] = 70;
  STAGE_M[6] = 3; STAGE_H[6] = 5; STAGE_OFF[6] = 7817;  STAGE_MW[6] = 2;
  STAGE_NCLS[96] = 5;
  STAGE_VN[24] = 74;
  STAGE_VN[25] = 3;
  STAGE_VN[26] = 2;
  STAGE_M[7] = 2; STAGE_H[7] = 3; STAGE_OFF[7] = 8096;  STAGE_MW[7] = 1;
  STAGE_NCLS[112] = 3;
  STAGE_VN[28] = 3;
  STAGE_VN[29] = 2;
  STAGE_M[8] = 2; STAGE_H[8] = 10; STAGE_OFF[8] = 8169;  STAGE_MW[8] = 1;
  STAGE_NCLS[128] = 16;
  STAGE_VN[32] = 8;
  STAGE_VN[33] = 2;
  STAGE_M[9] = 1; STAGE_H[9] = 6; STAGE_OFF[9] = 8557;  STAGE_MW[9] = 1;
  STAGE_NCLS[144] = 4;
  STAGE_NCLS[145] = 2;
  STAGE_NCLS[146] = 2;
  STAGE_VN[36] = 16;
  STAGE_M[10] = 1; STAGE_H[10] = 7; STAGE_OFF[10] = 8665;  STAGE_MW[10] = 1;
  STAGE_NCLS[160] = 7;
  STAGE_VN[40] = 9;
  STAGE_M[11] = 3; STAGE_H[11] = 18; STAGE_OFF[11] = 8774;  STAGE_MW[11] = 1;
  STAGE_NCLS[176] = 10;
  STAGE_VN[44] = 8;
  STAGE_VN[45] = 17;
  STAGE_VN[46] = 12;
  STAGE_M[12] = 1; STAGE_H[12] = 16; STAGE_OFF[12] = 9336;  STAGE_MW[12] = 2;
  STAGE_NCLS[192] = 2;
  STAGE_NCLS[193] = 8;
  STAGE_NCLS[194] = 16;
  STAGE_VN[48] = 69;
  STAGE_M[13] = 1; STAGE_H[13] = 11; STAGE_OFF[13] = 9778;  STAGE_MW[13] = 1;
  STAGE_NCLS[208] = 11;
  STAGE_VN[52] = 19;
  STAGE_M[14] = 2; STAGE_H[14] = 18; STAGE_OFF[14] = 9947;  STAGE_MW[14] = 1;
  STAGE_NCLS[224] = 23;
  STAGE_VN[56] = 16;
  STAGE_VN[57] = 2;
  STAGE_M[15] = 2; STAGE_H[15] = 74; STAGE_OFF[15] = 10624;  STAGE_MW[15] = 2;
  STAGE_NCLS[240] = 5;
  STAGE_NCLS[241] = 95;
  STAGE_VN[60] = 74;
  STAGE_VN[61] = 2;
  STAGE_M[16] = 3; STAGE_H[16] = 52; STAGE_OFF[16] = 14662;  STAGE_MW[16] = 2;
  STAGE_NCLS[256] = 64;
  STAGE_NCLS[257] = 19;
  STAGE_NCLS[258] = 19;
  STAGE_NCLS[259] = 19;
  STAGE_NCLS[260] = 19;
  STAGE_NCLS[261] = 19;
  STAGE_NCLS[262] = 19;
  STAGE_NCLS[263] = 19;
  STAGE_NCLS[264] = 5;
  STAGE_NCLS[265] = 4;
  STAGE_NCLS[266] = 4;
  STAGE_NCLS[267] = 5;
  STAGE_NCLS[268] = 12;
  STAGE_VN[64] = 74;
  STAGE_VN[65] = 3;
  STAGE_VN[66] = 2;
  STAGE_M[17] = 3; STAGE_H[17] = 100; STAGE_OFF[17] = 18597;  STAGE_MW[17] = 2;
  STAGE_NCLS[272] = 5;
  STAGE_NCLS[273] = 95;
  STAGE_NCLS[274] = 64;
  STAGE_NCLS[275] = 19;
  STAGE_NCLS[276] = 19;
  STAGE_NCLS[277] = 19;
  STAGE_NCLS[278] = 19;
  STAGE_NCLS[279] = 19;
  STAGE_NCLS[280] = 19;
  STAGE_NCLS[281] = 19;
  STAGE_NCLS[282] = 5;
  STAGE_NCLS[283] = 4;
  STAGE_NCLS[284] = 4;
  STAGE_NCLS[285] = 5;
  STAGE_NCLS[286] = 12;
  STAGE_VN[68] = 74;
  STAGE_VN[69] = 3;
  STAGE_VN[70] = 2;
  return 18;
}

/* TOKV: unisa/gold.py TOKS */
char *TOKV = "eof\0type\0id\0num\0str\0if\0else\0while\0for\0do\0switch\0case\0default\0return\0break\0continue\0sizeof\0struct\0typedef\0enum\0{\0}\0(\0)\0[\0]\0;\0,\0=\0+=\0-=\0*=\0/=\0?\0:\0+\0-\0*\0/\0%\0==\0!=\0<\0>\0<=\0>=\0&&\0||\0!\0&\0++\0--\0.\0->\0goto\0|\0^\0<<\0>>\0union\0~\0%=\0&=\0|=\0^=\0<<=\0>>=\0...\0";
#define NTOKV 68

/* PRODV: unisa/gold.py PRODS */
char *PRODV = "end\0fn\0global\0typedef\0struct\0enum\0decl\0if\0while\0for\0do\0switch\0case\0default\0return\0break\0continue\0block\0expr\0neg\0not\0deref\0addr\0sizeof\0prim\0index\0call\0inc\0field\0done\0fn_sig\0var_def\0goto\0bnot\0preinc\0uplus\0";
#define NPRODV 36

/* ACTV: unisa/gold.py LEXACT */
char *ACTV = "skip\0nl\0ident\0num\0str\0charlit\0cmt\0linecmt\0op\0bad\0";
#define NACTV 10

/* TYPEV: unisa/front/lex.py TYPEKW */
char *TYPEV = "int\0char\0long\0short\0void\0unsigned\0signed\0float\0double\0const\0static\0_Bool\0extern\0volatile\0register\0auto\0inline\0restrict\0";
#define NTYPEV 18

/* DIRV: unisa/gold.py DIRS */
char *DIRV = "ifdef\0ifndef\0if\0elif\0else\0endif\0define\0include\0undef\0";
#define NDIRV 9

/* PPACTV: unisa/gold.py PPACT */
char *PPACTV = "take\0skip\0pop\0macro\0";
#define NPPACTV 4

/* NTV: unisa/gold.py NT */
char *NTV = "top\0stmt\0unary\0postfix\0after_name\0";
#define NNTV 5

/* TYSV: unisa/gold.py TYS */
char *TYSV = "void\0i8\0i16\0i32\0i64\0u8\0u16\0u32\0u64\0ptr\0arr\0struct\0fn\0f32\0f64\0";
#define NTYSV 15

/* TOPSV: unisa/gold.py TOPS */
char *TOPSV = "+\0-\0*\0/\0%\0<\0==\0=\0&\0[]\0.\0call\0sizeof\0,\0un*\0|\0^\0<<\0>>\0";
#define NTOPSV 19

/* TYOUTV: unisa/gold.py TYOUT */
char *TYOUTV = "void\0i8\0i16\0i32\0i64\0u8\0u16\0u32\0u64\0ptr\0arr\0struct\0fn\0f32\0f64\0illegal\0";
#define NTYOUTV 16

/* SCTXV: unisa/gold.py CTX */
char *SCTXV = "top\0param\0local\0expr\0sizeof\0field\0";
#define NSCTXV 6

/* SKINDV: unisa/gold.py KIND */
char *SKINDV = "type_kw\0id\0typedef_id\0star\0lparen\0";
#define NSKINDV 5

/* SACTV: unisa/gold.py ACTS */
char *SACTV = "bind_global\0bind_param\0bind_local\0lookup\0type_name\0fn_name\0field\0";
#define NSACTV 7

/* IRFAMV: unisa/gold.py FAMILY */
char *IRFAMV = "alu\0mem\0ctrl\0call\0lit\0fpu\0";
#define NIRFAMV 6

/* IRFLAV: unisa/gold.py FLAVOR */
char *IRFLAV = "add\0sub\0mul\0lt\0le\0gt\0ge\0eq\0ne\0neg\0and\0or\0xor\0shl\0shr\0ult\0ule\0ugt\0uge\0lshr\0div\0mod\0udiv\0umod\0load\0store\0lea\0ld\0st\0zero\0jump\0jumpz\0ret\0call\0push\0arg\0frame\0callr\0imm\0print\0write\0exit\0dadd\0dsub\0dmul\0ddiv\0dlt\0dle\0deq\0sadd\0ssub\0smul\0sdiv\0slt\0sle\0seq\0i2d\0u2d\0i2s\0u2s\0d2i\0d2u\0s2d\0d2s\0dsqrt\0ssqrt\0dgt\0dge\0sgt\0sge\0";
#define NIRFLAV 70

/* IRRECV: unisa/gold.py RECIPE */
char *IRRECV = "add64\0and64\0arg\0call\0callpush\0callr\0cvtdi\0cvtds\0cvtdu\0cvtid\0cvtis\0cvtsd\0cvtud\0cvtus\0div\0eq\0exit\0fadd32\0fadd64\0fdiv32\0fdiv64\0feq32\0feq64\0fle32\0fle32_rev\0fle64\0fle64_rev\0flt32\0flt32_rev\0flt64\0flt64_rev\0fmul32\0fmul64\0frame\0fsqrt32\0fsqrt64\0fsub32\0fsub64\0imm\0jump\0jumpz\0ld\0lea\0load64\0lshr64\0mod\0mul64\0ne\0or64\0print\0ret\0shl64\0shr64\0sle64\0sle64_rev\0slt64\0slt64_rev\0st\0store64\0sub64\0udiv\0ule64\0ule64_rev\0ult64\0ult64_rev\0umod\0write\0xor64\0zero\0bad\0";
#define NIRRECV 70

/* BF_ENC_0: unisa/gold.py STAGES["enc"].fields[0] (op) */
char *BF_ENC_0 = "exit\000read\000write\000open\000close\000getdents64\000mmap\000munmap\000mprotect\000getpid\000clock_gettime\000nanosleep\000futex\000socket\000connect\000bind\000listen\000accept\000clone\000execve\000lseek\000unlink\000rename\000add64\000sub64\000xor64\000mul64\000slt64\000sle64\000load64\000store64\000jump\000jumpz\000call\000ret\000nop\000cas64\000fence\000syscall_gate\000tls_base\000cycle_counter\000stack_enter\000and64\000or64\000shl64\000shr64\000callr\000ult64\000ule64\000lshr64\000fadd64\000fsub64\000fmul64\000fdiv64\000flt64\000fle64\000feq64\000fadd32\000fsub32\000fmul32\000fdiv32\000flt32\000fle32\000feq32\000cvtid\000cvtud\000cvtis\000cvtus\000cvtdi\000cvtdu\000cvtsd\000cvtds\000fsqrt64\000fsqrt32\000";
#define NBF_ENC_0 74

/* BF_ENC_1: unisa/gold.py STAGES["enc"].fields[1] (os) */
char *BF_ENC_1 = "lnx\000osx\000win\000";
#define NBF_ENC_1 3

/* BF_ENC_2: unisa/gold.py STAGES["enc"].fields[2] (arch) */
char *BF_ENC_2 = "x86_64\000arm64\000";
#define NBF_ENC_2 2

/* BH_ENC_Y: the classes of head "y" of stage "enc", in the constructed net's order (weights/built.json) */
char *BH_ENC_Y = "syscall\000svc\000winapi\000x86\000arm\000";
#define NBH_ENC_Y 5
#define HD_ENC_Y 0

/* BF_RELOC_0: unisa/gold.py STAGES["reloc"].fields[0] (kind) */
char *BF_RELOC_0 = "jmp\000jz\000call\000";
#define NBF_RELOC_0 3

/* BF_RELOC_1: unisa/gold.py STAGES["reloc"].fields[1] (arch) */
char *BF_RELOC_1 = "x86_64\000arm64\000";
#define NBF_RELOC_1 2

/* BH_RELOC_Y: the classes of head "y" of stage "reloc", in the constructed net's order (weights/built.json) */
char *BH_RELOC_Y = "rel32\000arm26\000arm19\000";
#define NBH_RELOC_Y 3
#define HD_RELOC_Y 0

/* BF_REGMAP_0: unisa/gold.py STAGES["regmap"].fields[0] (treg) */
char *BF_REGMAP_0 = "r0\000r1\000r2\000r3\000r4\000r5\000r6\000r7\000";
#define NBF_REGMAP_0 8

/* BF_REGMAP_1: unisa/gold.py STAGES["regmap"].fields[1] (arch) */
char *BF_REGMAP_1 = "x86_64\000arm64\000";
#define NBF_REGMAP_1 2

/* BH_REGMAP_Y: the classes of head "y" of stage "regmap", in the constructed net's order (weights/built.json) */
char *BH_REGMAP_Y = "r8\000r9\000rax\000rcx\000rdi\000rdx\000rsi\000rsp\000x0\000x1\000x2\000x3\000x4\000x5\000x6\000x7\000";
#define NBH_REGMAP_Y 16
#define HD_REGMAP_Y 0

/* BF_TYINFO_0: unisa/gold.py STAGES["tyinfo"].fields[0] (t) */
char *BF_TYINFO_0 = "void\000i8\000i16\000i32\000i64\000u8\000u16\000u32\000u64\000ptr\000arr\000struct\000fn\000f32\000f64\000illegal\000";
#define NBF_TYINFO_0 16

/* BH_TYINFO_SIZE: the classes of head "size" of stage "tyinfo", in the constructed net's order (weights/built.json) */
char *BH_TYINFO_SIZE = "1\0002\0004\0008\000";
#define NBH_TYINFO_SIZE 4
#define HD_TYINFO_SIZE 0

/* BH_TYINFO_UNS: the classes of head "uns" of stage "tyinfo", in the constructed net's order (weights/built.json) */
char *BH_TYINFO_UNS = "0\0001\000";
#define NBH_TYINFO_UNS 2
#define HD_TYINFO_UNS 1

/* BH_TYINFO_NARROW: the classes of head "narrow" of stage "tyinfo", in the constructed net's order (weights/built.json) */
char *BH_TYINFO_NARROW = "0\0001\000";
#define NBH_TYINFO_NARROW 2
#define HD_TYINFO_NARROW 2

/* BF_PFCONV_0: unisa/gold.py STAGES["pfconv"].fields[0] (conv) */
char *BF_PFCONV_0 = "d\000i\000u\000x\000X\000o\000p\000c\000s\000";
#define NBF_PFCONV_0 9

/* BH_PFCONV_Y: the classes of head "y" of stage "pfconv", in the constructed net's order (weights/built.json) */
char *BH_PFCONV_Y = "int\000u32\000hex\000HEX\000oct\000chr\000str\000";
#define NBH_PFCONV_Y 7
#define HD_PFCONV_Y 0

/* BF_PEEP_0: unisa/gold.py STAGES["peep"].fields[0] (a) */
char *BF_PEEP_0 = "store64\000load64\000imm\000mov\000jump\000jumpz\000alu\000other\000";
#define NBF_PEEP_0 8

/* BF_PEEP_1: unisa/gold.py STAGES["peep"].fields[1] (b) */
char *BF_PEEP_1 = "store64\000load64\000imm\000mov\000jump\000jumpz\000add64\000sub64\000mul64\000shl64\000shr64\000lshr64\000or64\000xor64\000and64\000other\000none\000";
#define NBF_PEEP_1 17

/* BF_PEEP_2: unisa/gold.py STAGES["peep"].fields[2] (rel) */
char *BF_PEEP_2 = "none\000same_slot\000same_slot_same_reg\000const0\000const1\000pow2\000copy_dead\000to_next\000to_jump\000a_dead\000dest_to_mov\000copy_into\000";
#define NBF_PEEP_2 12

/* BH_PEEP_Y: the classes of head "y" of stage "peep", in the constructed net's order (weights/built.json) */
char *BH_PEEP_Y = "keep\000load_to_mov\000drop_b\000drop_a\000retarget\000to_mov\000to_shl\000fold_imm\000retarget_dest\000fold_copy\000";
#define NBH_PEEP_Y 10
#define HD_PEEP_Y 0

/* BF_OPINFO_0: unisa/gold.py STAGES["opinfo"].fields[0] (op) */
char *BF_OPINFO_0 = "imm\000mov\000add64\000sub64\000mul64\000xor64\000and64\000or64\000shl64\000shr64\000lshr64\000.div\000.mod\000.udiv\000.umod\000slt64\000sle64\000ult64\000ule64\000eq\000ne\000load64\000store64\000.ld\000.st\000.lea\000.zero\000jump\000jumpz\000call\000callr\000.hostcall\000.hostaddr\000ret\000.frame\000.arg\000.print\000.write\000.exit\000.sys\000.sys6\000.argc\000.argv\000nop\000fadd64\000fsub64\000fmul64\000fdiv64\000flt64\000fle64\000feq64\000fadd32\000fsub32\000fmul32\000fdiv32\000flt32\000fle32\000feq32\000cvtid\000cvtud\000cvtis\000cvtus\000cvtdi\000cvtdu\000cvtsd\000cvtds\000fsqrt64\000fsqrt32\000other\000";
#define NBF_OPINFO_0 69

/* BH_OPINFO_SIMPLE: the classes of head "simple" of stage "opinfo", in the constructed net's order (weights/built.json) */
char *BH_OPINFO_SIMPLE = "0\0001\000";
#define NBH_OPINFO_SIMPLE 2
#define HD_OPINFO_SIMPLE 0

/* BH_OPINFO_ACLS: the classes of head "acls" of stage "opinfo", in the constructed net's order (weights/built.json) */
char *BH_OPINFO_ACLS = "store64\000load64\000imm\000mov\000jump\000jumpz\000alu\000other\000";
#define NBH_OPINFO_ACLS 8
#define HD_OPINFO_ACLS 1

/* BH_OPINFO_BCLS: the classes of head "bcls" of stage "opinfo", in the constructed net's order (weights/built.json) */
char *BH_OPINFO_BCLS = "store64\000load64\000imm\000mov\000jump\000jumpz\000add64\000sub64\000mul64\000shl64\000shr64\000lshr64\000or64\000xor64\000and64\000other\000";
#define NBH_OPINFO_BCLS 16
#define HD_OPINFO_BCLS 2

/* BF_PREC_0: unisa/gold.py STAGES["prec"].fields[0] (op) */
char *BF_PREC_0 = "||\000&&\000|\000^\000&\000==\000!=\000<\000>\000<=\000>=\000<<\000>>\000+\000-\000*\000/\000%\000other\000";
#define NBF_PREC_0 19

/* BH_PREC_Y: the classes of head "y" of stage "prec", in the constructed net's order (weights/built.json) */
char *BH_PREC_Y = "none\0001\0002\0003\0004\0005\0006\0007\0008\0009\00010\000";
#define NBH_PREC_Y 11
#define HD_PREC_Y 0

/* BF_BINSEL_0: unisa/gold.py STAGES["binsel"].fields[0] (op) */
char *BF_BINSEL_0 = "+\000-\000*\000/\000%\000<\000>\000<=\000>=\000==\000!=\000&\000|\000^\000<<\000>>\000";
#define NBF_BINSEL_0 16

/* BF_BINSEL_1: unisa/gold.py STAGES["binsel"].fields[1] (sign) */
char *BF_BINSEL_1 = "s\000u\000";
#define NBF_BINSEL_1 2

/* BH_BINSEL_Y: the classes of head "y" of stage "binsel", in the constructed net's order (weights/built.json) */
char *BH_BINSEL_Y = "add\000sub\000mul\000div\000mod\000lt\000gt\000le\000ge\000eq\000ne\000and\000or\000xor\000shl\000shr\000udiv\000umod\000ult\000ugt\000ule\000uge\000lshr\000";
#define NBH_BINSEL_Y 23
#define HD_BINSEL_Y 0

/* BF_ISEL_0: unisa/gold.py STAGES["isel"].fields[0] (op) */
char *BF_ISEL_0 = "exit\000read\000write\000open\000close\000getdents64\000mmap\000munmap\000mprotect\000getpid\000clock_gettime\000nanosleep\000futex\000socket\000connect\000bind\000listen\000accept\000clone\000execve\000lseek\000unlink\000rename\000add64\000sub64\000xor64\000mul64\000slt64\000sle64\000load64\000store64\000jump\000jumpz\000call\000ret\000nop\000cas64\000fence\000syscall_gate\000tls_base\000cycle_counter\000stack_enter\000and64\000or64\000shl64\000shr64\000callr\000ult64\000ule64\000lshr64\000fadd64\000fsub64\000fmul64\000fdiv64\000flt64\000fle64\000feq64\000fadd32\000fsub32\000fmul32\000fdiv32\000flt32\000fle32\000feq32\000cvtid\000cvtud\000cvtis\000cvtus\000cvtdi\000cvtdu\000cvtsd\000cvtds\000fsqrt64\000fsqrt32\000";
#define NBF_ISEL_0 74

/* BF_ISEL_1: unisa/gold.py STAGES["isel"].fields[1] (arch) */
char *BF_ISEL_1 = "x86_64\000arm64\000";
#define NBF_ISEL_1 2

/* BH_ISEL_FORM: the classes of head "form" of stage "isel", in the constructed net's order (weights/built.json) */
char *BH_ISEL_FORM = "syscall\000svc\000winapi\000x86\000arm\000";
#define NBH_ISEL_FORM 5
#define HD_ISEL_FORM 0

/* BH_ISEL_SYMBOL: the classes of head "symbol" of stage "isel", in the constructed net's order (weights/built.json) */
char *BH_ISEL_SYMBOL = "accept\000add\000addsd\000addss\000and\000asr\000b\000bind\000bl\000blr\000call\000casal\000cbz\000clock_gettime\000clone\000close\000cmpxchg\000connect\000cset\000cvtsd2ss\000cvtsi2sd\000cvtsi2ss\000cvtss2sd\000cvttsd2si\000divsd\000divss\000dmb\000eor\000execve\000exit\000fadd\000fcmp\000fcvt\000fcvtzs\000fcvtzu\000fdiv\000fmul\000fsqrt\000fsub\000futex\000getdents64\000getpid\000imul\000jmp\000jz\000ldr\000listen\000lseek\000lsl\000lsr\000mfence\000mmap\000mov\000mprotect\000mrs\000mul\000mulsd\000mulss\000munmap\000nanosleep\000none\000nop\000open\000or\000orr\000push\000rdfsbase\000rdtsc\000read\000rename\000ret\000sar\000scvtf\000setb\000setbe\000setl\000setle\000shl\000shr\000socket\000sqrtsd\000sqrtss\000stp\000str\000sub\000subsd\000subss\000svc\000syscall\000ucomisd\000ucomiss\000ucvtf\000unlink\000write\000xor\000";
#define NBH_ISEL_SYMBOL 95
#define HD_ISEL_SYMBOL 1

/* BF_ABI_0: unisa/gold.py STAGES["abi"].fields[0] (op) */
char *BF_ABI_0 = "exit\000read\000write\000open\000close\000getdents64\000mmap\000munmap\000mprotect\000getpid\000clock_gettime\000nanosleep\000futex\000socket\000connect\000bind\000listen\000accept\000clone\000execve\000lseek\000unlink\000rename\000add64\000sub64\000xor64\000mul64\000slt64\000sle64\000load64\000store64\000jump\000jumpz\000call\000ret\000nop\000cas64\000fence\000syscall_gate\000tls_base\000cycle_counter\000stack_enter\000and64\000or64\000shl64\000shr64\000callr\000ult64\000ule64\000lshr64\000fadd64\000fsub64\000fmul64\000fdiv64\000flt64\000fle64\000feq64\000fadd32\000fsub32\000fmul32\000fdiv32\000flt32\000fle32\000feq32\000cvtid\000cvtud\000cvtis\000cvtus\000cvtdi\000cvtdu\000cvtsd\000cvtds\000fsqrt64\000fsqrt32\000";
#define NBF_ABI_0 74

/* BF_ABI_1: unisa/gold.py STAGES["abi"].fields[1] (os) */
char *BF_ABI_1 = "lnx\000osx\000win\000";
#define NBF_ABI_1 3

/* BF_ABI_2: unisa/gold.py STAGES["abi"].fields[2] (arch) */
char *BF_ABI_2 = "x86_64\000arm64\000";
#define NBF_ABI_2 2

/* BH_ABI_SYSNO: the classes of head "sysno" of stage "abi", in the constructed net's order (weights/built.json) */
char *BH_ABI_SYSNO = "0\0000x2000001\0000x2000003\0000x2000004\0000x2000005\0000x2000006\0000x200000a\0000x2000014\0000x200001e\0000x200003b\0000x2000049\0000x200004a\0000x2000061\0000x2000062\0000x2000068\0000x200006a\0000x2000080\0000x20000c5\0000x20000c7\0000x2000168\0000x2000203\0001\00010\000101\00011\000113\000172\000198\0002\000200\000201\000202\000203\000215\000217\000220\000221\000222\000226\000228\000276\0003\00035\00039\00041\00042\00043\00049\00050\00056\00057\00059\00060\00061\00062\00063\00064\0008\00082\00087\0009\00093\00098\000none\000";
#define NBH_ABI_SYSNO 64
#define HD_ABI_SYSNO 0

/* BH_ABI_ARG0: the classes of head "arg0" of stage "abi", in the constructed net's order (weights/built.json) */
char *BH_ABI_ARG0 = "rdi\000rsi\000rdx\000r10\000rcx\000r8\000r9\000rax\000rsp\000x0\000x1\000x2\000x3\000x4\000x5\000x6\000x7\000x8\000none\000";
#define NBH_ABI_ARG0 19
#define HD_ABI_ARG0 1

/* BH_ABI_ARG1: the classes of head "arg1" of stage "abi", in the constructed net's order (weights/built.json) */
char *BH_ABI_ARG1 = "rdi\000rsi\000rdx\000r10\000rcx\000r8\000r9\000rax\000rsp\000x0\000x1\000x2\000x3\000x4\000x5\000x6\000x7\000x8\000none\000";
#define NBH_ABI_ARG1 19
#define HD_ABI_ARG1 2

/* BH_ABI_ARG2: the classes of head "arg2" of stage "abi", in the constructed net's order (weights/built.json) */
char *BH_ABI_ARG2 = "rdi\000rsi\000rdx\000r10\000rcx\000r8\000r9\000rax\000rsp\000x0\000x1\000x2\000x3\000x4\000x5\000x6\000x7\000x8\000none\000";
#define NBH_ABI_ARG2 19
#define HD_ABI_ARG2 3

/* BH_ABI_ARG3: the classes of head "arg3" of stage "abi", in the constructed net's order (weights/built.json) */
char *BH_ABI_ARG3 = "rdi\000rsi\000rdx\000r10\000rcx\000r8\000r9\000rax\000rsp\000x0\000x1\000x2\000x3\000x4\000x5\000x6\000x7\000x8\000none\000";
#define NBH_ABI_ARG3 19
#define HD_ABI_ARG3 4

/* BH_ABI_ARG4: the classes of head "arg4" of stage "abi", in the constructed net's order (weights/built.json) */
char *BH_ABI_ARG4 = "rdi\000rsi\000rdx\000r10\000rcx\000r8\000r9\000rax\000rsp\000x0\000x1\000x2\000x3\000x4\000x5\000x6\000x7\000x8\000none\000";
#define NBH_ABI_ARG4 19
#define HD_ABI_ARG4 5

/* BH_ABI_ARG5: the classes of head "arg5" of stage "abi", in the constructed net's order (weights/built.json) */
char *BH_ABI_ARG5 = "rdi\000rsi\000rdx\000r10\000rcx\000r8\000r9\000rax\000rsp\000x0\000x1\000x2\000x3\000x4\000x5\000x6\000x7\000x8\000none\000";
#define NBH_ABI_ARG5 19
#define HD_ABI_ARG5 6

/* BH_ABI_RET: the classes of head "ret" of stage "abi", in the constructed net's order (weights/built.json) */
char *BH_ABI_RET = "rdi\000rsi\000rdx\000r10\000rcx\000r8\000r9\000rax\000rsp\000x0\000x1\000x2\000x3\000x4\000x5\000x6\000x7\000x8\000none\000";
#define NBH_ABI_RET 19
#define HD_ABI_RET 7

/* BH_ABI_GATE: the classes of head "gate" of stage "abi", in the constructed net's order (weights/built.json) */
char *BH_ABI_GATE = "syscall\000svc0\000svc80\000winapi\000none\000";
#define NBH_ABI_GATE 5
#define HD_ABI_GATE 8

/* BH_ABI_NRREG: the classes of head "nrreg" of stage "abi", in the constructed net's order (weights/built.json) */
char *BH_ABI_NRREG = "none\000rax\000x16\000x8\000";
#define NBH_ABI_NRREG 4
#define HD_ABI_NRREG 9

/* BH_ABI_ARGSHAPE: the classes of head "argshape" of stage "abi", in the constructed net's order (weights/built.json) */
char *BH_ABI_ARGSHAPE = "plain\000atfd_1\000atfd_1_zero\000atfd_2_zero5\000";
#define NBH_ABI_ARGSHAPE 4
#define HD_ABI_ARGSHAPE 10

/* BH_ABI_RETCONV: the classes of head "retconv" of stage "abi", in the constructed net's order (weights/built.json) */
char *BH_ABI_RETCONV = "none\000wcount\000bool_inv\000bool_neg\000dword_sx\000";
#define NBH_ABI_RETCONV 5
#define HD_ABI_RETCONV 11

/* BH_ABI_WINIMP: the classes of head "winimp" of stage "abi", in the constructed net's order (weights/built.json) */
char *BH_ABI_WINIMP = "none\000ExitProcess\000WriteFile\000ReadFile\000VirtualAlloc\000VirtualProtect\000VirtualFree\000CloseHandle\000CreateFileA\000SetFilePointer\000DeleteFileA\000MoveFileExA\000";
#define NBH_ABI_WINIMP 12
#define HD_ABI_WINIMP 12

/* ENC_X86_ALU2: unisa/catalog.py ENCSPEC["x86_64"]["alu2"] -- opcode data, not a decision */
char *ENC_X86_ALU2 = "add64\000sub64\000xor64\000and64\000or64\000";
#define NENC_X86_ALU2 5
long ENC_X86_ALU2_V(int i) {
  if (i == 0) return 1;
  if (i == 1) return 41;
  if (i == 2) return 49;
  if (i == 3) return 33;
  if (i == 4) return 9;
  return 0 - 1;
}

/* ENC_X86_SETCC: unisa/catalog.py ENCSPEC["x86_64"]["setcc"] -- opcode data, not a decision */
char *ENC_X86_SETCC = "slt64\000sle64\000eq\000ne\000ult64\000ule64\000";
#define NENC_X86_SETCC 6
long ENC_X86_SETCC_V(int i) {
  if (i == 0) return 156;
  if (i == 1) return 158;
  if (i == 2) return 148;
  if (i == 3) return 149;
  if (i == 4) return 146;
  if (i == 5) return 150;
  return 0 - 1;
}

/* ENC_X86_SHIFTEXT: unisa/catalog.py ENCSPEC["x86_64"]["shiftext"] -- opcode data, not a decision */
char *ENC_X86_SHIFTEXT = "shl64\000shr64\000lshr64\000";
#define NENC_X86_SHIFTEXT 3
long ENC_X86_SHIFTEXT_V(int i) {
  if (i == 0) return 4;
  if (i == 1) return 7;
  if (i == 2) return 5;
  return 0 - 1;
}

/* ENC_ARM_ALU3: unisa/catalog.py ENCSPEC["arm64"]["alu3"] -- opcode data, not a decision */
char *ENC_ARM_ALU3 = "add64\000sub64\000xor64\000and64\000or64\000shl64\000shr64\000lshr64\000";
#define NENC_ARM_ALU3 8
long ENC_ARM_ALU3_V(int i) {
  if (i == 0) return 2332033024;
  if (i == 1) return 3405774848;
  if (i == 2) return 3388997632;
  if (i == 3) return 2315255808;
  if (i == 4) return 2852126720;
  if (i == 5) return 2596282368;
  if (i == 6) return 2596284416;
  if (i == 7) return 2596283392;
  return 0 - 1;
}

/* ENC_ARM_INVCOND: unisa/catalog.py ENCSPEC["arm64"]["invcond"] -- opcode data, not a decision */
char *ENC_ARM_INVCOND = "slt64\000sle64\000eq\000ne\000ult64\000ule64\000";
#define NENC_ARM_INVCOND 6
long ENC_ARM_INVCOND_V(int i) {
  if (i == 0) return 10;
  if (i == 1) return 12;
  if (i == 2) return 1;
  if (i == 3) return 0;
  if (i == 4) return 2;
  if (i == 5) return 8;
  return 0 - 1;
}

#endif /* UNISA_MODEL_INC */
