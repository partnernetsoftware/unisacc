/* The pruning reference must pass unsupported tape through unchanged.
   This small host harness lets the tape-reader gate observe that contract. */
#include <stdio.h>
#include "../src/tapeprune.c"

static char input[tp_SRCMAX + 1];

int main(void) {
    size_t n = fread(input, 1, sizeof(input), stdin);
    char *output;
    if (ferror(stdin) || n == sizeof(input)) return 2;
    output = tp_prune(input, (int)n);
    if (putchar(output == input ? 'P' : 'R') == EOF) return 2;
    if (fwrite(output, 1, (size_t)tp_prune_length, stdout) != (size_t)tp_prune_length) return 2;
    return ferror(stdout) ? 2 : 0;
}
