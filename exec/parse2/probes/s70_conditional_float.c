#include <stdio.h>
int hits;
double d(void) { hits++; return 2.5; }
float f(void) { hits++; return 1.25f; }
int i(void) { hits++; return 3; }
int main(void) {
    int k;
    unsigned long big = 9223372036854775808UL;
    for(k=0;k<2;k++) {
        double a=k?d():i();
        double b=k?i():d();
        float c=k?f():i();
        double e=k?f():d();
        double n=k?(k?d():i()):(k?i():f());
        double u=k?big:0.5;
        printf("%d %d %d %d %d %d\n",(int)(a*4),(int)(b*4),(int)(c*4),(int)(e*4),(int)(n*4),u>1.0);
    }
    printf("%d\n",hits);
    return 0;
}
