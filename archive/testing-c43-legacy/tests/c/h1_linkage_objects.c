/* H1-1: extern inherits internal linkage; automatic names do not erase it. */
static int x = 3, y = 4;
extern int x, y;
static int x, y;

static int shadow(void) {
    int x = 9;
    return x;
}

extern int x;
static int x;

int main(void) {
    return x == 3 && y == 4 && shadow() == 9 ? 0 : 1;
}
