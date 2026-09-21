/* `strlen` with no <string.h>: the driver notices the undefined function and
   appends the header that defines it.  Our headers define their functions
   `static`, and statics are renamed per file, so the definition has to reach
   THIS unit -- not just the last one. */
int tail(const char *s) { return (int)strlen(s); }
