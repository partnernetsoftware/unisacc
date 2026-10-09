/* examples/cx-lab/sample.c -- tiny probe input for the cx-lab experiment, nothing else reads this file */
int add(int a, int b) { return a + b; } // a line comment
/* a block
   comment */
int main(void) {
    int x = add(2, 3);
    const char *msg = "hi";
    int i, sum = 0;
    for (i = 0; i < x; i++) {
        if (i % 2 == 0) sum = sum + i;
        else sum = sum - i;
    }
    return sum;
}
