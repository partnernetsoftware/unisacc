struct P {
    int x;
    int y;
};

int main(void) {
    struct P p;
    struct P *q;
    p.x = 4;
    p.y = 5;
    q = &p;
    printf("%d\n", q->x + q->y);
    return 0;
}
