/* R13-0b #23 -- the real consequence: miniz's compress/uncompress round trip.
   `sizeof(s->m_huff_code_sizes[table_num])` inside MZ_CLEAR_ARR is 8 instead of
   the row size, so every dynamic-Huffman deflate block is corrupt.  This
   fixture compresses a buffer whose content forces a dynamic block, then
   decompresses it with the same library and compares against the input; it
   prints a checksum either way, so a wrong answer is visible in the output and
   not only in an exit status.

   miniz's own example3 found this by compressing sqlite3.c.  The payload here
   is generated deterministically instead of shipping a 9 MB source file. */
#include <stdio.h>
#include <string.h>
#include "miniz.h"

static unsigned char src[24000];
static unsigned char comp[32000];
static unsigned char back[24000];

int main(void) {
    mz_ulong clen = sizeof comp, blen = sizeof back;
    int i, rc;
    unsigned long sum = 0;
    /* skewed byte distribution plus long repeats: deflate cannot use a stored
       or fixed-Huffman block for this, so the dynamic path runs. */
    for (i = 0; i < (int)sizeof src; i++) {
        src[i] = (unsigned char)((i % 37) < 30 ? 'a' + (i % 7) : (i * 31) & 255);
    }
    for (i = 0; i < 200; i++) memcpy(src + 1000 + i * 13, "REPEAT-REPEAT-", 14);
    rc = mz_compress(comp, &clen, src, (mz_ulong)sizeof src);
    printf("compress rc=%d\n", rc);
    if (rc != MZ_OK) return 1;
    /* the compressed stream must be self-consistent and decompressible by the
       very same library, and gcc-built inflate must accept it too */
    rc = mz_uncompress(back, &blen, comp, clen);
    printf("uncompress rc=%d\n", rc);
    printf("clen<src=%d\n", clen < (mz_ulong)sizeof src);
    printf("roundtrip=%d\n", (int)blen == (int)sizeof src && memcmp(src, back, sizeof src) == 0);
    for (i = 0; i < (int)sizeof src; i++) sum = sum * 131 + back[i];
    printf("sum=%lu\n", sum & 0xffffffffUL);
    return 0;
}
