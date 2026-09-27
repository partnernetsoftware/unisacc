/* Known reference/model defect: an inner tag-only declaration hides the outer complete type. Host cc rejects sizeof the incomplete type. Not an equal-list probe. */
struct S {int x;};
int main(void) {struct S; struct S *p=0; return sizeof(*p);}
