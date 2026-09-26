int f(int *p){int a=1; int *q=&a; *q=*p+2; return *q;}
int g(int **pp){int *r=*pp; **pp=3; return *r;}
int main(){int x=4; int *y=&x; int k=f(y); return k+g(&y);}
