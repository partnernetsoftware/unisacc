int main(void) { int a[2][3]; a[1][2] = 7; { int a[3][2]; a[2][1] = 4; } return a[1][2]; }
