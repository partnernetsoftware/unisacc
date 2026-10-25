/* Pointer arithmetic counts ELEMENTS, not bytes (C99 6.5.6): ++, --, +=,
   -=, p + n, p - q -- on int, long and struct pointers.  The C front end
   stepped p++ by one byte and the Python one answered p - arr in bytes;
   no probe had used either on anything wider than a char. */
struct P { int a; int b; long c; };
int main(void) {
    int s[5]; int *p; long l[4]; long *q; struct P ps[3]; struct P *r; int i;
    for (i = 0; i < 5; i++) s[i] = i * 10;
    for (i = 0; i < 4; i++) l[i] = i * 100;
    for (i = 0; i < 3; i++) ps[i].a = i + 1;
    p = s; p++; printf("%d ", *p);
    p = s; ++p; printf("%d ", *p);
    p = s + 3; p--; printf("%d ", *p);
    p = s + 3; --p; printf("%d ", *p);
    p = s; p += 2; printf("%d ", *p);
    p = s + 4; p -= 3; printf("%d ", *p);
    p = s; p = p + 4; printf("%d ", *p);
    p = s + 4; printf("%d ", (int)(p - s));
    q = l; q++; printf("%ld ", *q);
    r = ps; r++; printf("%d ", r->a);
    r = ps; r += 2; printf("%d ", r->a);
    p = s; printf("%d ", *(p++)); printf("%d\n", *p);
    return 0;
}
