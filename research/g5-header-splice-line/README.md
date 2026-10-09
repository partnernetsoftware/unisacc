# 0.0.37 g5 (5): continuation lines inside an included header shift later diagnostics

e7.c includes m.h (a 3-line `#define` joined by two backslashes); the error on e7.c:3 is reported as
`m.h:6:13` by the reference and by the product 0.0.36 alike (cc: e7.c:3).  e8.c (the same
macro in the main file) is right (e8.c:5).  zlib: deflate.h has 34 continuation lines, so
trees.c:158 was reported as trees.c:98 / deflate.h:3xx.
Diagnostics only (positions), no wrong code.  splice() records the FILE LINE of each backslash
(N17a) for the main file; the header's joins are not mapped back through the include swap.
Fix: reference front_pp.c splice/diag_at plus the product's position δ; its own slice.

Not diagnostics only (cdx 10-09): `__LINE__` uses the same mapping (line_at), so it is wrong code in the
reference and the product: e10.c (`#include "m.h"` then `__LINE__` on line 3) prints 5; cdx line.c rc 1.

Parked fix header-splice-line.patch (reference): ireg_nl counts the header's lines as the buffer has
them (joined `\`+newline, CRLF too, not counted), and each header's joins (isp_at/isp_reg) map a
position inside the header back to its file line, in both diag_at and line_at.
Acceptance acc/ (main file joins, header joins, nested header sub/n.h, CRLF header cr.h; __LINE__ and
__FILE__): cc lines `15 4 3 3`; fixed reference `15 4 3 3`, rc 0; old reference and product 0.0.36
`11 2 2 2`, rc 1.  __FILE__ spelling: cc says ./h1.h, we say h1.h (implementation-defined; acc
compares ours).  e7 -> e7.c:3, e9 -> m2.h:4, zlib trees.c -> :159 (real line), cdx line.c rc 0.

Review round 2 (cdx): the header-join table refuses when full (`unisacc: too many continuation lines in
headers`, rc 1; big.h with 4100 joins) instead of dropping a record.  acc2/: `#line` inside a header with
joins is refused (position now hl.h:3, old hl.h:2) -- and that refusal exited 0 (see ../g5-line-refusal-rc);
two units a.c+b.c and b.c+a.c both print `5 3` like cc (old reference `3 3`: the first unit's joins
leaked into the second).
