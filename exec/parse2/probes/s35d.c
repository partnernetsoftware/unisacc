/* reference suspected wrong: *"z" must be 122 (cc prints 122); unisacc.com at 4b37a77 prints an address (699646247).
   E3 rejects it as not covered -- an isolation of the reference defect, not a language rule */
#include <stdio.h>
int f(void) { return *"z"; }
int main(void) { printf("%d\n", f()); return 0; }
