/* 64-bit pointer / 32-bit int CoreModel ABI. layoutcheck.c asserts every
   field used by assembly; these are host structure offsets, not model data. */
#define CM_NS 0
#define CM_NQ 4
#define CM_ISNET 16
#define CM_MODE 64
#define CM_COUNT 72
#define CM_KEYS 80
#define CM_NEXT 88
#define CM_SEQ 96
#define CM_LO 104
#define CM_HI 112
#define CM_BASE_NEXT 120
#define CM_BASE_SEQ 128
#define CM_SIZE 136

#define BUF_BYTES 0
#define BUF_ATTR 8
#define BUF_LENGTH 16
#define BUF_CAPACITY 20
#define BUF_SIZE 24

#define MEM_KEYS 0
#define MEM_VALUES 8
#define MEM_USED 16
#define MEM_CAP 24
#define MEM_COUNT 32
#define MEM_SIZE 40

#define INTERN_ENTRIES 0
#define INTERN_CAP 8
#define INTERN_COUNT 16
#define INTERN_SIZE 24
#define ENTRY_BYTES 0
#define ENTRY_LENGTH 8
#define ENTRY_VALUE 16
#define ENTRY_SIZE 24

#define BLOBS_ENTRIES 0
#define BLOBS_COUNT 8
#define BLOBS_CAP 12
#define BLOBS_SIZE 16
#define BLOB_BYTES 0
#define BLOB_LENGTH 8
#define BLOB_SIZE 16
#define RES_ENTRIES 0
#define RES_COUNT 8
#define RES_SIZE 16
#define RES_KEY 0
#define RES_LENGTH 8
#define RES_ID 12
#define RES_ENTRY_SIZE 16

#define STACK_ENTRIES 0
#define STACK_COUNT 8
#define STACK_CAP 12
#define STACK_SIZE 16
#define FRAME_BYTES 0
#define FRAME_ATTR 8
#define FRAME_CURSOR 16
#define FRAME_END 24
#define FRAME_SIZE 32

#define M_MODEL 0
#define M_RESULT 8
#define M_REGS 16
#define M_INPUT 24
#define M_X 32
#define M_XATTR 40
#define M_XN 48
#define M_R 56
#define M_OT 64
#define M_OSEL 72
#define M_STATUS 76
#define M_OUT 80
#define M_ERR 104
#define M_SCRATCH 128
#define M_STACK 152
#define M_FRAMES 168
#define M_MEMORY 184
#define M_BLOBS 224
#define M_STRINGS 240
#define M_RESOURCES 264
#define MACHINE_SIZE 280
#define RESULT_REASON 48
#define RESULT_REASON_N 56
#define CM_STR 24
#define CM_STRL 32

#define CM_NRG 8
#define CM_START 12
#define CM_QOFF 40
#define CM_QLEN 48
#define CM_QA 56
#define RESULT_OUT 0
#define RESULT_ERR 24
#define RESULT_SIZE 64
