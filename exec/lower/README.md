# Lowering through the generic executor

Current scope: Linux data layout, directly from raw E3/E4 tape text.

```
python3 exec/lower/gen.py data.json
python3 exec/c/tbl.py data.json data.tbl
run data.tbl input.tape > data-and-tape.txt
```

The delta decodes `.str`/`.bss`, aligns and reorders data using the existing
zero-last policy, updates symbol addresses, and allocates scratch space.
It emits the full data header followed by still-unlowered tape instructions.
Do **not** send that output straight to the target encoder: register mapping,
entry setup, syscall rules and push/pop fusion remain to be implemented.

`data.py` holds the hand algorithm; the generated artifact is a transition
table, not an integer network. The runtime executor is unchanged. Escape and
scratch-size declarations are imported at generation, not the Python layout
algorithm. `check.sh` uses Python parsing/layout only as the test referee,
and feeds real E4 output into the delta.
