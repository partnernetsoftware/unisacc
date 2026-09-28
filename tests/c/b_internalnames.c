/* Ordinary function names must not alias compiler control-flow labels. */
int R1(void) { return 7; }
int L3(int v) { if (v) return 3; return 4; }
int main(void) {
    int a, b, c;
    a = R1();
    b = L3(1);
    c = L3(0);
    return a == 7 && b == 3 && c == 4 ? 0 : 19;
}
