#include <stdlib.h>
const char *peek(void) { const char *v = getenv("H1PROBE"); return v ? v : "(none)"; }
