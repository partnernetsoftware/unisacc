/* context_index.h — read-only context packet for the owned mailbox (see context_index.c).
 * Returns 1 and sets *out to a malloc'd JSON packet (caller frees) on success; 0 on allocation failure. */
#ifndef CONTEXT_INDEX_H
#define CONTEXT_INDEX_H

#include "reload_session.h"

long clock_now_ms(void);

int context_index_packet(const reload_session_state *owned,const char *owned_dir,char **out);

#endif /* CONTEXT_INDEX_H */
