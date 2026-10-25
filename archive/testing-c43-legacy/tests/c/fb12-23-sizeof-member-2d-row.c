/* R13-0b: external trial of 0.0.12, real-world batch.  Source: ~/unisacc-feedback-0.0.12-realworld/23-sizeof-member-2d-row.c (+ .out.txt for the gcc-vs-unisacc record). */
/* sizeof applied to one row of a 2-D array that is a struct member,
   sizeof(d->sz[t]) or sizeof(c.sz[2]), yields 8 (pointer size) instead of
   the row size (288).  A local 2-D array row is correct (10).  Silent
   wrong code: miniz's MZ_CLEAR_ARR(d->m_huff_code_sizes[table_num]) =
   memset(x, 0, sizeof(x)) clears only 8 bytes, so every dynamic-Huffman
   deflate block is corrupt (output differs from gcc; gcc's inflate
   rejects it) - found by compressing sqlite3.c with miniz example3. */
#include <stdio.h>
#include <string.h>
struct C { unsigned short cnt[3][288]; unsigned char sz[3][288]; };
static struct C c;
int main(void) {
    struct C *d = &c;
    int t = 1;
    unsigned char local[4][10];
    printf("%d %d %d %d\n", (int)sizeof(d->sz[t]), (int)sizeof(d->cnt[0]),
           (int)sizeof(c.sz[2]), (int)sizeof(local[1]));
    memset(d->sz[0], 7, sizeof(d->sz[0]));
    printf("%d\n", d->sz[0][287]);
    return 0;
}
