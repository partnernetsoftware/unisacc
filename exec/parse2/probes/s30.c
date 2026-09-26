int a[3] = {1, 2, 3};
struct S { int x; long y; };
struct S s = {4, 5};
int g = 7;
int main(void){ return a[1] + s.x + g; }
