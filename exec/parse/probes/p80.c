struct N { char tag; int v; struct N *next; long w; };
int sum(struct N *h) { int t; t = 0; while (h) { t = t + h->v + h->tag; h = h->next; } return t; }
int main(void) {
    struct N a; struct N b; struct N *p;
    a.tag = 1; a.v = 10; a.w = 100; a.next = &b;
    b.tag = 2; b.v = 20; b.next = 0;
    p = &a;
    p->next->v = p->v + 5;
    return sum(p) + (int)a.w;
}
