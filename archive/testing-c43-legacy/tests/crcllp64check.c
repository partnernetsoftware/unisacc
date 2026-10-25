/* The actual codec and package reader, not a second CRC/parser implementation. */
#define UNISA_RUNTIME_LIBRARY
#include "../exec/c/run.c"
#ifdef CRC_COFF_ABI
_Static_assert(sizeof(long)==4,"Windows LLP64 long");
_Static_assert(sizeof(void*)==8,"Windows pointer");
_Static_assert(sizeof(ctab[0][0])==4,"CRC table cell");
_Static_assert(sizeof(*PCRC)==4,"package CRC cell");
#endif
int main(int argc,char **argv) {
    crc_init();
    if (crc("",0)!=0 || crc("123456789",9)!=0xcbf43926U || crc("abc",3)!=0x352441c2U) return 1;
    /* Compare each aligned/unaligned slice length with an independent bitwise oracle. */
    char b[259]; for(int i=0;i<259;i++)b[i]=(char)(i*37);
    for(int off=0;off<4;off++)for(int n=0;n<256;n++) {
        uint32_t x=4294967295U;
        for(int i=0;i<n;i++){x^=(unsigned char)b[off+i];for(int k=0;k<8;k++)x=(x>>1)^((x&1)?0xedb88320U:0);}
        if(crc(b+off,n)!=(x^4294967295U))return 2;
    }
    if (argc>1) {
        package(argv[1]);
        uint32_t expected=2147483648U;
        if(argc>2)expected=4294967295U;
        if(PCRC[0]!=expected)return 3;
        unpackage();
    }
    puts("CRC32 and header exact"); return 0;
}
