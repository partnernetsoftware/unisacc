int run(void) {
    static char a[5];
    static int n = 2;
    a[0] = a[0] + 1;
    n = n + 1;
    { static int n; n = n + 3; }
    return a[0] + n;
}
int main(void) { return run() + run(); }
