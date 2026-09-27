/* Shared address walk: every operand is evaluated exactly once. */
int hits; int a[3]={2,3,4}; int ix(void){hits++;return 1;} int *get(void){hits++;return a;} struct S{int n;int *p;}; int fun(void){return 7;} int main(void){struct S s={5,a}; int *p=&((a[ix()])); int *q=&(*get()); int (*f)(void)=&(fun); return *p!=3 || *q!=2 || hits!=2 || *(&(s.n))!=5 || *(&(s.p[2]))!=4 || f()!=7;}
