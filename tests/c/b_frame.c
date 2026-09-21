/* A frame bigger than 256 bytes.  arm64 addresses a local as [fp, #-off],
   and the unscaled form's immediate is a SIGNED 9-bit field: -256..255.
   Past that the encoder was masking the offset, which is a sign flip rather
   than a truncation, so `[fp, #-260]` quietly became `[fp, #+252]` and the
   program read some other local.  No fault, no diagnostic, just a wrong
   answer -- and every probe we had was small enough to miss it.  [I-22] */
#include <stdio.h>

static void fill(char *p, int n)
{
    int i;
    for (i = 0; i < n; i++) p[i] = (char)(i & 63);
}

/* Past 4095 the frame adjustment itself stops fitting in an immediate.  A
   Blowfish key is a 4,168-byte local, which is how we found out. */
static int wide_frame(int seed)
{
    char big[5000];
    int i, sum = 0;
    for (i = 0; i < 5000; i++) big[i] = (char)((i + seed) & 15);
    for (i = 0; i < 5000; i++) sum += big[i];
    return sum;
}

int main(void)
{
    char pad[1024];
    int guard = 0x5a5a;
    long wide = 1234567890123L;
    short narrow = 30000;
    char byte = 77;
    int i, sum = 0;

    fill(pad, 1024);
    for (i = 0; i < 1024; i++) sum += pad[i];

    printf("%d %d %ld %d %d\n", sum, guard, wide, (int)narrow, (int)byte);
    guard = guard + 1;
    wide = wide * 2;
    narrow = (short)(narrow - 1);
    byte = (char)(byte + 1);
    printf("%d %ld %d %d\n", guard, wide, (int)narrow, (int)byte);
    printf("%d %d %d\n", (int)pad[0], (int)pad[300], (int)pad[1023]);
    printf("%d %d\n", wide_frame(0), wide_frame(3));
    return 0;
}
