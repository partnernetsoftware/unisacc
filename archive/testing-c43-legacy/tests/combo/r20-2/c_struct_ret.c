typedef struct { int a; int b; } T;
static T mk(int n){T t; t.a=n; t.b=n; return t;}
static T g(int n){return mk(n);}
int main(void){return g(2).a-2;}
