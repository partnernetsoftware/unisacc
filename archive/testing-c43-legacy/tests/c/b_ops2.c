/* ~, octal, the compound assignments that were missing, line splicing */
#define SUM(a, b) \
    ((a) + \
     (b))
int main() {
    int x; int y;
    x = 0022;
    y = 5;
    y %= 3; y <<= 4; y |= 1; y &= 0xff; y ^= 2; y >>= 1;
    printf("%d %d %d %d\n", x, ~x, y, SUM(x, 3));
    return 0;
}
