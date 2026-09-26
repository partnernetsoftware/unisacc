int f(int a, int b) { int c; c = a + b * 2; if (a < b) c = -c; else c = !c; while (c) c = c - 1; for (c = 0; c < 3; c = c + 1) a = f(a, c); return a; }
int main() { return f(1, 2); }
