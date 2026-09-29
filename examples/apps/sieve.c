/* sieve: the sieve of Eratosthenes over a malloc'd byte array.
 *
 * Counts the primes up to N (default 1,000,000; the first argument after
 * `--` overrides it), printing the first fifteen and the largest.  Exercises
 * long arithmetic, malloc/memset/free and atol.  Deterministic on every
 * target.
 *
 *   unisacc -run examples/apps/sieve.c
 *   unisacc -run examples/apps/sieve.c -- 100
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    long n = argc > 1 ? atol(argv[1]) : 1000000;
    long i, j, count = 0, last = 0;
    char *comp;
    if (n < 2) { fprintf(stderr, "usage: sieve [N >= 2]\n"); return 2; }
    comp = malloc(n + 1);
    if (!comp) { fprintf(stderr, "sieve: out of memory\n"); return 1; }
    memset(comp, 0, n + 1);
    for (i = 2; i * i <= n; i++)
        if (!comp[i])
            for (j = i * i; j <= n; j += i) comp[j] = 1;
    printf("first primes:");
    for (i = 2; i <= n; i++)
        if (!comp[i]) {
            if (count < 15) printf(" %ld", i);
            count++;
            last = i;
        }
    printf("\nprimes <= %ld: %ld (largest %ld)\n", n, count, last);
    free(comp);
    return 0;
}
