/* C99 6.5.17p2: evaluating the left operand reads a volatile lvalue.
   tests/volatilecomma.py checks actual VM reads and system-cc assembly,
   rather than merely comparing two compiler tapes. */
volatile int observed = 7;
int ordinary = 7;
int designator(void) { return 4; }
int main(void) {
    volatile int *p = &observed;
    int i = 0;
    int (*fp)(void) = designator;
    (observed, 0);
    (ordinary, 0);
    observed;                 /* discarded value still reads */
    &observed;                /* address-of does not read */
    sizeof(observed);         /* unevaluated operand does not read */
    (*p, 0);
    *p;                       /* dedicated star-statement path */
    &*p;
    sizeof(*p);
    for (observed; i < 1; observed) i++;
    *fp;                      /* function designator: no code-as-data read */
    return 0;
}
