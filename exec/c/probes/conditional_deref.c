/* Pointer/null common types and shared dereference compound assignment. */
#if defined(QT_RUNTIME_ZERO)
int main(void) { int x=1,z=0; int *p=&x; return *(1?p:z); }
#elif defined(QT_FLOAT_ZERO)
int main(void) { int x=1; int *p=&x; return *(1?p:0.0); }
#elif defined(QT_DIFFERENT_POINTER)
int main(void) { int x=1; double y=2; return *(1?&x:&y); }
#else
#include <stdio.h>
unsigned value=7; int calls=0;
unsigned *address(void) { ++calls; return &value; }
int rhs(void) { ++calls; return 3; }
enum { ZERO=0 };
int main(void) {
 int x=23, yes=1; int *p=&x; void *q=p;
 int *a=yes?p:0; int *b=yes?0:p; int *c=yes?p:(void*)0;
 int *d=yes?p:((ZERO-0)*0); void *e=yes?q:p;
 *address() += rhs(); *(unsigned*)((char*)&value) ^= 2;
 printf("%d %d %d %d %d %u %d\n",*a,b==0,*c,*d,*(int*)e,value,calls);
 return 0;
}
#endif
