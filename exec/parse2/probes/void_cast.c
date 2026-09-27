/* Casting to void evaluates side effects once and performs no numeric conversion. */
int count;
int tick(void) { count++; return 7; }
void note(void) { count++; }
int main(void) {
    int x = 0;
    char c = 3;
    int *p = &x;
    void *q = (void *)p;
    int y;
    (void)c;
    (void)tick();
    (void)x++;
    (void)(void)note();
    y = ((void)++x, tick());
    (void)(float)2.5;
    (void)(double)3.5;
    (void)(p = (int *)q);
    return count!=3 || x!=2 || y!=7 || p!=&x
        || (unsigned char)257!=1 || (int)3.5!=3;
}
