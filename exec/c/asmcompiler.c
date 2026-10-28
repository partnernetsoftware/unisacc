/* Development product entry: language decisions come from carried networks.
   The transitional C run loop calls the CX transition unit directly. */
/* The shipped driver: its step budget is the build constant in compiler.c, never a host variable.
   Test builds (run.c standalone, unisacc.c as the reference) do not define this, so they still read
   UNISA_MAXSTEPS (plan step 1, 0.0.43). */
#define UNISA_PRODUCT_BUILD 1
#include "compiler.c"
