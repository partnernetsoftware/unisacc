/* Anonymous static identity and nested initializers use the ordinary walker. */
struct S { int n; struct S *next; };
struct S *first = &(struct S){1, &(struct S){2, 0}};
struct S *other = &(struct S){1, 0};
struct S copy = ((struct S){3, &(struct S){4, 0}});
int hits;
int tick(void) { hits++; return hits; }
int main(void) {
    struct S *p;
    struct S *saved = 0;
    int *a;
    int i = 0;
    while (i < 2) {
        p = &(struct S){tick(), &(struct S){7, 0}};
        if (i && p != saved) return 1;
        saved = p;
        if (p->n != i + 1 || p->next->n != 7) return 2;
        i++;
    }
    a = (int[]){8, 9};
    return hits != 2 || first == other || first->n != 1
        || first->next->n != 2 || other->n != 1 || copy.n != 3
        || copy.next->n != 4 || a[1] != 9;
}
