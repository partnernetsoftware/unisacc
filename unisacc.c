/* Classic reference/fallback assembly source. Constructed weights live in
 * kernel/weight.<stage>.inc; dense answers in kernel/dense.<stage>.inc.
 * Default product networks are separate .net/P3 data, not these literals.
 * tests/export_ref.sh exports one independent C file for transfer/selfhost. */
#include <stdio.h> /* explicit library dependency of the classic compiler */
#include "src/version.h"
#include "kernel/unisa_model.inc"
#include "kernel/unisa_headers.inc"
#include "kernel/unisa_core.c"
#include "src/front_pp.c"
#include "src/front_parse.c"
#include "src/tapelink.c"
#include "src/asmtext.c"
#include "src/opt.c"
#include "src/main.c"
#include "src/back_lower.c"
#include "src/back_encode.c"
#include "src/tapeprune.c"
#include "src/back_image.c"
#include "src/tapebin.c"
