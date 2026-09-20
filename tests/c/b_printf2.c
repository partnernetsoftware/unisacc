/* printf evaluates every argument before it writes anything */
int fred(int p) { printf("yo %d\n", p); return 42; }
int (*fp)(int) = &fred;
int seq;
int bump(void) { seq = seq + 1; return seq; }
int main() {
    seq = 0;
    printf("a %d\n", fred(1));
    printf("b %d\n", (*fp)(2));
    printf("%d %d %d\n", bump(), bump(), bump());
    return 0;
}
