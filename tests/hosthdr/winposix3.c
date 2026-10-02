/* Windows POSIX layer batch 3 (0.0.21): getenv/setenv/unsetenv.
   Portable and deterministic: only names this probe sets are read back,
   so the same probe runs under hosthdr and tests/winposix.sh.  Empty values
   are not probed: Windows' environment block does not keep a NAME= entry. */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main(void) {
    char *v; int r;
    printf("absent %d\n", getenv("UNISA_WP3_NOPE") == 0);
    r = setenv("UNISA_WP3_A", "alpha", 1); v = getenv("UNISA_WP3_A");
    printf("set %d %s\n", r, v ? v : "(null)");
    r = setenv("UNISA_WP3_A", "beta", 0); v = getenv("UNISA_WP3_A");
    printf("noover %d %s\n", r, v ? v : "(null)");
    r = setenv("UNISA_WP3_A", "gamma", 1); v = getenv("UNISA_WP3_A");
    printf("over %d %s\n", r, v ? v : "(null)");
    errno = 0; r = setenv("BAD=NAME", "x", 1); printf("badname %d %d\n", r, errno == EINVAL);
    errno = 0; r = setenv("", "x", 1); printf("emptyname %d %d\n", r, errno == EINVAL);
    r = unsetenv("UNISA_WP3_A"); printf("unset %d %d\n", r, getenv("UNISA_WP3_A") == 0);
    r = unsetenv("UNISA_WP3_A"); printf("unset2 %d\n", r);
    return 0;
}
