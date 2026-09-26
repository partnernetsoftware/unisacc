struct P { int x; int y; };
int main(void){ struct P a[2]; int i; i = 1; a[0].x = 1; a[i].y = 2; return a[0].x + a[i].y; }
