typedef char *va_list;
int g(va_list ap) { long x; x = va_arg(ap, char); x = va_arg(ap, short); x = va_arg(ap, unsigned long); x = va_arg(ap, unsigned char); x = (long)va_arg(ap, int *); va_arg(ap, int); return 0; }
int k(int a, int b, int c, ...) { va_list ap; va_list q; va_start(ap, a); return 0; }
int main() { return 0; }
