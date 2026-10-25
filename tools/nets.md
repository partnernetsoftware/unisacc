# Eighteen-net native contracts

Prepare material and run each command separately from the repository root:

```sh
tools/bound 30 sh ./unisacc.com tools/net-material.cx -- "$PWD" "$PWD/out/nets-fresh"
tools/bound 25 sh ./unisacc.com tools/net-material.cx -- --verify "$PWD" "$PWD/out/nets-fresh"
tools/bound 55 sh ./unisacc.com out/nets-fresh/nets.cx -- "$PWD"
tools/bound 55 sh ./unisacc.com tests/net-controls.cx -- "$PWD" "$PWD/out/nets-fresh" "$PWD/out/net-controls-fresh"
```

Output directories must be new. All handwritten CX uses the implicit runtime and
module interfaces. No Python, host C compiler, subprocess-per-key, weight rebuild
or test-result cache is used. The existing native watchdog has its separately
documented host C cold bootstrap.

The material tool expands the existing kernel/unisa_core.c and its quoted model
includes, makes implementation state private, and adds checked scalar functions.
The infer body and model literals are retained. DENSE is carried privately as part
of the source but never read by the contract. Invalid stage/head checks belong to
the adapter; field-domain refusal is tested in the original infer implementation.
This single-instance kernel has mutable workspaces and is not concurrent/reentrant.

Semantic index mappings use the existing model vocabularies and canonical CHARC/
DEFINED tuples read as data from unisa/gold.py. Composite heads reuse the declared
isel/abi vocabularies. Gold labels come from the actual eighteen gold TSV files,
with strict schema, vocabulary, column, duplicate-key and full-domain checks.
They are independent expected-label input to inference; they are not an external
C-language semantics oracle. The additional lexcls/lexword files are not among
the eighteen declared nets.

One default-run batch executes 8,770 keys and 22,694 key/head inferences. Every
winner must match gold and be the unique maximum logit. For each field, both -1
and its upper boundary are tested in every legal context of the other fields,
across all heads, followed by a valid recovery query. This produced 28,868 field
refusals and recoveries, plus 72 adapter refusals, locally. It does not enumerate
infinitely many invalid integers. Optional NAME selects one net for debugging.

Material receipts bind the recursive kernel source, gold, canonical mapping data,
tool, batch and bootstrap product, plus generated module and batch hashes.
Preparation is PREPARED only; --verify regenerates in memory and compares bytes.
The batch itself also checks each gold file's before/after hash. Source inputs
must remain fixed while running; perform material verification before and after
the batch when using it as evidence.

The separate controls intentionally change a label in an isolated gold fixture,
change one model-literal byte in the private prepared copy, and restore the copy.
They require the specific label mismatch, material-identity refusal and successful
restoration; three controls passed. They do not mutate repository gold or kernel
sources. A forced kill during the control can leave the private material invalid;
--verify then refuses it and a new material directory can be prepared.

These results cover the local classic integer-kernel contract. They do not prove
the complete P3 route, stream interfaces, resources, all six native hosts or
release qualification. Internal timing phases remain UNMEASURED. The main --all
entry reports these implemented but unscheduled nets as NOT_RUN until integration.
