/* cc interop slice 1, half A (compiled by unisacc -c -b TARGET): calls the
   integer/pointer functions half B (compiled by cc) defines */
#include <stdio.h>
int add(int a, int b);
long lmul(long a, long b);
signed char nchar(int x);
unsigned char uchar(int x);
short nshort(int x);
unsigned short ushort(int x);
unsigned int uint32(long x);
int neg32(long x);
unsigned long mylen(const char *s);
char *pick(char *s, int k);
void setv(int *p, int v);
long sum6(long a, long b, long c, long d, long e, long f);
int mix6(signed char a, short b, int c, long d, unsigned char e, int *f);
int zero(void);
int main(void) {
    char buf[16] = "interop";
    int v = 0; int w = 7;
    printf("add %d %d\n", add(2, 3), add(-5, 1));
    printf("lmul %ld\n", lmul(123456789L, 1000L));
    printf("char %d %d\n", nchar(200), uchar(-1));
    printf("short %d %d\n", nshort(40000), ushort(-2));
    printf("u32 %u %d\n", uint32(-1L), neg32(0x1FFFFFFFFL));
    printf("len %lu\n", mylen(buf));
    printf("pick %s\n", pick(buf, 2));
    setv(&v, 42); printf("setv %d\n", v);
    printf("sum6 %ld\n", sum6(1, 20, 300, 4000, 50000, 600000));
    v = mix6(-3, -300, 70000, 1L << 40, 250, &w); printf("mix6 %d %d\n", v, w);
    printf("zero %d\n", zero());
    printf("nested %d\n", add(add(1, 2), (int)mylen(pick(buf, 3))));
    return 0;
}
