typedef struct { int quot; int rem; } dv_t;
static dv_t dv(int a, int b) { dv_t r; r.quot = a / b; r.rem = a % b; return r; }
int main(void) { dv_t x; x = dv(7, 2); return x.quot + x.rem; }
