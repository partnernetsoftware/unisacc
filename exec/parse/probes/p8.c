int s(int n) { int t = 0, i; for (i = 0; i < n; i = i + 1) { int j; for (j = i; j; j = j - 1) t = t + j; while (t > 100 || t < 0 && !t) t = t - 100; } return t; }
int main() { return s(10) == 165; }
int h() { return 0; }
