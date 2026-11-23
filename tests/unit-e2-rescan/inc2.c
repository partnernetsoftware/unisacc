/* 0.0.95 e2 rescan fixture: two nested #include levels.  The saving grows with nesting, so the
 * patched pack must be strictly smaller than the original AND save more than inc1.c does. */
#include "inc1.c"
int rescan_two;
