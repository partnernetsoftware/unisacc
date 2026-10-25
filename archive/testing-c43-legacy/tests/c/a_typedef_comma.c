/* typedef with a comma-separated declarator list: `a` is a plain typedef, `*b` a pointer one */
typedef int a, *b;
typedef char c1, c2[4];

int main(void) {
    a x = 7;
    b p = &x;
    c1 ch = 'z';
    c2 s = { 'o', 'k', 0 };
    if (*p != 7) return 1;
    if (ch != 'z') return 2;
    if (s[0] != 'o' || s[1] != 'k') return 3;
    return 0;
}
