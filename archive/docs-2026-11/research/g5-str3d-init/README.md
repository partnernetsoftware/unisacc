# 0.0.37 g5 (3): string in a 3-D char array -- reference accepted wrong code

str3d.c: cc rc 0; reference rc 1 (wrong bytes, with or without the unsized-rows patch);
product 0.0.36 refuses exactly: `not covered: string initializer outside a two-dimensional character row`
(exec/parse2/initializers-result.tsv IL.rowunsupported).  str2d.c (2-D rows) is right everywhere.

Parked fix str3d-refuse.patch: the reference refuses the same shape (UNCOVERED ref.parse
init.rowunsupported, rc 1, both declarations reported); str2d.c still rc 0.  Its own slice,
independent of unsized-rows.patch and of the bkscr bound.
