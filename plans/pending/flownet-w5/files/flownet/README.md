# flownet

flownet is the flow x net paradigm as a `.cx` module: finite gold tables become integer nets, a consumer's flows ask
those nets every decision, and the bytes come out of one emit hook. It knows no language and no target. Everything
domain-specific (contracts, gold tables, flows, the emit hook) belongs to the consumer, sits in the consumer's own
directory, and is handed in.

Requirement and history: `plans/m4-flownet-cx-module.md` (waves W0 to W6, criteria D1 to D6, red lines).
Moving from the Python m4 package: [`docs/flownet-migration.md`](../docs/flownet-migration.md).

## The API lives in API.md

The export surface, its version (CXIF v1) and, per function, the semantics, the error codes and the lifecycle are
written once, in [`API.md`](API.md) (W6 B2). This file does not repeat them. Until API.md lands, the header comment
of `api.cx` is the reference. Rules that hold for the whole surface:

- One entry point: `#pragma unisa use flownet.api`. A consumer outside this directory reaches the modules through
  links in its own directory, one per module (`tests/flownet/*.cx` are such links).
- Functions only, every name prefixed `fn_` (W0: an unresolved external object reads as zero, an unresolved
  function is refused by name, so no object is exported).
- New entries are appended; an existing entry's meaning or bytes never change silently (section 9, B2/B3).

## Modules

| module | role |
|---|---|
| `fnx.cx` | the base: the one formatter (text allocated at its exact length, never cut short), the one failure sink, the injection types |
| `contract.cx` | the contract: nets, flows and the decision nets of each flow, reserved and observed nets, choices, book phases |
| `nets.cx` | integer nets: read from a table, or constructed from gold (exact by construction), and the forward pass |
| `flow.cx` | asks, books, gates, the trace check, the run (and its test hooks: pinned books, the sweep) |
| `api.cx` | the one public face; nothing else is meant to be called from outside |

## The injection contract (what a consumer supplies, what flownet promises)

A consumer supplies four things, all through the API:

1. **A contract**: which nets exist, which flow asks which net, which nets are reserved or observed, the book
   phases. As a directory it names or as contract text.
2. **A label source** (gold): for each net, its domain (the field rows and the class row of a gold table's header)
   and the label of each key. flownet asks every key exactly once, at construction, keeps the answers as the
   net's gold, and builds the integer net from them. The label source is never consulted while flows run.
3. **Flows**: the consumer's run. Every decision is an ask; the run writes its result with `fn_out`.
4. **An emit hook**: what to do with the whole output of a run (write a file, run an assembler, judge it).

flownet promises:

- Every constructed net is exact on every key of its domain (`fn_acc` re-checks; construction refuses otherwise).
- An ask whose key is outside the net's domain is refused by name; it is never answered from anywhere else.
- No silent gold fallback (red line 1): a run whose books show any gold fallback fails the trace check, and a run
  that is refused or fails its check never reaches the emit hook. The hook is called once, after the books are clean.
- If the consumer pins its books (`fn_expect`), a run that asks a net more, less, or in another phase is refused
  before the hook. Pinned books prove the nets are asked; that the answers reach the bytes is proven by the sweep
  test hook (`FLOWNET_SWEEP`) together with a per-perturbation record that the consumer's gate keeps.

## Errors

Every failure is a named refusal: the call returns `-1` (int) or `0` (pointer), and `fn_error()` gives one line,
`PATH[:LINE]: reason` for a file, or what was asked and why it was refused. The first failure since the last clear
wins, so the line names the cause, not a consequence. There is no partial result: a refused construction leaves no
nets open, a refused run never calls the emit hook. Hard limits are counts only (nets, flows, phases, choices,
fields, columns), each refused at its PATH:LINE; text is never truncated. The per-function codes are in API.md.

A consumer's own command line follows the same shape (D5): a refusal is exit status 1, nothing on stdout, no output
file, and one stderr line naming the input, the reason and, where there is one, the position.

## Coverage (W5, red line 2)

The 28-program corpus is not full coverage. `tests/flownet-coverage.tsv` lists every program of the repository the
first consumer's language can be given, in three states, covered / refused / untested, each with its class, and
the gate keeps it exact:

- `flownet-w5-cover` (no Python): every program built through flownet again, equal to the record; a program whose
  bytes differ from the reference, or that flownet builds while the reference refuses it, is red, never a state.
- `flownet-w5-oracle`: the reference verdicts that the W1 baseline does not already hold, measured again.

Measured at W5 (2026-10-23): 47 programs, 39 covered (byte-identical), 8 refused (4 outside the language, refused
by both; 4 refused by the external assembler on WAT identical to the reference's), 24 untested (copies embedded in
demo pages, each matched to a covered twin), 1 alias. Details and the classes: `docs/flownet-migration.md`.

## Gates

Every flownet gate step is a `tests/gate.sh` job named `flownet-*` (`./tests/gate.sh --list | grep flownet`), each
bounded at 58 s. Mechanism only (W6 B1, `flownet-w6-b1`): this directory holds `.cx` code and `.md` documentation,
no tables, and names no consumer.
