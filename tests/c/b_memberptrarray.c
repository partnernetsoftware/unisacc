#include <stdio.h>
#include <stdlib.h>
struct F { int n; int *p[]; };
struct A { char tag; int *p[2]; };
struct C { int **p[1]; };
int main(void) {
 int a[2]={13,17}; int *q=a; struct A x; struct C y;
 struct F *f=malloc(sizeof(struct F)+2*sizeof(int*));
 x.p[0]=a; x.p[1]=a+1; f->p[0]=a; f->p[1]=a+1; y.p[0]=&q;
 printf("%d %d %d %d %d\n",x.p[0][1],*x.p[1],f->p[0][1],*f->p[1],**y.p[0]);
 free(f); return 0;
}
