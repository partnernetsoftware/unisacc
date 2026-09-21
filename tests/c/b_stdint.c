/* <stdint.h>'s unsigned spellings really are unsigned.  They used to alias
   the signed ones, on the theory that only the width mattered; a uint8_t of
   0xa2 then printed as ffffffffffffffa2, because the load sign-extended. */
#include <stdio.h>
#include <stdint.h>

int main(void)
{
    uint8_t b = 0xa2;
    uint16_t h = 0xbeef;
    uint32_t w = 0xdeadbeef;
    int8_t sb = -3;
    int i;
    uint8_t buf[4];

    printf("%x %x %x %d\n", b, h, w, (int)sb);
    printf("%.2x %.4x %.8x\n", b, h, w);
    printf("%d %d %d\n", (int)b, (int)h, (int)(w >> 16));
    buf[0] = 0x00; buf[1] = 0x7f; buf[2] = 0x80; buf[3] = 0xff;
    for (i = 0; i < 4; i++) printf("%.2x", buf[i]);
    printf("\n%.5d|%.3o|%.1d|\n", 42, 9, 7);
    return 0;
}
