/* include/cx.h -- the .cx prelude (edition 1; plans/cx-modules.md CX1).  The source is
 * include/cx-nested.h.  unisacc reads include/cx-flat.h, the same text with every #include inlined
 * (written by tools/cx-flatten.cx): E2 swaps its whole buffer once per #include, so the nested form
 * costs about 1.7 s of E2 per .cx unit and the flat one about 0.1 s (B1,
 * plans/perf-bottlenecks-2026-10-30.md).  Host compilers (gcc_cx, clang_cx, -include include/cx.h)
 * read the nested source.  After editing any header the prelude reaches, rerun
 * `sh ./unisacc.com tools/cx-flatten.cx -- --write .`; `--check .` names a stale flat file. */
#ifdef __UNISA__
#include "cx-flat.h"
#else
#include "cx-nested.h"
#endif
