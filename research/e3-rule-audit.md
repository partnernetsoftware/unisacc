# E3 rule-source audit (2026-09-26, after 965c3a4)

This audit asks, for each semantic rule in `exec/parse2/gen2.py`, one question: is the rule read from a gold table (`weights/gold/*.tsv`, produced by `unisa/gold.py`), or is it written by hand in the generator? It answers cdx's convergence review. It is a plan, not an implementation.

## Rules already read from gold

| rule | gold table | how E3 reads it |
|---|---|---|
| operator → IR instruction, signed or unsigned | `binsel`, `irsel` | `E.optext` |
| operator precedence | `prec` | `ladder()` |

## Rules written by hand that a gold table already holds

The product reads `type.tsv` and `tyinfo.tsv` at runtime through `tyask` and `tyax`. E3 re-derives the same facts as hand-written branch ladders.

| rule in gen2 | procedures | gold table that holds it |
|---|---|---|
| result type of `a op b`: promotion, the long/unsigned pairing, compare → int | `OPX.*` `.x1`–`.x4`, `.w*`, `.u`, `.s`, `.l8`/`.i4` | `type` (t1, op, t2 → y), 4,275 keys |
| whether an unsigned int operand needs its operands masked | `UICHK`, `.w6` | `tyinfo.uns` / `tyinfo.size` |
| narrowing on return or cast, and masks on loads | `NARROW`, `NARU`, `width_dispatch` masks | `tyinfo.narrow`, `tyinfo.size` |
| element size and pointer step | `ELSZ`, `SCALE`, `DSCALE`, `STEPTY` | `tyinfo.size` (and the struct layout tables) |
| which operands are legal | `NODBL`, `NOFLT`, `INTONLY`, `DEAD.pa` | `type` → `illegal` |

## Rules written by hand with no gold table

These rules were measured from the reference's output. They stay hand-written until a table exists for them:

- the frame and slot layout;
- the variadic push reversal;
- the struct word copy;
- the `__rv_` return buffer.

## Minimal convergence plan

No third generator and no new framework.

1. Let E3's value descriptor carry the TYS axis index (`tyax`) instead of the pair (vt, vb).
2. Replace `OPX`'s type ladders with one table lookup. The lookup is loaded from `type.tsv` at generation time and keyed on (t1, op, t2). The mask, narrow and size decisions follow from `tyinfo`.
3. Acceptance: `E3KEEP` with the current 162 equal files stays green, and `gen2.py` gets shorter. Count its physical lines before and after.

Honest status: E3 is still a structured table/state-machine prototype, not a net. Lower state counts do not show that maintenance got simpler.
