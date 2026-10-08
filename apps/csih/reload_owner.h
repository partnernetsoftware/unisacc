/* POSIX process record-lock ownership for participating reload processes.
 * Trusted ancestors, one acquire lifecycle per session in each process.
 * No other code in that process may open the same lock file: closing ANY fd
 * for that file releases the process's record locks. Do not duplicate acquire.
 * fork children do not inherit record locks; exec closes this fd and releases
 * ownership. This API does not promise an atomic handoff across exec and does
 * not protect an older process that has not joined this protocol.
 */
#ifndef CSIH_RELOAD_OWNER_H
#define CSIH_RELOAD_OWNER_H
#include <stddef.h>
int reload_owner_acquire(const char *absolute_lock_path, int *out_fd,
                         char *why, size_t cap);
/* Sets *fd=-1 even on close failure; failure means return of ownership has
 * NOT been confirmed. Never unlinks the persistent lock file. */
int reload_owner_release(int *fd, char *why, size_t cap);
#endif
