#include <stdio.h>
typedef struct { long *v; int n; } A;
typedef struct { int p, f; A a; } N;
int main(void){
    long arr[3] = {5, 6, 7};
    A x = {arr, 3};
    N ini = {-1, 4, x};                 /* 6.7.8p13: a struct-typed initializer initializes the member whole */
    N lit; lit = (N){-2, 8, x};
    A list[2] = {x, x};
    printf("%ld %d | %ld %d | %ld %d\n", ini.a.v[2], ini.a.n, lit.a.v[1], lit.a.n, list[1].v[0], list[1].n);
    return 0;
}
