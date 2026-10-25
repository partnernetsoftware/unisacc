/* A struct array member decays; a nested struct itself does not. */
#include <stdio.h>
struct Pair { int x; int y; };
struct Box { char lead; struct Pair one[1]; struct Pair many[2]; struct Pair plain; };
struct Box box = {'x', {{2,3}}, {{4,5},{6,7}}, {8,9}};
int main(void) {
    struct Box local = {'y', {{10,11}}, {{12,13},{14,15}}, {16,17}};
    struct Box *p = &local;
    box.many[1].x = 20;
    p->many[1].y = 30;
    printf("%d %d %d %d %d %d %d %d\n",box.one[0].y,box.many[0].x,
           box.many[1].x,box.plain.y,p->one[0].x,p->many[1].y,
           p->plain.x,(int)sizeof p->many);
    return 0;
}
