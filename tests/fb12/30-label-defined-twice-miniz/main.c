/* LAYER (measured on candidate 0.0.13-dev4, 2026-09-30): the arm64 encoder fallback -- the same message as #34.
   dev4 answers `reject: not covered: ARM64 operand or instruction`, **the same
   fallback message fb12-34-arm64-fallback produces**, and with the same missing
   information (no position, no construct name).  So this fixture and 34 are not
   two defects: the label-naming change is what miniz needs, but what the product
   actually reports here is the encoder fallback.  When 34 is fixed this one may
   move too -- check it rather than assuming.
*/
#include "miniz.h"
int main(void){ return (int)mz_crc32(0, 0, 0); }
