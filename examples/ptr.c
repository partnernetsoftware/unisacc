int main(void) {
    int a[3];
    int *p;
    a[0] = 1;
    a[1] = 2;
    a[2] = 4;
    p = a;
    printf("%d\n", p[0] + p[1] + *(p + 2));
    return 0;
}
