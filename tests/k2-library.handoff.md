# K2 block 2 (library* + layoutfacts) — handoff, round 1 (2026-10-03)

Round 1 = analysis only; no .py converted yet, graphhash not run (nothing changed).

## Call tree (all rooted in gen2.py:919 and gen2.py:~923)
- gen2 -> libraryexports.install(E,P,b,start,TYINT) -> returns 'LX.version'
  - librarytypes.install(E,P,b,integers,UNION)
    - librarycallbackgraph.install(E,P,b,frame)      frame = callback from librarytypes
    - librarytypesv3.install(E,P,b,frame,blob)        frame/blob callbacks; reads g.st (assert)
  - libraryimports.install(...) -> imports_start (-> librarydata)
  - libraryvariadic.install(E,P,b,integers)
  - librarycallables.install(E,P,b,imports_start) -> callable_start  (imports libraryvariadic.REQUESTS)
  - librarymodule.install(E,P,callable_start) -> module_start
  - modelinput.u64 (decoder chain, other agent) x2
- gen2 -> layoutfacts.install (install root exec/, not parse2)
- gen2.addr uses librarydata.global_address (shared with gen2-helper agent: move to a submanifest)

## Why the leaves cannot be swapped alone
Every leaf is installed by a parent .py and receives `frame`/`blob` callbacks or `b` (gen2's bank dict).
Translator refusals confirm (nested install, return values, g.st reads). So conversion must be top-down
in one go for the librarytypes subtree, or the callbacks must be precomputed first:
1. frame('STX'/'LDX') and sigframe are STATIC action lists (regs x ALUI/STX) -> json facts
   (exec/facts/librarytypes.tsv `frame_STX`, `frame_LDX`; librarycallbackgraph `child_STX/LDX`),
   referenced as seq cells `frame.STX=frame_STX`. Generate via exec/facts/export.py.
2. `b` ints, E.ARR, gen2.DIM, LF.* ints, libraryexports.* ints, MEMBERRANK -> facts (export.py),
   bank-overlap asserts (libraryexports RANK_BANKS, callbackgraph range assert) -> export.py --check.
3. librarytypesv3 `assert 'L3.original.X' not in g.st` -> manifest `assert-absent` row.
4. fresh tables (*-fresh.tsv) -> `fresh` op / `holder` rows with opts.cols; section order must be preserved
   (allocation order is part of graphhash).
5. Then librarytypes-manifest.tsv = rows head / template ints / rows mid / call librarycallbackgraph /
   rows tail / call librarytypesv3; libraryexports calls it with `call librarytypes`.
6. Return values (imports_start, callable_start, module_start, 'LX.version') -> `result=` names;
   gen2 consumes libraryexports' result via assemble.run env.
7. Move libraryvariadic.REQUESTS and librarydata.global_address out before deleting those .py.

## Next round
Start with step 1+2 for librarytypes/librarycallbackgraph/librarytypesv3 (one commit: facts),
then write librarytypes-manifest.tsv by hand, swap the call in libraryexports.py to assemble.run,
graphhash --only exec/parse2, delete the three .py if identical.
