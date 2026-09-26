typedef char *va_list;
int g(const char *f, va_list ap) { int x; char *s; long l; x = va_arg(ap, int); s = va_arg(ap, char *); l = va_arg(ap, long); return x; }
int f(const char *fmt, ...) { va_list ap; int r; va_start(ap, fmt); r = g(fmt, ap); va_end(ap); return r; }
int h(int a, long b, ...) { va_list ap; va_start(ap, b); a = va_arg(ap, int); va_end(ap); return a; }
int main() { return 0; }
