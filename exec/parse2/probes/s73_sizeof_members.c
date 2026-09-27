#include <stdio.h>
struct Inner {char one[1]; int many[3];};
struct Outer {struct Inner in; struct Inner a[2]; struct Inner *p; long *ptrs[3];};
int main(void){struct Outer o;struct Outer *p=&o;
 printf("%lu %lu %lu %lu %lu %lu\n",(unsigned long)sizeof o.in.one,(unsigned long)sizeof(o.in.many),(unsigned long)sizeof p->a,(unsigned long)sizeof(p->p->many),(unsigned long)sizeof o.ptrs,(unsigned long)sizeof o.a->many);
 return 0;}
