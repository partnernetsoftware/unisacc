/* Regression: *"z" must be 122. The product at 4b37a77 returned an
   address; 48a5c4b resets the string literal's type and restores the load.
   E3 now uses its ordinary dereference path against the fixed reference. */
#include <stdio.h>
int f(void) { return *"z"; }
int main(void) { printf("%d\n", f()); return 0; }
