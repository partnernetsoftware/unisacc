/* Declared return kind converts the expression, then narrows integers. */
#include <stdio.h>
double d(int x) { return x; }
float f(double x) { return x; }
int i(double x) { return x; }
unsigned long u(double x) { return x; }
float fu(unsigned long x) { return x; }
double du(unsigned long x) { return x; }
unsigned char c(double x) { return x; }
short s(float x) { return x; }
_Bool b(double x) { return x; }
int main(void) {
 unsigned long big=9223372036854779904UL;
 printf("%d %d %d %d\n",(int)d(3),(int)(f(2.75)*4),i(3.75),u(9223372036854779904.0)==big);
 printf("%d %d %d %d %d\n",fu(big)>0.0f,du(big)>0.0,c(255.75),s(123.75f),b(0.5));
 return 0;
}
