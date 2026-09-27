/* Declared compound-literal type survives its initializer expressions. */
#include <stdio.h>
struct P { int x, y; };
int main(void) {
    int x=3; int *p=&x; struct P s={5,7};
    int a=*(int *){&x};
    int b=*((int *[]){&x})[0];
    int c=**(int **){&p};
    int d=((struct P *){&s})->y;
    int e=(double){3.0} == 3.0;
    int f=(float){2.5} == 2.5f;
    int g=(unsigned char){255} == 255;
    int h=(_Bool){9} == 1;
    int i=((double[]){1.25,2.5})[1] == 2.5;
    int j=(struct P){(int){4},6}.x;
    printf("%d %d %d %d / %d %d %d %d %d %d\n",a,b,c,d,e,f,g,h,i,j);
    return 0;
}
