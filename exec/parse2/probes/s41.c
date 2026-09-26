/* Literal subscripts reuse POSTIX and must not inherit a prior array rank. */
int main(void) {
    int a[2][3]; int k = 1; int v;
    a[0][0] = 7;
    v = a[0][0];
    return v + "abc"[k] + ("xy" "z")[2];
}
