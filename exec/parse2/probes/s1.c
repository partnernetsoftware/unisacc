int f(int a, int b) { int c; c = a + b * 2; if (a < b) c = -c; else c = !c; while (c) c = c - 1; if (a == b) c = 7; return a + f(c, b) % 3; }
int main() { return f(1, 2); }
