/* sys/un.h -- 0.0.21 item 11 (dsh): AF_UNIX socket addresses, in the host
 * kernel's layout (macOS: length byte + 104-byte path; Linux: 108). */
#ifndef _UNISA_SYS_UN_H
#define _UNISA_SYS_UN_H
#include <sys/socket.h>
#ifdef __APPLE__
struct sockaddr_un { unsigned char sun_len; unsigned char sun_family; char sun_path[104]; };
#else
struct sockaddr_un { sa_family_t sun_family; char sun_path[108]; };
#endif
#endif
