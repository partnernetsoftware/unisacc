# iterate/kernel -- J10 step 3, first and second slices

`genmodel.c` writes part of `kernel/unisa_model.inc` from declared inputs, in
the C subset unisacc compiles. The parts are the `S_*` defines, `MODEL`, `NSTAGE`
and the `STAGE_*`/`act`/`z` declarations, `DENSE`, `DENSE_LEN`,
`STAGE_DOFF/NH` and `model_dims()`.
The second slice adds the 15 TSV vocabularies (`TOKV`, `PRODV`, `ACTV`, `DIRV`,
`PPACTV`, `NTV`, `TYSV`, `TOPSV`, `TYOUTV`, `SCTXV`, `SKINDV`, `SACTV`, `IRFAMV`,
`IRFLAV`, `IRRECV`) and `BF_*`/`BH_*`/`HD_*` for 11 stages (see "Second slice"
below). The provenance header, `TYPEV` (lex.py TYPEKW) and `ENC_*` are still
written by Python and are out of scope.

    genmodel -o OUT order.tsv vocab.tsv weights/built.uns2 weights/gold/<18 stages>.tsv

## Inputs

genmodel reads only the inputs named on the command line. It opens two paths
(`fopen(path, "rb")` on an argument and `fopen(opath, "wb")`), and check.sh
counts those call sites. It never reads `built.json`, `kernel/`, `gold.py` or
anything relative to the working directory.

- **order.tsv**: the declared stage order. It holds `stage<TAB>name` lines in
  stage-ID order (`S_<NAME>` = index) and `heads_max<TAB>16`, the stride of
  `STAGE_NCLS`. There must be exactly 18 stage lines and no repeats.
  `order_check.py` asserts that it equals `gold.ALL` and `ckernel.HEADS_MAX`.
  **Transition:** `gold.ALL` is still what the Python side uses and what this
  file is compared against. order.tsv is not yet the only ordering authority.
- **built.uns2**: decoded with a bounds check before every read. The header
  checks are magic, version 1, flags 0, pad 0, nStages 18 and nUnits = the sum
  of H. Per section it checks the NUL-padded name (no repeats), pad,
  `h0 = sum(fieldLens)`, `W1 bytes = ceil(H*h0/8)` with zero padding bits,
  and per head `ncls >= 1` and `1 <= wBits <= 16`. Each payload must be used
  exactly: its byte length is `ceil(bits read / 8)`, with zero padding and
  `class < ncls`. No bytes may follow the last section.
- **gold TSVs**: the same reader contract as `iterate/construct` (schema, header,
  values, classes, every key exactly once). Each TSV is identified by its
  `# stage NAME:` line, not by its file name or argument position.

Stages are matched **by name**. Every order.tsv stage must be exactly one UNS2
section and exactly one TSV, and all three lists hold 18 names without repeats.
If so, the three name sets are equal. Then UNS2 and the TSV must agree on the
number of fields, the values per field, the number of heads and the classes per
head. Nothing is inferred by position.

## Outputs, as `ckernel.py` writes them

- `MODEL`: sections in order.tsv order. For each unit and field there are `MW`
  little-endian u64 masks (`MW = max(1, ceil(len/64))` over the stage's
  fields), and a full mask is written 0. Then come `u32 nEntries` and the
  7-byte entries `(unit u16, head u8, class u16, w u16)`, ordered by head,
  then unit, then row: the UNS2 payload order, which is `_blob`'s.
- `DENSE`: one byte per stage, key and head. Keys run in field-major order (the
  last field fastest, which is `S.keys()`), and heads are the minor index.
  Each byte is the index of **the TSV label** in that head's `#head` class list.
  It is taken straight from the TSV rows, never from `infer()`.

Exit codes: 1 bad input (reader, UNS2 decode, name or dimension mismatch),
2 usage, 4 capacity (fields > 4, H > 4096, classes > 512 or 256 in DENSE, ...),
7 the output could not be opened, written or closed (short `fwrite`, `fclose`
failure). Nothing is written unless every check passes.

## check.sh

`iterate/kernel/check.sh [ua]` runs from the repo root, with `ua` defaulting to
`/tmp/ua_ref`. `CHECKS="..."` selects checks and `--batches` runs six batches
of at most 60 s each. Each step is bounded with `alarm`. A check gets
`receipt <check>` only when all of its `need` marks are present. If a selected
check has no receipt, the run fails.

