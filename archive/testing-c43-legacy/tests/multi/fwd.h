/* A header whose statics call one another FORWARD: `vfmt` is defined first
   and calls `digits`, which is defined below it.  The header is included by
   ONE unit, and the point of the fixture is that it is not the first one --
   see fwd1.c / fwd2.c.  [R13-0b, multi gate] */
#ifndef FWD_H
#define FWD_H
static int digits(void) { return 7; }
static int vfmt(void) { return digits() + 1; }
static int both(void) { return digits() + vfmt(); }
#endif
