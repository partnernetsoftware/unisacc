# 0.0.37 C2 g5 (2): unsized outer dimension counted scalars, not rows

The reference sized `T a[][N] = {...}` by the number of SCALARS in the initialiser
(initcountat) times the row: `{{1,2,3}}` for `unsigned u[][4]` gave 3 rows (sizeof 48, cc 16).
C99 6.7.8p22: a braced row or a string fills one row, loose scalars fill elements.
The released product (0.0.36 .com) is right (16 16 24 on sz3.c); only the reference differs.

zlib crc32.h `crc_braid_big_table[][256]` (7 rows) became 1585 rows: the clear
`@mem.zero r1, 0, 3246080` encodes ~6.4 MB in bk_assemble's sizing pass into
`bkscr[8192]` (back_encode.c), overrunning into bkz_at -> "back end: data gap" when another
unit has bss; alone, `sizeof` is wrong and the program crashes (SIGBUS).

First difference: bk_assemble sizing, i=3205 `.zero a=1,0,3246080` bkol=6426624 (bkz_at[0] 0 -> garbage).

Fix (unsized-rows.patch, parked): initrowsat/initrowsof count rows; the three unsized `[]`
sites recompute n after dimtail for non-struct elements.  Checks (cc / fixed / old):
- sz3.c `16 16 24` / same / `48 16 36`
- rows.c (strings, flat, mixed, designator, 3-D, parenthesised and comma exprs, block scope, static
  local): all sizes and values equal to cc; old: 6 of 8 sizes wrong
- with typedef-reset.patch too: zlib 1.3.1 (16 units + test/example.c, -DZ_HAVE_UNISTD_H)
  builds and its output equals cc's.
Open separately: the bkscr bound (a large `.zero` should not unroll into scratch).

Revision (cdx review 10-09): a string no longer fills a whole outer row.  In a char array it
consumes its innermost subarray (sper = decldim3 or the row), for pointer elements one element.
sp.c (cdx probe: `const char *p[][2]={"a","b"}`, `char s[][2][4]={"a","b"}`): cc rc 0, fixed rc 0,
old rc 1.  sp2.c sizes `32 16 9 24 12` equal cc (old `48 24 9 24 18`); rows.c, sz3.c, zlib example
still equal cc.  Separate, pre-existing (old reference too): sp2.c VALUES of a string in a char
[][2][N] subarray are wrong (`gs[1][0]` empty, `ls[1][0]` "I"; cc "c" "c") -- initialisation, not
sizing; to fix or refuse exactly in its own slice.
