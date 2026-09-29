/* args: hello, with the command line echoed back.
 *
 * Prints argc and every argv[i]; with no arguments it is the smallest
 * useful check that a compiled program starts, sees its arguments and
 * exits 0.  Arguments after `--` are passed to the program under -run.
 * Deterministic on every target (argv[0] is whatever the loader passes).
 *
 *   unisacc -run examples/apps/args.c
 *   unisacc -run examples/apps/args.c -- one "two words" 3
 */
#include <stdio.h>

int main(int argc, char **argv)
{
    int i;
    printf("Hello from unisacc! argc=%d\n", argc);
    for (i = 1; i < argc; i++) printf("  argv[%d] = %s\n", i, argv[i]);
    return 0;
}
