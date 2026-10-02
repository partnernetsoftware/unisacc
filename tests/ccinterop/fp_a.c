/* cc interop slice 2B/2C, half A (unisacc -c -b TARGET): floating-point
   arguments and returns, mixed with integers, and calls with 8 and 10
   arguments (stack arguments) */
#include <stdio.h>
double dadd(double a, double b);
float fmulf(float a, float b);
double mixd(int a, double b, long c, float d, char *e, double f);
float tofl(double x);
double fromfl(float x);
long dtol(double x, int k);
int fcmp(float a, double b);
double nine(double a, double b, double c, double d, double e, double f, double g, double h, double i, double j);
long sum8(long a, long b, long c, long d, long e, long f, long g, long h);
long sum10(long a, int b, long c, int d, long e, long f, long g, long h, int i, long j);
double mix10(int a, double b, long c, float d, int e, double f, long g, long h, long i, long j);
long ptr8(char *a, long b, long c, long d, long e, long f, long g, char *h);
int main(void) {
    char s[8] = "abcdefg";
    printf("dadd %.6f\n", dadd(1.25, -3.5));
    printf("fmulf %.6f\n", (double)fmulf(1.5f, -2.25f));
    printf("mixd %.6f\n", mixd(3, 0.5, -40L, 2.5f, s, 1e10));
    printf("tofl %.6f fromfl %.6f\n", (double)tofl(3.141592653589793), fromfl(0.1f));
    printf("dtol %ld\n", dtol(-7.9, 3));
    printf("fcmp %d %d\n", fcmp(1.5f, 1.5), fcmp(1.0f, 2.0));
    printf("nine %.6f\n", nine(1, 2, 3, 4, 5, 6, 7, 8, 9.5, 10.25));
    printf("sum8 %ld\n", sum8(1, 2, 3, 4, 5, 6, 70, 800));
    printf("sum10 %ld\n", sum10(1, -2, 3, -4, 5, 6, 7, 8, -9000, 1L << 40));
    printf("mix10 %.6f\n", mix10(1, 2.5, 3, 4.5f, 5, 6.5, 7, 8, 9, 10));
    printf("ptr8 %ld\n", ptr8(s, 1, 2, 3, 4, 5, 6, s + 3));
    printf("nested %.6f\n", dadd(dadd(0.5, 0.25), (double)sum8(1, 1, 1, 1, 1, 1, 1, 1)));
    return 0;
}
