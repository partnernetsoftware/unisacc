/* more arguments than there are argument registers: all of them go on the
   tape stack, pushed in source order */
int f8(int a, int b, int c, int d, int e, int f, int g, int h) {
    return a * 1 + b * 2 + c * 3 + d * 4 + e * 5 + f * 6 + g * 7 + h * 8;
}
int f6(int a, int b, int c, int d, int e, int f) { return a + b + c + d + e + f; }
int rec(int n, int a, int b, int c, int d, int e, int f, int g) {
    if (n == 0) return a + b + c + d + e + f + g;
    return rec(n - 1, a + 1, b, c, d, e, f, g);
}
int main() {
    printf("%d %d %d\n", f8(1, 2, 3, 4, 5, 6, 7, 8), f6(1, 2, 3, 4, 5, 6),
           rec(3, 0, 1, 2, 3, 4, 5, 6));
    return 0;
}
