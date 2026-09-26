int g = 42;
int *gp = &g;
int f(int x){ return x + 1; }
int (*fp)(int) = &f;
int (*fp2)(int) = f;
int main(void){ return *gp + fp(1) + fp2(2); }
