# exec/ — E0: a generic delta executor on a toy delta

Milestone E0 of prd.md S-17. `exec.c` runs the machine of
research/delta-framework.md §2 with option A primitives (§3); it knows no
language. `toy/` is the §4 LL(1) expression grammar as a postfix translator.

    ./exec/check.sh    # build with cc and unisacc, T1 table check, 29 cases vs toy/ref.py
    ./exec/ledger.sh   # sizes and speed (after check.sh)

## Machine

C = (q, r, i, s, W, o, h). One step: obs = (q, r, b, t) with b = x[i] or 256
(EOF), t = stack top or NG (⊥); (q', acts) = δ(obs); q := q'; r := 0; run the
actions in order, stopping at the first halt. Accept: o on stdout, exit 0.
Reject(k): `reject k at i` on stderr, no stdout, exit 1. Bad table: exit 2.
Storage is static and bounded (`MAX*` in exec.c); running out is exit 3
`exhausted`, never a reject.

## Actions (code, params)

| code | name | params | effect |
|---|---|---|---|
| 0 | ACC | – | h := accept |
| 1 | REJ | k | h := reject(k) |
| 2 | ADV | – | i := i+1 if i < \|x\| |
| 3 | PUSH | g | push g (0 ≤ g < NG) |
| 4 | POP | – | pop; empty stack: reject(254) |

> The E0 table above is the 19-action toy machine of exec.c. The machine E1-E3 run on has 56 actions; its authority is the header of `exec/pp/sim.py`, and `exec/c/run.c` implements the same. There, a POP on an empty stack is a bad-table error on both sides, not a reject (2026-09-26, bdy review).
| 5 | EMIT | c | append byte c |
| 6 | COPY | – | append x[i]; at EOF: reject(253) |
| 7 | SETR | v | r := v (0 ≤ v < NR) |
| 8 | LDI | k v | W[k] := v |
| 9 | BYTE | k | W[k] := x[i] or 256 |
| 10 | ALU | op d a b | W[d] := W[a] op W[b], u32 wrap; op 0 add 1 sub 2 mul 3 divu (÷0 → 0) 4 remu (÷0 → a) 5 and 6 or 7 xor 8 shl 9 shr (shift by b&31) |
| 11 | CMP | a b | r := 0/1/2 for W[a] <,=,> W[b] (unsigned) |
| 12 | LOAD | d k | W[d] := W[W[k]] |
| 13 | STORE | k v | W[W[k]] := W[v] |
| 14 | OUTW | k | append W[k] & 255 |
| 15 | GETI | k | W[k] := i |
| 16 | SETI | k | i := min(W[k], \|x\|) |
| 17 | SPAN | a b | append x[W[a] .. min(W[b], \|x\|)) (nothing if W[a] ≥ that) |
| 18 | SETRW | k | r := W[k]; W[k] ≥ NR: reject(252) |

ALU op 10 is ltu: W[a] < W[b] (unsigned) as 0/1.  Codes 15-18 and ALU op
10 were added for E1 (exec/lex/tbl.py lowers the E1 lexer delta's actions to
these); like the rest they name positions, bytes and integers, no language.

W is a dictionary u32 → u32, missing keys read 0. NR ≥ 3 (CMP's codes).

## Table format (text, decimal integers, `#` to end of line is a comment)

    NQ NR NG q0
    <default entry>
    nrows
    q r b t <entry>        x nrows; -1 in q/r/b/t = any value
    entry := q' n a1 p.. a2 p.. ... an p..   (n actions, each code + its params)

Every observation starts at the default entry; rows are applied in order,
later rows overriding earlier ones. The loader expands this to the dense map
over the whole domain NQ×NR×257×(NG+1), stored factored: state q is indexed
only by the components (r, b, t) that some row for q names -- a component no
row names cannot change q's entries.  `exec -fdump` prints the factored cells
(-1 = unread); `exec -dump` prints the whole domain, and
`toy/gen.py check` compares it line for line with `delta_ref` (T1 for the toy).

## Toy

`toy/gen.py` derives Q (entry states + one item per grammar position, 34),
Γ (the return points after non-tail calls, 6) and δ from the grammar
table in it; `toy/toy.tbl` is its output (107 rows). Reject codes: 1 no F
alternative, 2 `)` expected, 3 trailing input. `toy/ref.py` is the
hand-written recursive-descent reference.

Not done in E0: the table is not built as an integer net through the
`unisa build-weights` route; T1 here is the enumeration check only.

## E1: the lexer delta on this executor

    exec/lex/run.sh gen; exec/lex/run.sh xbuild          # table, cc + unisacc builds, T1
    E1EXEC=/tmp/e1x/exec_ua:/tmp/e1x/e1.tbl exec/lex/run.sh lexdiff   # (probes, self, corpus.*)
    exec/lex/ledger.sh                                   # __text, table bytes, speed
