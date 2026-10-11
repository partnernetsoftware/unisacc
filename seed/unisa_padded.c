/* seed/unisa_padded.c -- C99 reference for unisa.padded.Padded (K6-4).
 *
 *   unisa-padded HEX FULL
 *       HEX  : the stored prefix as hex (even length; "" is the empty prefix)
 *       FULL : the requested logical length as a decimal integer
 *   Prints the observable values of Padded(prefix, full) one per line:
 *     LEN <max(FULL, stored)>            len(p)
 *     STORED <len(prefix)>               p.stored()
 *     PREFIX <hex>                       bytes(p)  (stored bytes only)
 *     RSTRIP <len>                       len(p.rstrip(b'\0'))
 *     SLICE <len>                        len(p[:])  (buffer users see the stored prefix)
 *     VIEW <len>                         len(memoryview(p))
 *     ZERO_TAIL <LEN-STORED>             logical zero tail length
 *   Malformed HEX (odd length, non-hex) or FULL (not a decimal integer) is refused by name;
 *   nothing is printed on refusal.  Never allocates the logical length (the point of Padded).
 *
 * Point-sample equivalence only; not a product closure member; keeps unisa/padded.py.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void die(const char *why) { fprintf(stderr, "unisa-padded: %s\n", why); exit(1); }

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int main(int argc, char **argv) {
    const char *hex, *fulls;
    size_t hn, stored, i, rlen;
    unsigned char *pre;
    long long full, len;
    char *end = NULL;
    if (argc != 3) { fprintf(stderr, "usage: unisa-padded HEX FULL\n"); return 2; }
    hex = argv[1]; fulls = argv[2];
    hn = strlen(hex);
    if (hn % 2) die("odd-length hex prefix");
    stored = hn / 2;
    pre = malloc(stored ? stored : 1);
    if (!pre) die("out of memory");
    for (i = 0; i < stored; i++) {
        int a = hexval(hex[2 * i]), b = hexval(hex[2 * i + 1]);
        if (a < 0 || b < 0) { free(pre); die("non-hex digit in prefix"); }
        pre[i] = (unsigned char)(a * 16 + b);
    }
    errno = 0;
    if (fulls[0] == 0) { free(pre); die("FULL is empty"); }
    full = strtoll(fulls, &end, 10);
    if (errno || end == fulls || *end != 0) { free(pre); die("FULL is not a decimal integer"); }
    /* Padded.__init__: self.full = max(full, stored) */
    len = full > (long long)stored ? full : (long long)stored;
    /* rstrip(b'\0'): trailing NUL bytes of the stored prefix */
    rlen = stored;
    while (rlen > 0 && pre[rlen - 1] == 0) rlen--;
    printf("LEN %lld\n", len);
    printf("STORED %zu\n", stored);
    printf("PREFIX ");
    for (i = 0; i < stored; i++) printf("%02x", pre[i]);
    printf("\n");
    printf("RSTRIP %zu\n", rlen);
    printf("SLICE %zu\n", stored);
    printf("VIEW %zu\n", stored);
    printf("ZERO_TAIL %lld\n", len - (long long)stored);
    free(pre);
    return 0;
}
