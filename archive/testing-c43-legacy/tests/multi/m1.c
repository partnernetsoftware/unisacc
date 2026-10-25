/* Unit one.  `hidden` and `helper` are file scope: unit two has its own
   pair with the same spelling and different values, so a program that
   prints 8 309 11 could not have shared them. */
#include <stdio.h>
#include "m.h"

int shared = 10;

static int hidden = 1;

static int helper(int x)
{
    return x + hidden;
}

int one(int x)
{
    return helper(x) * 2;
}

int main(void)
{
    /* Sequenced on purpose: two() mutates `shared`, and the order in which
       printf's arguments are evaluated is unspecified. */
    int a = one(3);
    int b = two(3);
    printf("%d %d %d\n", a, b, shared);
    return 0;
}
