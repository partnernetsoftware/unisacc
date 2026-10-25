/* Unit two.  It calls nothing of unit one's, but it writes unit one's
   global -- there is no linker, so the address has to come out of the same
   symbol table. */
#include "m.h"

extern int shared;

static int hidden = 100;

static int helper(int x)
{
    return x + hidden;
}

int two(int x)
{
    shared = shared + 1;
    return helper(x) * 3;
}
