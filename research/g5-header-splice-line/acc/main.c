#include <stdio.h>
#include <string.h>
#include "h1.h"
#include "cr.h"
#define LONGM(x) \
    ((x) + \
     2)
int main(void) {
    int bad = 0;
    bad |= __LINE__ != 10;
    bad |= h1_line != 4 || strcmp(h1_file, "h1.h") != 0;
    bad |= n_line != 3 || strcmp(n_file, "sub/n.h") != 0;
    bad |= cr_line != 3;
    bad |= strcmp(__FILE__, "main.c") != 0;
    printf("%d %d %d %d %s %s %s\n", __LINE__, h1_line, n_line, cr_line, h1_file, n_file, __FILE__);
    return bad;
}
