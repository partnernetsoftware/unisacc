static int probe(int n) { int a[n]; a[0] = 11; a[n-1] = 17; return a[0] + a[n-1] - 28; }
int main(void) { return probe(3); }
