struct S { int x; long y; };
int main(void){ int a[3] = {1, 2}; struct S s = {4, 5}; return a[1] + s.x; }
