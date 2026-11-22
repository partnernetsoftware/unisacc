/* K5 speed probe 2b: tapelink multi-unit callee */
int tl_mix(int a, int b) { return (a * 33) ^ (b + 19); }
int tl_acc(int n) {
    int s = 1;
    for (int i = 1; i <= n; i++) s = tl_mix(s, i);
    return s;
}
