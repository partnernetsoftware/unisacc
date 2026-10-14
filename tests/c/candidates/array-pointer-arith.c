#include <stdio.h>

unsigned char grows[4][8];
unsigned char (*gp)[8] = grows;

int main(void) {
    unsigned char rows[4][8] = {{0}};
    unsigned char (*p)[8] = rows;
    unsigned char (*r)[8];
    unsigned char *q;
    int ints[4][3] = {{0}};
    int (*ip)[3] = ints;
    int *iq;
    int i = 2;

    q = *(p+2); q[0] = 5;
    if (rows[2][0] != 5) return 1;
    q = *(p+i); q[1] = 6;
    if (rows[2][1] != 6) return 2;
    q = *(2+p); q[2] = 7;
    if (rows[2][2] != 7) return 3;
    r = p+2; r[0][3] = 8;
    if (rows[2][3] != 8) return 4;
    if (sizeof(*(p+2)) != 8) return 5;
    if ((p+3)-(p+1) != 2) return 6;
    q = p[2];
    if (q[0] != 5 || q[3] != 8) return 7;

    q = *(gp+2); q[0] = 9;
    q = *(gp+i); q[1] = 10;
    q = *(2+gp); q[2] = 11;
    r = gp+2; r[0][3] = 12;
    if (grows[2][0] != 9 || grows[2][1] != 10 ||
        grows[2][2] != 11 || grows[2][3] != 12) return 8;
    if (sizeof(*(gp+2)) != 8 || (gp+3)-(gp+1) != 2) return 9;

    iq = *(ip+3-1); iq[2] = 13;
    if (ints[2][2] != 13 || sizeof(*(ip+2)) != 12) return 10;
    if ((ip+3)-(ip+1) != 2) return 11;
    iq = *(1+(1+ip)); iq[1] = 14;
    if (ints[2][1] != 14) return 12;
    puts("pass");
    return 0;
}
