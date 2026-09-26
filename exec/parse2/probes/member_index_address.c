/* Pointer members must be loaded before indexing, even under address-of.
   Independent expected exit: 7 + 8 + 9 + 1 = 25. */
struct Item { int value; };
struct Box { struct Item *entries; int *numbers; };
int main(void) {
    struct Item a[3]; struct Box box; struct Box *p=&box;
    struct Item *x; struct Item *y; int numbers[2]; int *z; int i;
    a[0].value=7; a[1].value=8; numbers[0]=9;
    box.entries=a; box.numbers=numbers;
    i=0; x=&box.entries[i++]; y=&p->entries[1]; z=&box.numbers[0];
    return x->value+y->value+*z+i;
}
