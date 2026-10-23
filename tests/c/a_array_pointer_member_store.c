#include <stdio.h>
struct Slots { unsigned char (*dl)[8]; };
static void store(unsigned char *p, unsigned long value) {
    for (int i=0;i<8;i++) { p[i]=value&255; value=value>>8; }
}
static int bytes_at(struct Slots x, int r, int k) { return x.dl[r][k]; }
static int take(struct Slots x) { x.dl[2][1] = 7; return bytes_at(x, 2, 1); }
int main(void) {
    unsigned char bytes[4][8]={{0}};
    struct Slots c={bytes};
    for(int j=0;j<4;j++) store(c.dl[j],(unsigned long)(j+1));
    for(int j=0;j<4;j++) if(bytes[j][0]!=j+1) return 9;
    /* `.` and `->` reach the same row; a row read back through a
       plain pointer, and a row passed by value, index the same way */
    struct Slots *s = &c;
    s->dl[3][7] = 9; unsigned char *q = s->dl[3]; q[0] = 4;
    if (bytes[3][7] != 9 || bytes[3][0] != 4) return 10;
    if (c.dl[2] != &bytes[2][0] || s->dl[1] != &bytes[1][0]) return 11;
    if (take(c) != 7) return 12;
    puts("array member writes: pass");
    return 0;
}
