/* string literals as values: in ?:, assigned to a char *, indexed; interleaved with printf formats in the pool */
int g(char *s) { return s[1]; }
int h() { return g("nan") + g("nan") + g(""); }
int main() { char *w; int u; u = 1; w = u ? "NAN" : "nan"; printf("x%d\n", u); w = "a\tb"; return w[0] + w[2]; }
