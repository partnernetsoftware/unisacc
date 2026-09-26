/* Subscripts after a pointer-valued call, also after a multidimensional value. */
char *word(void) { return "xyz"; }
int main(void) {
    int a[2][3]; int n;
    a[0][0] = 2; n = a[0][0];
    return (word()[0] & 255) + word()[n];
}
