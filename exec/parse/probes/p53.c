/* local arrays T a[N]: frame layout, a[i] read/write, decay in calls/arithmetic/assignment, &a[i] */
int f(int *p) { return *p; }
int main() { int x; char c[5]; int a[3]; long *q[2]; x = 7; a[1] = x; c[2] = 3; x = a[1] + c[2]; f(a); q[0] = &a[2]; x = *(a + 1); return f(&a[1]); }
