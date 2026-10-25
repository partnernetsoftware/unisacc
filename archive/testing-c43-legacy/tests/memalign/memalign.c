/* 0.0.32 D2: memcpy/memset over every src/dst offset 0..15 and length 0..80 (the word path
   must only run on a shared 8-byte alignment; mismatched pairs stay correct bytewise) */
#include <string.h>
#include <stdio.h>
int main(void) {
    static long sa[16], da[16];
    unsigned char *s = (unsigned char *)sa, *d = (unsigned char *)da;
    int so, dof, n, i, bad = 0;
    for (so = 0; so < 16; so++) for (dof = 0; dof < 16; dof++) for (n = 0; n <= 80; n++) {
        for (i = 0; i < 128; i++) { s[i] = (unsigned char)(i * 7 + 1); d[i] = 0xee; }
        memcpy(d + dof, s + so, n);
        for (i = 0; i < 128; i++) {
            int want = (i >= dof && i < dof + n) ? s[so + i - dof] : 0xee;
            if (d[i] != want) bad++;
        }
        memset(d + dof, 0x5a, n);
        for (i = 0; i < 128; i++) {
            int want = (i >= dof && i < dof + n) ? 0x5a : (i >= dof && i < dof + n ? 0 : d[i]);
            if (i >= dof && i < dof + n && d[i] != 0x5a) bad++;
            (void)want;
        }
        if (dof > 0 && d[dof - 1] == 0x5a) bad++;
        if (dof + n < 128 && d[dof + n] == 0x5a && n > 0) bad++;
    }
    printf("%d\n", bad);
    return bad != 0;
}
