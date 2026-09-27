/* Declared parameter conversions reuse assignment conversion facts. */
int calls;
double source(void) { calls++; return -9.75; }
int as_char(char x) { return x; }
int as_short(short x) { return x; }
int as_int(int x) { return x; }
long as_long(long x) { return x; }
unsigned as_unsigned(unsigned x) { return x; }
int as_bool(_Bool x) { return x; }
float as_float(float x) { return x; }
double as_double(double x) { return x; }
int main(void) {
    if (as_char(source()) != -9 || calls != 1) return 1;
    if (as_short(1234.875) != 1234) return 2;
    if (as_int(-33.5f) != -33) return 3;
    if (as_long(123456789.75) != 123456789L) return 4;
    if (as_unsigned(12345.75) != 12345U) return 5;
    if (as_bool(-0.25) != 1 || as_bool(0.0) != 0) return 6;
    if (as_float(17) != 17.0f || as_double(23) != 23.0) return 7;
    return 0;
}
