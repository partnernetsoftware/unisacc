# E3 rule-source audit (2026-09-26, after 965c3a4)

This audit asks, for each semantic rule in `exec/parse2/gen2.py`, one question: is the rule read from a gold table (`weights/gold/*.tsv`, produced by `unisa/gold.py`), or is it written by hand in the generator? It answers cdx's convergence review. It is a plan, not an implementation.

## Rules already read from gold

| rule | gold table | how E3 reads it |
|---|---|---|
| operator → IR instruction, signed or unsigned | `binsel`, `irsel` | `E.optext` |
| operator precedence | `prec` | `ladder()` |

## What the gold type tables can and cannot replace (narrowed after cdx review)

- **`type.tsv` gives the result type only.** A comparison's result is i32, and that says nothing about the common type the operands are compared in. The product asks two separate questions (`src/front_parse.c` `binary()`, about line 2632): the conversion row `ck = tyask(lax, "+", rax)` and the result row `res`. E3 needs both. One result-type lookup cannot delete the conversion rules.
- **`tyax` folds every pointer into `ptr`.** It cannot stand in for:
  - the pointer depth,
  - the pointee type or size,
  - array dimensions,
  - struct identity,
  - lvalue position.

  `tyinfo(ptr).size = 8` is the pointer's width, not the element size. `tyinfo(struct).size` is not a struct's layout. So the value descriptor keeps (vt, vb, rank/dims, sid), and a `tyax` class is only an added field.
- **`type.tsv` rules on one finite abstraction.** It does not hold every C constraint: `i64 < void` gives i32 there. So `illegal` does not replace the context checks. NODBL and NOFLT are coverage limits of this prototype, not language illegality, and they stay.

## Minimal slice (the only step planned)

1. In `OPX`, take the arithmetic common type (the `ck` row) and the result type (the `res` row) from `type.tsv`, as the product does. Keep the pointer and struct information and every not-covered verdict. Size, step and narrowing are judged separately, later, by whether the table carries enough information for them. They are not removed in this step.
2. **Acceptance:**
   - the fixed list of 162 equal files at the current reference commit;
   - the independent value probes stay;
   - list the special branches actually deleted and the rules added;
   - check that no equal amount of logic moved into `gen.py` or a table script;
   - report JSON size and step changes separately.

   A lower `gen2.py` line count is only a secondary sign.

Honest status: E3 is still a structured table/state-machine prototype, not a net.
