#include <stdio.h>
#include <stdlib.h>
struct S { int n; char d[]; };
int main(void){ struct S *s = malloc(sizeof(struct S)+4); s->n=3; s->d[0]=65; printf("%d %c\n", s->n, s->d[0]); return 0; }
