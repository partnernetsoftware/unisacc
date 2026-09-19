/* short is 2 bytes and truncates */
short g = 0;
int main() {
    short a; short b[4]; int i;
    a = 300; g = -5;
    for (i = 0; i < 4; i = i + 1) b[i] = i * 1000;
    printf("%d %d %d %d %d %d\n", (int)a, (int)g, (int)b[0], (int)b[3],
           (int)sizeof(b), (int)(a + g));
    a = 70000;
    printf("%d %d %d\n", (int)a, (int)sizeof(short), (int)sizeof(long int));
    return 0;
}
