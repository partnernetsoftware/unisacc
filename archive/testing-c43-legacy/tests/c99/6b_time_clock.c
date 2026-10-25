/* time and clock [C99 7.23.2.4, 7.23.2.1]: both return a value other than
   their failure value.  The values themselves are not printed. */
#include <time.h>
#include <stdio.h>
int main(void) {
    time_t t = time(0);
    clock_t c = clock();
    printf("time %s, clock %s\n", t != (time_t)-1 ? "ok" : "failed", c != (clock_t)-1 ? "ok" : "failed");
    return (t != (time_t)-1 && c != (clock_t)-1) ? 0 : 1;
}
