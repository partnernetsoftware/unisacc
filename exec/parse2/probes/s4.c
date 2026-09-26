long g(char *s, long n) { char c; c = s[1]; *s = c; n = n + 2; return n; }
int main(void) { char buf; long v; int *p; int x; x = 3; p = &x; *p = *p + 1; v = g(&buf, 4); return x + (int)v; }
