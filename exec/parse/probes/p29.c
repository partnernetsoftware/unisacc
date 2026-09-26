int *f(int *q){ return q; } int main(){int a=1; int *p=f(&a)+1; p=p-1; return *p;}