| check | what it proves |
|---|---|
| order | order.tsv = gold.ALL and HEADS_MAX (`order_check.py`); genmodel.c names no stage |
| build | genmodel built with cc -O2, unisacc -O2 (osx/arm64), cc -fsanitize=undefined,address; each runs on the declared inputs |
| region | the Python **oracle**: `python3 -m unisa emit-kernel --out <scratch>` run at the same commit. Its region equals the shipped one, and genmodel's region is byte-identical to it in all three builds |
| empty | genmodel run with its cwd set to a directory that holds only `order.tsv built.uns2 gold/` (no `kernel/`, no `built.json`). The output equals the repo-root run (cc, ua) |
| sem | a **MIXED** model.inc: genmodel's region spliced into the shipped file, which supplies the header, vocab, BF/BH and ENC. It is built with the **real** `kernel/unisa_core.c` (only its `#include` lines are removed) and `sem.c`. For every key and head, `infer()` must equal the TSV label via DENSE and the max logit must be unique. Built with cc and with unisacc |
| oracle | `tests/build_ref.sh` builds the whole compiler, in a scratch tree, over the MIXED model.inc. Then `--check-oracle` runs |
| neg | 14 broken **copies**, each of which must exit 1 with its diagnostic and write no output, in cc, ua and san |
| wfail | output is a directory or a path in a missing directory (the open fails), or a real short write under `ulimit -f 1` with SIGXFSZ ignored. Each must exit 7 in cc, ua and san |
| label | one label changed in a temp copy of prec.tsv, with UNS2 unchanged. genmodel's output changes in exactly one DENSE line, and the semantic check REJECTS it (1 wrong) in cc and ua |
| perm | order.tsv with its stages reversed. The region is byte-identical to `permoracle.py`, which is emit_core with `ckernel.ALL` permuted the same way in a scratch process, and it differs from the unpermuted region. `S_*` = the permuted positions, and sem passes |
| fault | `CHECKS=order SKIP=order`: the check's body is dropped and nothing inside it fails. The run must still exit non-zero, because `order` has no receipt |
| vocab | `vocab_check.py`: vocab.tsv equals ckernel.py's tuples (second slice) |
| region2 | `vregion.awk`: the oracle's vocab/BF/BH region equals the shipped one, and genmodel's is byte-identical to it in cc, ua and san |
| vneg | 17 broken inputs (vocab.tsv or a TSV), each rc 1 with its diagnostic and no output, in cc, ua and san |
| vpos | a non-first vocab value starting 8 or 9 is accepted; cc reads it back from the emitted TOKV |
| pool | two different renames in the first-loaded (enc) and last-loaded (reloc) TSV both survive; exactly those two lines change |
| rename | opinfo `add64` -> `addq`: the changed symbols are predicted from the schema and vocab.tsv, and match exactly |
| integ | the MIXED file equals the shipped one modulo comments; `tests/snap.sh` closure and nativeboot over it |

The region (`region.awk`) runs from the first `#define S_` line to the `}`
that closes `int model_dims(void) {`. Whole-line `/* ... */` comment blocks
are removed, because genmodel's comments name its own inputs. Everything
else, including blank lines, is compared byte for byte.

## Results (2026-09-25)

- region: 204,496 B and 2,514 lines. Oracle = shipped, and cc, ua and san are all identical.
- sem, MIXED model.inc with the real kernel: 18 stages, 8,484 keys and 20,184
  (key, head) decisions, with 0 wrong, 0 non-unique and 0 layout errors (cc and unisacc).
- `--check-oracle` on the compiler built over the MIXED file: 20,184 questions and 0 differences.
- negatives: 14 × 3 builds; wfail: 3 × 3 builds. The short write is reported
  as `short write, 1024 of 205057 B`, including by the unisacc build.
- timings: the full run takes 10.8 s; `--batches` takes 3.7, 4.2, 4.2 and 4.0 s.

## Not done here (recorded)

