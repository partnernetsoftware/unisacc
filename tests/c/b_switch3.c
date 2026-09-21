/* `continue` belongs to the enclosing loop, never to a switch [C99 6.8.6.2p1].
   A switch used to push its own exit as the continue target too, so the rest
   of the loop body ran after a `continue` that was supposed to skip it. */
#include <stdio.h>

int main(void)
{
    int i, j, hits = 0, skips = 0, inner = 0;

    for (i = 0; i < 6; i++) {
        switch (i % 3) {
        case 0:
            skips = skips + 1;
            continue;
        case 1:
            hits = hits + 1;
            break;
        default:
            hits = hits + 10;
        }
        hits = hits + 100;
    }

    /* a loop INSIDE a switch: break belongs to the loop */
    switch (2) {
    case 2:
        for (j = 0; j < 4; j++) {
            if (j == 2) break;
            inner = inner + 1;
        }
        inner = inner + 1000;
        break;
    default:
        inner = 0 - 1;
    }

    /* a switch with no enclosing loop at all */
    switch (i) {
    case 6:  hits = hits + 5; break;
    default: hits = hits - 5;
    }

    printf("%d %d %d\n", hits, skips, inner);
    return 0;
}
