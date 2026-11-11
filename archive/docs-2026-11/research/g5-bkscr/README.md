# 0.0.37 g5 (4): bk_assemble sizing overran bkscr[8192]

The sizing pass encodes each instruction into bkscr[8192] to measure it.  An x86 `.zero` of
3246080 bytes unrolls to 6426624 bytes of stores and wrote past bkscr into bkz_at
(first difference: i=3205 `.zero a=1,0,3246080`, bkz_at[0] 0 -> 0xf2a00630d28e0110),
reported later as "back end: data gap".

Parked fix bkscr-bound.patch: sizing stores only within bkscr and still counts every byte.
Independent of the unsized-rows fix (which removes this trigger in zlib): with ONLY this patch the
reduced zlib case (research/../g5r rd2 ct.c + a bss unit) links (rc 0, 6.5 MB image, the
oversized table still wrong until unsized-rows lands).
Open, not in this slice: a large `.zero` unrolls (6.4 MB of text for a 3 MB clear).

Boundary (cdx review): boundary.tsv -- volatile char a[N] = {1} with sizing lengths 8187..8204 on
osx/arm64 (4-byte words: 8188/8192/8196/8204) and osx/x86_64 (8187/8193/8194/8200; 8191/8192 are not
reachable by this op there).  Old and patched reference images are byte-identical in every row and every
program prints N (all bytes past [0] zero).  No in-memory sentinel was run: the patch stores nothing past
bkscr by construction (`bkol < 8192` guard).  The data-gap rc 0 proves linking only, not the
oversized array's meaning (that is unsized-rows.patch).
