#include <stdio.h>
enum P { POS=148 }; enum N { NEG=-3 }; typedef enum P TP; typedef enum N TN;
struct S { TP p:8; TN n:4; unsigned z:1; };
struct S global={148,-3,1};
int main(void) { struct S a={148,-3,1}; printf("%d %d %d %d\n",(enum P)a.p,a.n,global.p,global.n); { typedef enum N TP; struct L { TP n:4; }; struct L x={-3}; printf("%d\n",x.n); } { struct L { TP p:8; }; struct L x={148}; printf("%d\n",x.p); } { enum P { NP=-2 }; struct L { enum P p:4; }; struct L x={-2}; printf("%d\n",x.p); } { struct L { enum P p:8; }; struct L x={148}; printf("%d\n",x.p); } return 0; }
