/* Case labels share constant expressions and signed 64-bit serialization. */
#include <stdio.h>
enum Op { NEG=-3, START=2, NEXT=START+1 };
int f(long x) {
    switch(x) {
    case NEG: return 1;
    case START+NEXT: return 2;
    case 'a': return 3;
    case 4294967296L: return 4;
    default: return 5;
    }
}
int u(unsigned int x) { switch(x) { case -1: return 6; case 0x80000000: return 7; default:return 8; } }
int narrow(int x) { switch(x) { case 4294967296L: return 9; default:return 0; } }
int nested(long x) { switch(x) { case 4294967296L: switch((unsigned int)-1) { case -1:return 10; } } return 0; }
int main(void) { printf("%d %d %d %d %d\n", f(-3),f(5),f(97),f(4294967296L),f(0)); printf("%d %d %d %d %d\n",u(-1),u(0x80000000),u(0),narrow(0),nested(4294967296L)); return 0; }
