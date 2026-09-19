/* Minimal <stdio.h> for the unisa C subset.
 * printf is desugared by the walker against its static format string [W-9];
 * the rest maps onto the `.sys` gate. */
#ifndef _UNISA_STDIO_H
#define _UNISA_STDIO_H
#include <stddef.h>
#define NULL 0
#define EOF (0-1)
#define stdin 0
#define stdout 1
#define stderr 2
int printf();
#endif
