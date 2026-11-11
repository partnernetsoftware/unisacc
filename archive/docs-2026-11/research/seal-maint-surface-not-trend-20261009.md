# Seal maintenance-surface snapshot ≠ trend theorem 2026-10-09

Paper-A heartbeat evidence for **§8 «表示与维护成本» / «Representation versus maintenance cost»**. **No equal-coverage before/after proof.** Table 1 key counts (8509 / 8769 / 9174), Table 4/5 cells, product code, weights bytes, and facts were **not** edited.

## Question

Can tip file-type line counts be read as proof that the **maintenance surface has shrunk** after moving control into tables—or as a monotonic trend? Paper A §8 already says representation migration does not automatically reduce maintained language logic; [`paper-a-notes.md`](paper-a-notes.md) still lists **维护面统计** (equal-coverage column trend) as open.

## Tip measured

`ded65f53bbbfacff74854de7219aa471842ef2a0` (`git rev-parse HEAD` on main after `git pull origin main`).

## Method (commands run on tip)

```bash
git rev-parse HEAD

# Shipped truth tables (data)
find weights/gold -name '*.tsv' | wc -l
wc -l weights/gold/*.tsv

# Declaration / fact tables under exec (not shipped gold)
find exec -name '*.tsv' | wc -l
find exec -name '*.tsv' -exec wc -l {} +

# Stage constructors (generators)
find exec -name '*.py' | wc -l
find exec -name '*.py' -exec wc -l {} +

# Offline constructor orchestration (seed Python)
find unisa -name '*.py' | wc -l
find unisa -name '*.py' -exec wc -l {} +

# Handwritten compiler paths in product tree
find src -name '*.c' | wc -l
find src -name '*.c' -exec wc -l {} +

# Embedded headers (e.g. syscall constants moved out of tables — anecdote in §8, not counted as trend)
find include -name '*.h' | wc -l
wc -l include/*.h

# Build-time control still in Python (read-only ledger; sites not lines)
python3 tests/decisionledger.py
python3 tests/decisionledger.py --seedpy
```

`facts/` has **no** `*.tsv` at tip (`find facts -name '*.tsv' | wc -l` → 0). There is no top-level `table/` TSV tree at tip.

## Category totals obtained (tip)

| Category | Count | Notes |
| --- | ---: | --- |
| `weights/gold/*.tsv` | 20 files, **9174** lines (`wc -l` total) | Shipped gold tables; key-domain identity 8769 vs 8509 is a **separate** sealed measurement ([`seal-remeasure-20261009.md`](seal-remeasure-20261009.md)), not a maintenance trend |
| `exec/**/*.tsv` | 927 files, **40902** lines | Declarations / facts / stage inputs under `exec/` (top-heavy: `parse2` 387, `facts` 213, `enc` 101, …) |
| `exec/**/*.py` | 119 files, **13590** lines | Generators + checks; includes tools excluded from some ledger caps |
| `unisa/**/*.py` | 48 files, **15318** lines | Offline constructor / gold / build orchestration |
| `src/**/*.c` | 12 files, **17979** lines | Handwritten compiler implementation paths |
| `include/*.h` | 64 files, **6591** lines | Embedded C library / constants |
| `tests/decisionledger.py` | **9** direct transition sites across **2** stages (`enc` 1, `parse2` 8); baseline 10 | Counts **sites**, not lines; see also `--ops` (9 distinct) |
| `tests/decisionledger.py --seedpy` | **1** stage-specific file under cap 1; `assemble.py` **420** lines (cap 420) | Manifest assembler; 60 check/sim tools listed, not counted |

Machine-readable twin: [`seal-maint-surface-not-trend-20261009.json`](seal-maint-surface-not-trend-20261009.json).

## Judgment

- This inventory is a **snapshot ledger** at one commit: file/line totals and ledger **sites** label where maintenance work lives today (tables, generators, constructor, C paths, headers, residual Python control).
- It is **not** an equal-coverage before/after comparison (no v0.0.10 ledger replay, no declared-rules vs legacy parity slice in this pass).
- **Paper A does not claim** the maintenance surface has already shrunk; §8’s syscall/header anecdote illustrates representation cost, not a proved maintenance reduction.
- **Equal-coverage trend** (`.py` / `.c` / `.tsv` columns vs same behavioural coverage) stays an **open obligation** in [`paper-a-notes.md`](paper-a-notes.md) (**维护面统计**).

## Self-check

- `research/unisacc-paper.md` / `.en.md` Table 1, Table 4, Table 5: **not edited** for counts in this change (one aligned §8 sentence only).
- Product / `kernel/` / `weights/` / `facts/` / `src/` code: **not edited**.
- No invented numbers; every figure in this note came from the commands above on tip `ded65f53`.
