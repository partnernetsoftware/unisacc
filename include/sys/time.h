/* sys/time.h -- 0.0.18 R18-5: struct timeval for programs that include the
 * header (kilo).  No gettimeofday yet: nothing is declared without a body. */
#ifndef _UNISA_SYS_TIME_H
#define _UNISA_SYS_TIME_H
#include <time.h>
struct timeval { long tv_sec; long tv_usec; };
#endif
