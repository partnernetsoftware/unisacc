/* long long: a return type, a parameter and a local, as long */
static long long f(long long v) { long long w; w = v * 3; if (w < 0) return 0 - w; return w; }
int main() { return (int)f(0 - 5); }
