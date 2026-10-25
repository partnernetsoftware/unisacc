/* cc interop slice 1, half B (compiled by cc, no libc) */
int add(int a, int b) { return a + b; }
long lmul(long a, long b) { return a * b; }
signed char nchar(int x) { return (signed char)x; }
unsigned char uchar(int x) { return (unsigned char)x; }
short nshort(int x) { return (short)x; }
unsigned short ushort(int x) { return (unsigned short)x; }
unsigned int uint32(long x) { return (unsigned int)x; }
int neg32(long x) { return (int)x; }
unsigned long mylen(const char *s) { unsigned long n = 0; while (s[n]) n++; return n; }
char *pick(char *s, int k) { return s + k; }
void setv(int *p, int v) { *p = v; }
long sum6(long a, long b, long c, long d, long e, long f) { return a + b + c + d + e + f; }
int mix6(signed char a, short b, int c, long d, unsigned char e, int *f) { *f = *f * 2; return a + b + c + (int)(d >> 40) + e; }
int zero(void) { return 0; }
