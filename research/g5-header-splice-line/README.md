# 0.0.37 g5 (5): continuation lines inside an included header shift later diagnostics

e7.c includes m.h (a 3-line `#define` joined by two backslashes); the error on e7.c:3 is reported as
`m.h:6:13` by the reference and by the product 0.0.36 alike (cc: e7.c:3).  e8.c (the same
macro in the main file) is right (e8.c:5).  zlib: deflate.h has 34 continuation lines, so
trees.c:158 was reported as trees.c:98 / deflate.h:3xx.
Diagnostics only (positions), no wrong code.  splice() records the FILE LINE of each backslash
(N17a) for the main file; the header's joins are not mapped back through the include swap.
Fix: reference front_pp.c splice/diag_at plus the product's position δ; its own slice.
