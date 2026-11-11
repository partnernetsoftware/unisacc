# 0.0.37 g5 (6): every #line refusal printed "error: not covered" and still exited 0 with an image

err_atu (only caller family: the 14 pp.line refusals in linedir) called err_at, which prints but does
not count; nerr stayed 0, so the program was written.  l0.c (`#line 0`): reference rc 0 + image,
product 0.0.36 rc 1.  Parked err-atu-counts.patch: err_atu counts (and is silent while panicking, like
err_tok): l0.c rc 1, no image; `#line` in a header (../g5-header-splice-line/acc2/ml.c) rc 1;
l1.c (covered `#line 50 "x.c"`) still rc 0.
