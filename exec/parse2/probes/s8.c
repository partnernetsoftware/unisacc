typedef char *va_list;
int vf(int a, long b, ...) { va_list ap; int x; va_start(ap, b); x = va_arg(ap, int); va_end(ap); return x + a; }
int main(void) { return vf(1, 2, 3); }
