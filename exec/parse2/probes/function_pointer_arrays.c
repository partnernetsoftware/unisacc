#include <stdio.h>
typedef int (*Handlers[4])(int);
int apply(int (*[4])(int), int);
int plus(int x){return x+10;}
int times(int x){return x*3;}
int apply(Handlers fp,int i){return (*fp[i])(i);}
int twice(int (int),int);
int twice(int f(int),int n){return f(n)+f(n);}
int parenthesized(int ([4]),int);
int parenthesized(int a[4],int n){return a[n];}
typedef int (*Plain)(void);
int nested(int (int()),Plain);
int leaf(void){return 11;}
int invoke(Plain f){return f()+1;}
int nested(int f(int()),Plain g){return f(g);}
int named(int (*a[4])(int),int i){return (*a[i])(i);}
typedef double (*Floating[2])(double);
double fraction(double x){return x+1.5;}
double floatapply(Floating fs,int i){return fs[i](2);}
Handlers global={plus,times,plus,times};
int main(void){
    Handlers fs={plus,times,plus,times};
    int a[4]={4,5,6,7};
    Floating ds={fraction,fraction};
    printf("%d %d %d %d %d %d\n",apply(fs,0),apply(fs,1),apply(fs,2),apply(fs,3),twice(plus,2),parenthesized(a,3));
    printf("%d %d %d\n",nested(invoke,leaf),(int)sizeof(fs),(int)sizeof(fs[0]));
    printf("%d %d %f\n",named(fs,2),apply(global,3),floatapply(ds,1));
    return 0;
}
