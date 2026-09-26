/* pointer comparisons: == != < <= > >= with 0 and with pointers, if (p), !p, && on pointers */
int f(char *p, long *q, char *r) {
  int a; a = 0;
  if (p != 0) a = 1;
  if (p == r) a = a + 2;
  if (p < r) a = a + 3;
  if (0 == p) a = 4;
  if (p) a = a + 5;
  if (!p) a = 6;
  if (!q) a = a + 1;
  if (q > 0 && p <= r) a = a + (p && q) + (p || q) + (q == 0) + (q != 0);
  a = a + (p >= r);
  return a;
}
int main() { char c; long l; return f(&c, &l, &c) + f(0, 0, &c); }
