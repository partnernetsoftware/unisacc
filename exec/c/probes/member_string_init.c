/* Character-array members share string decoding across storage classes. */
#include <stdio.h>
struct P { int pre; char exact[2]; unsigned char pad[5]; char *ptr; int tail; };
struct P global={11,"AB","C","xy",17};
struct N { char lead; struct P p; int end; } nested={'!',{31,"GH","I","rs",37},41};
struct Q { char *names[2]; int values[2]; } other={{"hi","jk"},{3,4}};
struct R { int pre; char rows[2][3]; int tail; } rows={51,{"JK","LM"},57};
void show(struct P *p) {
 printf("%d %c%c %c %d %d %s %d\n",p->pre,p->exact[0],p->exact[1],p->pad[0],p->pad[1],p->pad[4],p->ptr,p->tail);
}
int main(void) {
 struct P local={21,{"DE"},{"F",},"uv",27};
 static struct P fixed={61,"NO","P","wx",67};
 show(&global); show(&local); show(&fixed); show(&nested.p);
 printf("%c %d %d %s %s %d\n",nested.lead,nested.end,rows.pre,rows.rows[0],rows.rows[1],rows.tail);
 printf("%s %s %d %d\n",other.names[0],other.names[1],other.values[0],other.values[1]);
 return 0;
}
