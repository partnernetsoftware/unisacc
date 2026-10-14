#include <stdio.h>
struct Slots { unsigned char (*dl)[8]; };
static void store(unsigned char *p, unsigned long value) {
    for (int i=0;i<8;i++) { p[i]=value&255; value=value>>8; }
}
int main(void) {
    unsigned char bytes[4][8]={{0}};
    struct Slots c={bytes};
    for(int j=0;j<4;j++) store(c.dl[j],(unsigned long)(j+1));
    for(int j=0;j<4;j++) if(bytes[j][0]!=j+1) return 9;
    puts("array member writes: pass");
    return 0;
}
