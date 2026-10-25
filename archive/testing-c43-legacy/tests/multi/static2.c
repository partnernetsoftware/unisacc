int g(void) {
    static int x = 9;
    static char text[] = "X";
    static int pair[2] = {2, 3};
    static char *ptr = "q";
    return x++ + text[0] - 88 + pair[1] - 3 + ptr[0] - 113;
}
