/* <time.h> [C99 7.23]: time_t, clock_t, CLOCKS_PER_SEC, time, clock and
   difftime together.  difftime of two time() calls made back to back is
   0.0 or a small positive number of seconds, never negative. */
#include <time.h>
#include <stdio.h>
int main(void) {
    time_t a = time(0), b;
    clock_t c = clock();
    double d;
    b = a;
    d = difftime(b, a);
    printf("difftime same instant %s\n", d == 0.0 ? "0" : "nonzero");
    printf("difftime ordered %s\n", difftime(time(0), a) >= 0.0 ? "ok" : "negative");
    printf("CLOCKS_PER_SEC positive %d\n", CLOCKS_PER_SEC > 0);
    printf("sizes %d %d\n", (int)sizeof(time_t) >= 4, (int)sizeof(clock_t) >= 4);
    return (d == 0.0 && a != (time_t)-1 && c != (clock_t)-1) ? 0 : 1;
}
