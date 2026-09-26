typedef char *va_list;
int f(int n, ...) { va_list ap; double d; unsigned long b; va_start(ap, n); d = va_arg(ap, double); b = *(unsigned long *)&d; va_end(ap); return (int)(b >> 52); }
int main(void) { return 0; }
