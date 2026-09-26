/* (double) of an int; a double comparison */
static double f(int i) { double d; d = (double)(i - 1); if (d > 1.0) return d * (double)i; return d; }
int main() { return 0; }
