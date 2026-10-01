# R20-2 structured rejection record: format and samples

Status legend: **[ran]** observed 2026-10-02 with `unisacc.com` (product, 2 Oct 01:12) and `/tmp/ua_ref`
(reference, 1 Oct 22:42; per memory, may be from an uncommitted tree). **[design]** proposal only.

## 1. How a refusal is reported today [ran / read]

- Executor (`exec/c/core.c:360`): a `REJECT` op sets `result->reason` (a string-table entry) and status 1.
  `exec/c/run.c:470` prints `reject: <reason>` with **no position, no key, no stage**.
  `exec/pipeline/simrun.py` prints `REJECT <k>` then the diagnostic; `exec/pipeline/run.py:151-160`
  strips `REJECT `, and classifies by the prefix `not covered`. `exec/c/chain.sh:76` matches `"reject: not covered"*`.
- Front-end refusals (shared with the reference, `src/front_pp.c:818`, `err_at`/`err_tok`) give
  `file:line:col: error: not covered: <construct>` plus a caret line. These carry position but no stage/key.
- Three inconsistent shapes thus exist: `reject: ...` (table δ, positionless), `file:l:c: error: not covered: ...`
  (front end), and plain errors (`unknown identifier`) that are really coverage gaps.
- `tests/combo.py:73-86`: the product passes iff rc 0 and `-S` tape equals the reference; else if
  `"not covered" in stderr` it counts "refused-by-name" and prints the **last** stderr line; otherwise FAIL
  ("misleading refusal" = first stderr line). No first-miss localisation; no comparison of the reference's
  view of the same point.

## 2. Record format [design]

One line on stderr, tab-separated, fixed field order, always prefixed so it is greppable and
independent of the human diagnostic (which stays, on the following lines):

```
UNCOVERED\t<stage>\t<key>\t<pos>\t<file>:<line>:<col>\t<construct>\t<reason>
```

| field | content |
|---|---|
| `stage` | stage name exactly as in `exec/pipeline/stages.tsv` (front-end refusals use `pp`, `parse`, `sema`) |
| `key` | the table key that missed, rendered as the stage's tape symbols, `,`-joined; `-` if the refusal is structural (no table lookup) |
| `pos` | first-miss position: 0-based byte offset into that stage's **input tape** (makes the record comparable across product and reference independent of source mapping) |
| `file:line:col` | source position mapped back via the line table (1-based, col as today's carets) |
| `construct` | innermost enclosing construct from a closed vocabulary (`decl.vla`, `expr.call.ret-struct`, `pp.line`, `type.struct.incomplete`, `type.complex`, ...); vocabulary lives in a data file shared by both routes |
| `reason` | the existing `not covered: ...` text without the prefix |

Escaping: fields are bytes; `\`, TAB, LF, CR and bytes <0x20 or >=0x7f are written `\\`, `\t`, `\n`, `\r`, `\xHH`.
Nothing else is escaped. Exit code stays 1. Only the **first** miss is recorded (the executor stops there).

Where the reference emits the matching frame: at the same decision points, i.e. every `err_at`/`err_tok`
call whose message is a coverage refusal in `src/` (front_pp.c, front_parse.c), and — the new part — wherever
the reference *accepts* a key the product table lacks, the reference must be able to say which (stage,key) it
used. Proposal: a reference flag `-trace-keys=STAGE` writing `KEY\t<stage>\t<key>\t<pos>` lines; the gate then
asserts the product's `UNCOVERED` key is absent from the δ table and present in the reference trace (a true
coverage gap) or absent from both (a common refusal: records must be byte-identical). Emit both via one helper
`report_uncovered(stage, key, pos, construct, reason)` so the two routes cannot drift in format.

## 3. Samples (inputs in `tests/combo/r20-2/`)

Current messages are **[ran]**; expected records are **[design]** (key/pos values are placeholders `<k>`/`<p>`
where they require the table encoding, which only the δ implementation can fix).

| file | product today (rc) | reference today | expected record |
|---|---|---|---|
| `a_line_escape.c` | `a_line_escape.c:1:7: error: not covered: this form of #line (...)` (1) | same text (bound reported rc 0 under -S; verify) | `UNCOVERED\tpp\t-\t<p>\ta_line_escape.c:1:7\tpp.line\tthis form of #line (C99 6.10.4: digit-sequence ["file"], in the main file)` — identical on both routes |
| `b_vla_inner.c` | `b_vla_inner.c:1:34: error: not covered: nonconstant bound` (1) | `b_vla_inner.c:1:34: error: a constant is required here` (1) | `UNCOVERED\tparse\t-\t<p>\tb_vla_inner.c:1:34\tdecl.vla\tnonconstant bound` on both; today's **text already diverges**, so a byte-compare would catch it |
| `c_struct_ret.c` | `reject: not covered: struct return expression outside local lvalue` (1), no position | accepts (0) | `UNCOVERED\t<stage>\t<k>\t<p>\tc_struct_ret.c:3:29\texpr.call.ret-struct\tstruct return expression outside local lvalue` — reference trace must show the key `<k>` it used: genuine gap |
| `e_complex.c` | `e_complex.c:1:16: error: unknown identifier` (1) | same (1) | `UNCOVERED\tparse\t-\t<p>\te_complex.c:1:16\ttype.complex\t_Complex` — today's misleading diagnostic on both routes; combo would FAIL it once records are required |
| `f_sigaction.c` | `f_sigaction.c:2:33: error: not covered: incomplete struct` (1) | accepts (0) | `UNCOVERED\tsema\t-\t<p>\tf_sigaction.c:2:33\ttype.struct.incomplete\tincomplete struct (sigaction)` — the record should name the tag; product/reference header disagreement |

Tried and dropped (both accept now) [ran]: compound literal address, designated bitfield init,
`FILE *` param with `%.4f` (R19-6② appears fixed in this build).

## 4. Changes to `tests/combo.py` [design]

1. Parse stderr for the first line starting `UNCOVERED\t`; split into the 7 fields; a refusal with rc!=0 and
   **no** record is FAIL ("unstructured refusal"), replacing the `"not covered" in err` substring test (line 79).
2. Print the record (not `splitlines()[-1]`) as the first failing point: `UNS <name> <stage> <file:l:c> <construct>`.
3. Run the reference with `-trace-keys=<stage>`; classify: key in ref trace → `GAP` (counted, listed by
   (stage,construct) for prioritising); both refuse → require byte-equal records, else FAIL.
4. For `rc==0` but tape differs, report the first differing tape byte offset with the reference's enclosing
   construct (same `construct` vocabulary), so tape diffs also localise.
5. Add a fixed list mode: `combo.py UA COM --files tests/combo/r20-2/*.c` alongside the generated grid,
   with expected records in a sibling `.uncovered` file each, so the samples above become gate rows.