- **Provenance header:** not implemented. genmodel writes its own short comment
  header, which is not compared. The shipped header lists the sha256 prefixes of
  `gold.py`, `catalog.py`, `lex.py`, `built.json` and `ckernel.py`, and the
  MIXED file keeps it unchanged. That header therefore still names
  `built.json`, which genmodel does not read. The header's format is still
  undecided (prd J10: reproduce the Python sha256 in C, or change the format).
- The MIXED file is **not** free of Python. Its header, vocabularies, BF/BH and
  ENC still come from `emit-kernel`.
- `sem.c` indexes `STAGE_NCLS[s * 16 + h]` as the kernel does (`<< 4`). A
  heads_max other than 16 would pass genmodel's checks but not the kernel's.

## heads_max contract (after review of 0197f45)

The real kernel indexes `STAGE_NCLS[(s << 4) + head]` (kernel/unisa_core.c,
inf): the per-stage stride is 16, fixed in the kernel's code.  heads_max in
order.tsv is therefore that stride, not a free choice.  genmodel refuses any
other value as soon as the number is read -- before the capacity check, before
any layout, before the output is opened -- with exit 8 and "heads_max N, the
kernel's STAGE_NCLS stride is 16" (constant KERNEL_HEADS).

check.sh (neg): heads_max 15 and 17 are refused with exit 8, that diagnostic
and no output file, on cc, unisacc and UBSan builds (17 used to hit the
capacity check first with exit 4; the contract check now runs before it).
`neg stride` confirms that the kernel source still contains
`STAGE_NCLS[(s << 4) + head]`, that genmodel defines KERNEL_HEADS 16 and that
order.tsv says heads_max 16 -- so the constant cannot go stale silently if the
kernel changes its indexing (a copy of the kernel with `<< 5` does not match).
The kernel layout itself was not changed.

## Second slice: vocab, BF/BH/HD (2026-09-25)

- **vocab.tsv** holds the mapping and the order: 15 `vocab SYM stage field|head name`
  rows, 1 `typekw TYPEV` placeholder row, 11 `bfbh stage` rows. genmodel's
  fixed symbol list is a CONSUMER ABI check (the symbols the kernel links
  against must be declared), not a second mapping. `vocab_check.py` asserts
  that vocab.tsv equals ckernel.py's tuples (transition).
- **Ordering contract:** rows are written in vocab.tsv order. Moving a row
  moves its lines, and vregion.awk does not assume that TOKV comes first.
- **TYPEV** is not generated. genmodel writes a delimited placeholder
  (`/* TYPEV: BEGIN placeholder ...` / `/* TYPEV: END placeholder */`). Only
  `mix.awk`, the scratch integrator, splices the old TYPEV block in, and it
  finds it by markers. It needs no TYPEKW input (check `empty typev`).
- **String pool:** every schema string of every TSV is copied into pool[],
  with its length and the pool's capacity checked before the copy, because
  tbuf is reused for each file.
- **Symbols:** every name written (first slice, vocab + N*, BF_/NBF_,
  BH_/NBH_/HD_) must be a legal C identifier (keywords rejected) and must not
  collide with any other name under case folding. Exit 1 on failure.
- **This tool's conservative input domain** (exit 1, no escaping is done): a
  quote, a backslash, a byte outside 0x20-0x7e, "??", an empty value, or a
  non-first vocab value that starts 0-7 (it would extend `\0`). 8 and 9 are
  accepted. BF/BH use `\000`, so a leading digit is fine there. This describes
  genmodel's accepted inputs; it does not say that Python writes broken C.
- **vregion.awk:** the start anchor is the `}` that closes model_dims (the
  opening line must appear exactly once). The end anchor is the first
  `/* ENC_` (py) or genmodel's END line, which must appear exactly once.
  Comments are removed, and so are ONLY the exact `char *TYPEV = "...";` and
  `#define NTYPEV n` lines. After that the region must be non-empty and its
  symbols must be exactly the 156 expected (15x2 vocab, 21x2 BF, 28x3 BH/HD).
- Results: region 12,661 B / 222 lines, identical for the oracle, the shipped
  file, cc, ua and san. The MIXED file equals the shipped one modulo comments.
  --check-oracle shows 0 differences in 20,184. closure 582/0 and nativeboot
  pass. The six --batches take 4.2 / 4.5 / 4.9 / 4.7 / 6.5 / 36.2 s.
