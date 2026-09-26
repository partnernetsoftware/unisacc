/* subscripts whose index is any expression (out[*n], a[i + j * 2], a[f(i)]); long * parameters */
int id(int x) { return x; }
void put(char *out, long cap, long *n, int c) { if (cap < 0 | *n < cap - 1) out[*n] = c; *n = *n + 1; }
int f(char *a, long *q, int i, int j) { a[i + j * 2] = 5; q[*q - 3] = a[id(i)]; return a[i + j * 2] + q[q[0] - 3] + a[(i, j)]; }
int main() { char b; long n; n = 0; put(&b, 1, &n, 65); return f(&b, &n, 0, 0); }
