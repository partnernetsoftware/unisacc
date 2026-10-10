# Seal: Abstract decision-network five-target self-host (2026-10-10 ~14:07 SGT)

Paper-A research-only heartbeat Cut A: tighten **abstract** (`abstract.txt` + TeX `\begin{abstract}`) so「the compiler self-hosts…」cannot be misread as whole-compiler / six-platform native self-hosting. Align with already-merged §10 / §5.3: **decision-network instance** does **byte-for-byte self-hosting (N1=N2=N3)** on **five named targets** only (not six platforms; not a semantic self-hosting theorem). Builds on [`paper-a-seal-conclusion-five-vs-six-selfhost-20261010-1358.md`](paper-a-seal-conclusion-five-vs-six-selfhost-20261010-1358.md) and [`seal-bootstrap-bytes-not-semantic-20261009.md`](../seal-bootstrap-bytes-not-semantic-20261009.md). Does **not** change key-count identities (8509 / 8769 / 9174), Table 1/4/5 numbers, product code, `kernel/`, `weights/`, `facts/`, tests, Softguess, or measurement-identity numbers. CN/EN markdown abstracts were already scoped; left unchanged.

## Branch tip (parent of this seal)

`cfb29cb2b55062193a12c781dc9ebb3eede8e343` (`attemptchain --layers (WF5 layer/family targets); R8 COV1 ruling: add coverage`)

## Attack

After the 2026-10-10 ~13:58 §10 five-vs-six pin, **abstract.txt** still said `and the compiler self-hosts byte-for-byte on five targets.` TeX abstract mirrored bare「the compiler self-hosts…」even though it already carried N1=N2=N3 and a semantic-self-hosting negation laundry list. Reviewers can read the subject as the whole shipping compiler (or all six platforms) rather than the decision-network instance under §5.3 named checks.

## Pin (one clause — abstract only)

**abstract.txt:** `and the decision-network instance self-hosts byte-for-byte (N1 = N2 = N3) on five named targets (see §5.3).`

**TeX abstract:** `and the decision-network instance self-hosts byte-for-byte (N1 = N2 = N3) on five named targets---` (existing laundry list after `---` kept; no new BootstrapHost laundry list pasted into abstract.txt)

**CN / EN markdown abstracts:** already scoped（决策网络实例 / This instance … decision-network instance）— **no body edit**.

## Before → after (verbatim quotes)

| Surface | Before | After |
| --- | --- | --- |
| `abstract.txt` | `…, and the compiler self-hosts byte-for-byte on five targets.` | `…, and the decision-network instance self-hosts byte-for-byte (N1 = N2 = N3) on five named targets (see §5.3).` |
| `main.tex` abstract | `…, and the compiler self-hosts byte-for-byte (N1 = N2 = N3) on five targets---` | `…, and the decision-network instance self-hosts byte-for-byte (N1 = N2 = N3) on five named targets---` |

## Files touched

| File | Change |
| --- | --- |
| [`arxiv-paper-a/abstract.txt`](../arxiv-paper-a/abstract.txt) | bare「the compiler self-hosts…」→ decision-network + N1=N2=N3 + five named targets (§5.3) |
| [`arxiv-paper-a/main.tex`](../arxiv-paper-a/main.tex) | abstract clause only (same subject fix; laundry list retained) |
| [`notes/paper-a-seal-abstract-decision-net-five-selfhost-20261010-1407.md`](paper-a-seal-abstract-decision-net-five-selfhost-20261010-1407.md) | this seal |
| [`notes/paper-a-heartbeat-report-20261010-1407.md`](paper-a-heartbeat-report-20261010-1407.md) | short report |
| [`paper-a-notes.md`](../paper-a-notes.md) | one-line heartbeat pointer |
| [`paper-notes-20261009.md`](../paper-notes-20261009.md) | 封口 cross-link pointer |

## Unchanged (explicit)

- CN `research/unisacc-paper.md` abstract body (already 决策网络实例 + N1=N2=N3 + five targets)
- EN `research/unisacc-paper.en.md` abstract body (already decision-network instance / This instance + N1=N2=N3)
- Key counts 8509 / 8769 / 9174; Table 1/4/5 cells
- Product / `kernel/` / `weights/` / `facts/` / tests
- Softguess: NONE
- Submission direction / root claim
- No new ~03:44 decision card

## Bans held

- No six-platform native bootstrap claim in abstract
- No semantic self-hosting theorem claim
- No long BootstrapHost negation laundry list newly pasted into `abstract.txt`
- No key-count / table numeric edits
- Research-only

## Tip / blob SHAs (fill after merge)

| Artifact | git blob SHA |
| --- | --- |
| Branch tip (merge) | _(after merge)_ |
| CN `research/unisacc-paper.md` | `f40070830bc44538ac15c3734dcc8257a8e3d7ee` **SAME** |
| EN `research/unisacc-paper.en.md` | `7878d8072a35438db9653cb21c257b362c7a0665` **SAME** |
| TeX `research/arxiv-paper-a/main.tex` | `eacae1a1dafa8b4d81eaaa4283d73e1792636a48` (was `1bb25b6c91cd7ce8c0bb6ceec5e6c571745982af`) |
| abstract `research/arxiv-paper-a/abstract.txt` | `43d7ea7e7297ed8be9a96e3950e014e359467973` (was `9e8838928ca79ec1708fcfb2df3683548566516f`) |

## OUT OF SCOPE

- §10 / §5.3 body re-edits (already pinned at ~13:58 / earlier)
- Measurement-identity freeze; platform matrix inventory seals
- Product code, tests, kernel, weights, facts
- Closing stale draft PR #31 (handled in heartbeat process, not this file’s claim pin)
