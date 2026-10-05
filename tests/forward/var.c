/* 0.0.28 N1': a variadic host function with integer and pointer arguments is forwarded */
int dprintf(int fd, const char *fmt, ...);
int main(void) {
    int n = dprintf(1, "var %d %s %ld %x\n", 42, "ok", 7L, 255);
    return n == 15 ? 0 : 1;
}
