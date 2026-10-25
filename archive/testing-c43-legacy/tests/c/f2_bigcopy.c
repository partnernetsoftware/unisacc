/* 0.0.28 F2: struct copies past 256 bytes (by value in, by value out, assignment) are a loop with an
   unrolled tail; 20,003 bytes leaves a 3-byte tail.  cc prints 42 7 7 9 5. */
#include <stdio.h>
#include <string.h>
struct B { char d[20000]; char t3[3]; int t; };
static struct B a, b, c;
static struct B f(struct B x) { x.t++; x.t3[2] = 5; return x; }
int main(void) {
    memset(&a, 7, sizeof a); a.t = 41; a.d[9] = 9;
    b = f(a); c = b;
    printf("%d %d %d %d %d\n", c.t, c.d[0], c.d[19999], c.d[9], c.t3[2]);
    return 0;
}
