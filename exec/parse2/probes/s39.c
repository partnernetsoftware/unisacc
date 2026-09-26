/* Constant bounds share the declared precedence table with expressions. */
enum E { N = 2 + 3 * 4, K = (N << 1) - 1, M = !0 + (~0 & 3) };
char a[64 * 1024];
int b[(K > N && M == 4) ? 3 : 7];
int c[1 | 2 ^ 3 & 4];
int main(void) {
    char d[(18 / 3 + 5 % 3) * 2 - -1];
    return !(sizeof(a) == 65536 && sizeof(b) == 12 && sizeof(c) == 12 && sizeof(d) == 17);
}
