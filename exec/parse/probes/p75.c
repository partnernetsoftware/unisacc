/* double comparisons with an int or long operand; (double) of a long */
static int f(double d, long n) { if (d == 0.5) return 1; if (d <= (double)n) return 2; return (d >= 2.0) + (d < n); }
int main() { return 0; }
