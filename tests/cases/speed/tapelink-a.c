/* K5 speed probe 2a: tapelink multi-unit caller */
int tl_mix(int a, int b);
int tl_acc(int n);
int main(void) {
    int s = tl_acc(16);
    return (tl_mix(s, 7) != 0) ? 0 : 1;
}
