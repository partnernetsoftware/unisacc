int main() { int a[8]; int *p; int *q; long d; char *c; char *e;
  p = a; q = p + 5; d = q - p; p = 2 + q; c = 0; e = c + 3; d = d + (e - c);
  return d; }
