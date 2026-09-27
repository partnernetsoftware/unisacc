#include <stdio.h>
#include <stdlib.h>
struct C { int count; char tail[]; };
struct D { char tag; double tail[]; };
struct P { int count; int *tail[]; };
struct V { int x; int y; };
struct S { int count; struct V tail[]; };
int main(void) {
 struct C *c=malloc(sizeof(struct C)+4);
 struct D *d=malloc(sizeof(struct D)+2*sizeof(double));
 struct P *p=malloc(sizeof(struct P)+2*sizeof(int *));
 struct S *s=malloc(sizeof(struct S)+2*sizeof(struct V));
 int n=17; c->count=3; c->tail[0]=65; c->tail[2]=67;
 d->tail[0]=1.5; d->tail[1]=2.5;
 p->tail[0]=&n; s->tail[1].x=8; s->tail[1].y=9;
 printf("%d %d %d %d %d %d %d %d\n",(int)sizeof(struct C),(int)sizeof(struct D),(int)sizeof(struct P),(int)sizeof(struct S),c->tail[2],(int)(d->tail[0]+d->tail[1]),*p->tail[0],s->tail[1].x+s->tail[1].y);
 free(c);free(d);free(p);free(s);return 0;
}
