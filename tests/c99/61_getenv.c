/* getenv [S-15 D2]: the environment is where a Unix kernel leaves it, after
   argv's NULL, and __argv(k) walks past argc to reach it.  (Windows: NULL.) */
#include <stdio.h>
#include <stdlib.h>
int main(void) {
    char *v; char *p;
    v = getenv("UNISA_GE_PROBE"); if (v) return 3;
    p = getenv("PATH");
    printf("path set %d  missing %d  prefix-only %d\n",
           p != 0, getenv("UNISA_GE_NO_SUCH") == 0, getenv("UNISA_GE") == 0);
    return 0;
}
