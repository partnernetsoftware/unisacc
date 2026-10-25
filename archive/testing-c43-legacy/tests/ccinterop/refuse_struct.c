struct s{int a;}; int f(struct s); int main(void){struct s v; v.a=1; return f(v);}
