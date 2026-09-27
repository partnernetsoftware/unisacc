#if defined(FP_BAD_RETURN)
int left(int x) { return x; }
double right(int x) { return x; }
int main(void) { return (1 ? left : right)(1); }
#elif defined(FP_BAD_PARAMETER)
int left(int x) { return x; }
int right(double x) { return x; }
int main(void) { return (1 ? left : right)(1); }
#elif defined(FP_BAD_PARAMETER_SHAPE)
typedef int A[2]; typedef int B[3];
int left(A *x) { return (*x)[0]; }
int right(B *x) { return (*x)[0]; }
int main(void) { return (1 ? left : right)(0); }
#elif defined(FP_BAD_RETURN_SHAPE)
typedef int A[2]; typedef int B[3];
A a; B b;
A *left(void) { return &a; }
B *right(void) { return &b; }
int main(void) { return (*(1 ? left : right)())[0]; }
#else
/* Typed member, nested, conditional, assignment and array-return calls. */
#include <stdio.h>
struct S { int x; } s={17};
typedef int A[3]; A a={7,11,23};
double twice(double x) { return x*2; }
struct S *gs(void) { return &s; }
A *ga(void) { return &a; }
double thrice(double x) { return x*3; }
double (*choose(int x))(double) { return x ? twice : thrice; }
double (*choose2(int x))(double) { return x ? thrice : twice; }
struct F { double (*f)(double); struct S *(*g)(void); A *(*h)(void); };
int main(void) {
 struct F t={twice,gs,ga};
 double (*(*n)(int))(double)=choose;
 printf("%.1f %d %d %.1f %.1f\n",t.f(3),t.g()->x,(*t.h())[2],(1 ? n : choose2)(1)(4),(t.f=twice)(5));
 return 0;
}
#endif
