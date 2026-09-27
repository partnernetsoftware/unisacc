#include <stdio.h>
static int same=7;
static int worker(int x) { goto same; same: return x+(x ? same : 0); }
int f(void);
int main(void) { printf("%d %d\n",f(),worker(2)); return 0; }
