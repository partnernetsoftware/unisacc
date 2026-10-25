/* 0.0.29 H4': a forwarded host function whose address is taken -- sqlite's aSyscall[] table holds
   gettimeofday and sysconf -- is forwarded like a call; it used to go unforwarded and the program
   jumped through code bytes.  And va_arg(ap, T *)->m: va_arg carries T's struct type */
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
typedef void (*fp)(void);
static struct { const char *n; fp f; } tab[] = { {"gettimeofday", (fp)gettimeofday}, {"sysconf", (fp)sysconf} };
typedef struct { long a, b; } Pair;
static long second(int n, ...) { va_list ap; long r; va_start(ap, n); r = va_arg(ap, Pair *)->b; va_end(ap); return r; }
int main(void) {
    Pair p = { 3, 4 };
    struct timeval tv; long (*sc)(int);
    sc = (long (*)(int))tab[1].f;
    printf("%s %d\n", tab[0].n, ((int (*)(struct timeval *, void *))tab[0].f)(&tv, 0));
    printf("%s %d\n", tab[1].n, sc(_SC_PAGESIZE) > 0);
    printf("second %ld\n", second(1, &p));
    return 0;
}
