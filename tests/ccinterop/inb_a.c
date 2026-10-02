/* cc interop slice 2, inbound half A (unisacc -c -b TARGET): functions
   declared `extern` here and defined here are exported to the cc half with
   the host C ABI; half B calls them back */
#include <stdio.h>
extern int twice(int x);
extern long sum6x(long a, long b, long c, long d, long e, long f);
extern char *skip(char *s, int k);
extern void store(int *p, int v);
extern unsigned char lowb(int x);
extern int depth(int n);
long drive(int k);              /* defined by cc */
int counter;
int twice(int x) { counter = counter + 1; return 2 * x; }
long sum6x(long a, long b, long c, long d, long e, long f) { return a - b + c - d + e - f; }
char *skip(char *s, int k) { return s + k; }
void store(int *p, int v) { *p = v + twice(v); }
unsigned char lowb(int x) { return (unsigned char)x; }
int depth(int n) { return n <= 0 ? 0 : 1 + (int)drive(n - 1); }
int main(void) {
    printf("internal %d\n", twice(21));
    printf("drive0 %ld\n", drive(0));
    printf("drive1 %ld\n", drive(1));
    printf("drive2 %ld\n", drive(2));
    printf("drive3 %ld\n", drive(3));
    printf("depth %d\n", depth(5));
    printf("counter %d\n", counter);
    return 0;
}
