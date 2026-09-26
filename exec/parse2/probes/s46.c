/* Static and global initializer order; qualifiers count in label ordinals. */
int before = 3;
int tick(void) {
    static const int n = 2;
    static char *p = "xy";
    static int count;
    count++;
    return n + p[1] + count;
}
int after = 7;
int main(void) { return tick() + tick() - before - after; }
