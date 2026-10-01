/* Generated from unisa/tape.py SHAPE; do not edit. */
#define TB_OPCOUNT 71
#define TB_RECORD_UNIT 6
#define TB_RECORD_GLOBAL 4
#define TB_RECORD_EXTERN 5
#define TB_RECORD_GDEF 7
char *tb_opnames[TB_OPCOUNT] = {"imm", "mov", "add64", "sub64", "mul64", "xor64", "and64", "or64", "shl64", "shr64", "lshr64", ".div", ".mod", ".udiv", ".umod", "slt64", "sle64", "ult64", "ule64", "eq", "ne", "load64", "store64", ".ld", ".st", ".lea", ".zero", "jump", "jumpz", "call", "callr", "callm", ".hostcall", ".librarycall", ".libraryaddr", ".hostaddr", "ret", ".frame", ".arg", ".print", ".write", ".exit", ".sys", ".sys6", ".argc", ".argv", "nop", "fadd64", "fsub64", "fmul64", "fdiv64", "flt64", "fle64", "feq64", "fadd32", "fsub32", "fmul32", "fdiv32", "flt32", "fle32", "feq32", "cvtid", "cvtud", "cvtis", "cvtus", "cvtdi", "cvtdu", "cvtsd", "cvtds", "fsqrt64", "fsqrt32"};
char *tb_shapes[TB_OPCOUNT] = {"ri", "rr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rri", "rir", "rrii", "riri", "rs", "rii", "L", "rL", "L", "r", "ri", "rr", "rr", "rsiiii", "ri", "", "i", "ir", "r", "rr", "r", "srrr", "srrrrrr", "r", "rr", "", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rrr", "rr", "rr", "rr", "rr", "rr", "rr", "rr", "rr", "rr", "rr"};
