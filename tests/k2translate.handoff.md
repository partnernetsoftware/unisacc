# K2 trace translator — handoff (2026-10-03, after round 6)

Copied from the agent scratch .k2tmp/handoff.md. Scratch helpers (.k2tmp/rec2.sh, emit.py, swap2.py, *.modes) are not committed; rec2.sh = for each line of a modes file run `tests/bound.py 55 python3 tests/k2translate.py record DIR m<i> GEN <line> -- MODULES` writing DIR/m<i>.flags; emit.py = run `emit MODULE flags=DIR/<stem>.m<i>.json...`.

## State
- tests/k2translate.py: `record OUTDIR TAG GEN [ARGS] -- MODULE...` (one generator run per mode, all modules wrapped),
  `emit MODULE FLAGS=TRACE...` (FLAGS comma list, `-` none). Helper scripts in .k2tmp/: rec2.sh (record per mode file),
  emit.py (emit over DIR/m*.flags), swap2.py (replace `from X import install as N; N(...)` in a caller by assemble.run),
  bo.modes (gen2 8 modes), sf.modes (pp), enc.modes, lex.modes, units.modes.
- Translated + deleted so far: pp/sourcefacts, pp/locations, parse2/booleans, conditional, bitfields, enumtypes,
  forward, statics, printfallback, vla, functiontypes (template rewritten to $state/$original/$hook), floatconst.
  truth.install translated (truth-manifest.tsv) but truth.py stays (conversions used by gen2).
- exec/assemble.py: one change by me: opts `domain_keys` (fact path to explicit key list).
- Refusals: tests/k2translate.refusals.tsv (file -> reason).

## Translator rules (emit)
- binding strings -> `@str:` cells (P4); facts only hold data; state names inside template facts -> `$k2L_n` params
  bound with @str (whole-cell use only; partial use like `X.{state}` needs template rewrite by hand, see functiontypes).
- fresh labels: first use -> `fresh:U:cur:kind` + export; later -> `$name`; re-bound export name before use -> refuse.
  Fresh labels in template facts -> `$k2F_n` params (new, round 6; first-use order must match allocation).
- Each install call is a segment (`["seg", args]`); multiple calls (phases) -> STEM-<phase>-manifest.tsv each,
  caller must pass env returned by the previous assemble.run (not yet exercised end to end).
- bool args are mode flags; when-merge via difflib + flag cube (never exercised: all traces were mode invariant).
- facts file never overwrites a non-translator file: uses k2-<stem>.tsv.

## Round 7 (slice A, parse2)
- gen2.addr -> exec/parse2/addr-manifest.tsv (hand-written): env in `entry` (state) + `pending` (acts), out `done`.
  Fresh order kept by `fresh:U:{entry}:K` cells (prefix = entry's first segment, same counter); librarydata
  global_address called as a `template` row (facts librarydata, `let` rebinds entry/classic/done as lists);
  emit(P(auto),'addr') became helpers-result section `address-auto` (one PRN edge) + auto_tail `@out:\n  sub64...`
  (template split leaves the leading \n in the tail). assemble.Run.fresh now interps `{NAME}` from env.
  gen2.addr is a 3-line shim calling the manifest; callers in callcontrol/unarycontrol should use
  `call addr` with bind entry=...,pending=... and read `$done` from the returned env (call op returns sub env as result).
- Next: printf (part0..4 + strwalk/fmtwalk helpers + strings.wide_hooks) and structured_control (big binding
  dict of gen2 constants -> needs a k2-control facts file; control-fresh/text/reject/stack/classes tsvs already exist).

## Next (coordinator decision: callbacks must NOT stay in the driver; gen2 itself will disappear)
1. Turn gen2's helper callbacks into sub-manifests first: addr (gen2.py `def addr`, uses librarydata.global_address
   and emit), structured_control/compound (`def structured_control`), printf (`def printf`, wraps printfcontrol +
   printfallback manifest). Each becomes STEM-manifest.tsv; label returns become `result=`/export names.
2. Then callers use `call` rows with bind maps: callcontrol (two phases begin/finish -> callcontrol-begin/-finish
   manifests, env exported from begin passed to finish), unarycontrol.
3. Record helpers by wrapping them like modules (the recorder wraps `install`; for gen2 helpers, add a wrapper by
   function name or move each helper into its own module first).
4. library*: librarytypes/libraryvariadic (caller libraryexports.py), librarydata (caller libraryimports.py);
   move REQUESTS (libraryvariadic, used by librarycallables) and global_address (librarydata, used by gen2 addr)
   before deleting.
5. membercontrol: wait for cdx's slice; 49 rows after folding.

## Gotchas
- Wrap every generator/graphhash run in the scratchpad slot.sh with GRAPHHASH_JOBS=1; gen2 ~25 s per mode, so a
  gen2 batch is ~200 s: run in background. A concurrent emit can push one gen2 run past the 55 s bound (exit 142);
  rerun before calling it red.
- Don't edit tests/k2translate.py while a record batch runs (each run re-imports it; mixed traces).
- zsh: an unmatched glob aborts the whole command (rm with globs); the worktree guard rejects compound commands
  containing `git` with loops/heredocs — put Python in .k2tmp/*.py files.
- json facts are written raw (assemble json.loads without unescape); str facts are escaped.
- Recorder guards: direct graph writes (g.on/state/seq outside finite_rules) and nested stage-module installs refuse.
