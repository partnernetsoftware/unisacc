/* Unsized function-pointer arrays infer their extent; static storage
   survives calls and must not occupy the caller's automatic frame. */
#include <stdio.h>
static int f0(void) { return 10; }
static int f1(void) { return 20; }
static int f2(void) { return 30; }
static int step(void) {
    static int (*a[])(void) = { f0, f1, f2 };
    static int (*b[4])(void) = { f2, f1, f0 };
    int r = a[0]() + a[2]() + b[0]();
    a[0] = f1;
    b[0] = f0;
    return r + (b[3] == 0);
}
static int entry_argc;
int main(int argc, char **argv) {
    entry_argc = argc;
    int (*auto_a[])(void) = { f0, f1, f2 };
    static int (*local[])(void) = { f0, f1, f2 };
    int first = step();
    int second = step();
    if (!argv) return 1;
    printf("%d %d %d %d %d %d %d\n", argc == entry_argc, first, second,
           auto_a[2](), local[1](), (int)sizeof auto_a, (int)sizeof local);
    return 0;
}
