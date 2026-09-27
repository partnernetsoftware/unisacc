static int same=3;
static int worker(int x) { goto same; same: return x+(x ? same : 0); }
int f(void) { return worker(5); }
