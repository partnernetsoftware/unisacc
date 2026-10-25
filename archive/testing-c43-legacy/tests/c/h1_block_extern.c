#include <stdio.h>
int x = 5, y = 6;
int a[3] = {1,2,3};
static int internal = 7;
static int read_global(void) { extern int x; return x; }
static int read_pair(void) { extern int x,y; return x+y; }
static int read_array(void) { extern int a[]; return a[2]; }
static int read_internal(void) { extern int internal; return internal; }
static int internal;
int main(void) {
    printf("%d %d %d %d\n",read_global(),read_pair(),read_array(),read_internal());
    return 0;
}
