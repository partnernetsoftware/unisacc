/* R13-0 #06 P2 -- stdin is `((FILE *)0)`.
   Expected with the input `hello\n`: `read: hello` and `stdin == NULL: 0`.
   Got on 0.0.12 (df8cc9b4): `cannot open input`, because the ordinary
   `FILE *in = argc > 1 ? fopen(...) : stdin; if (!in) ...` treats the
   sentinel-free stdin as a failure.  Recommended by the report: map fd 0 to a
   non-NULL sentinel.  This suite gives every probe no stdin, so the expected
   output here is the stdin==NULL report alone, plus exit 0. */
#include <stdio.h>
int main(int argc, char **argv) {
    FILE *in = argc > 1 ? fopen(argv[1], "r") : stdin;
    char line[64];
    if (!in) { printf("cannot open input\n"); return 1; }
    if (fgets(line, sizeof line, in)) printf("read: %s", line);
    printf("stdin == NULL: %d\n", stdin == NULL);
    return 0;
}
