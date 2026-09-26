struct P { int a; char c; long l; };
int g[5];
int main() { int a[3][4]; long x; char *p; struct P s; int n;
  n = sizeof(int) + sizeof(long) + sizeof(char *) + sizeof(struct P);
  n = n + sizeof a + sizeof(a[0]) + sizeof x + sizeof(p) + sizeof s + sizeof g + sizeof(double);
  return n; }
