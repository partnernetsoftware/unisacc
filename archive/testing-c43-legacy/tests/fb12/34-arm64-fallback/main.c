/* R13-0c #34 -- the arm64 encoder's fallback rejection, reduced to this.
   Four units (this file plus miniz.c, miniz_tdef.c, miniz_tinfl.c) and seven
   headers; 120,703 bytes against the original fixture's 142,070 (-15.0%).

   The reduction was function-level and stopped there: the four units are all
   needed (removing any one turns the error into `undefined function`, because
   the trigger sits in a call chain across them), so what is left is the limit
   of what deleting whole functions can reach.  The trigger itself is inside a
   function body; no statement-level pass was run.

   A candidate is "still reproducing" only when all four of these hold.  The
   first three were not enough on their own -- an earlier reduced set satisfied
   (a)(b)(c) while gcc could no longer LINK it, because (c) as first written
   used -fsyntax-only, which checks grammar and not symbols, so two functions
   the program still referenced had been deleted:

     (a)  <product> -I. -DMINIZ_NO_TIME main.c miniz*.c -o prog
              rc=1 and the message is exactly
              `reject: not covered: ARM64 operand or instruction`
     (b)  the same with -c        -> rc=0   (the front end and the tape are fine)
     (c)  cc -fsyntax-only ...    -> rc=0   (the program is still legal C)
     (d)  cc ... -o prog && ./prog -> rc=0 and the output equals expect.txt
              (we did not just break the program; compare with the reference)

   expect.txt is (d)'s output.  x86_64 fails differently -- that is #30, a
   different defect, and this fixture is about the arm64 fallback.

   Everything below this line is miniz, not this project. */
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
