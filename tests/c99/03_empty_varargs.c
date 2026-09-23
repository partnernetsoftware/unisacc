#include <stdio.h>
#define P(fmt, ...) printf(fmt, ##__VA_ARGS__)
int main(void){ P("bare\n"); return 0; }
