/* The unit WITHOUT the header.  It comes FIRST in one run and SECOND in the
   other; the defect was order-dependent: registering a static only when its
   definition was reached left a forward reference in the header's first
   function spelled without the unit suffix, so `n1 n2` passed and `n2 n1`
   reported `undefined function 'digits'`. */
int two(void) { return 2; }
