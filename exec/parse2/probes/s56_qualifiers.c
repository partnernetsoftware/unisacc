static inline int add(int * restrict a, int * restrict b) { return *a + *b; }
int main(void) { int x=2,y=3; int * const restrict p=&x; return add(p,&y)-5; }
