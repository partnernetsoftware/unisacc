int main(void) {
    int i;
    int s;
    s = 0;
    for (i = 0; i < 4; i++) {
        switch (i) {
        case 0:
            s += 1;
            break;
        case 1:
            s += 2;
            break;
        case 2:
            s += 3;
            break;
        default:
            break;
        }
    }
    printf("%d\n", s);
    return 0;
}
