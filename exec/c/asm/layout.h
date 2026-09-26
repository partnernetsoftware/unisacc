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
