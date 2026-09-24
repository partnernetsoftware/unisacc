/* <memory.h>: the pre-standard spelling of <string.h>, which glibc and
 * macOS both still ship.  crypto-algorithms includes it, and a header that
 * cannot be found is an error now [C99 6.10.2p4]. */
#ifndef _UNISA_MEMORY_H
#define _UNISA_MEMORY_H
#include <string.h>
#endif
