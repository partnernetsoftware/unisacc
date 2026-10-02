# examples/apps: historical six-application evidence (archived 2026-10-02)

The older six application checks below are historical evidence:

Independent takeover check (2026-09-26): all six compile warning-free under
host `cc -std=c99 -Wall -Wextra`. The Linux x86-64 compiler image produced by
the six-delta source-to-ELF route runs every example with `-run`; each exits 0
and its stdout equals host cc byte for byte. Execution was in the local Lima
x86-64 VM, emulated on an arm64 host. The compiler image is the existing C
compiler built through the new route, not an executor-based product switch.
The VM was stopped afterwards. These examples are not yet fixed E3 keep items.

Evidence provenance: product C source `7d50875`, delta generators/runtime
`1775315`; `exec/pipeline/elf.sh` produced
`/tmp/unisacc-self-route/final-pipeline/unisacc.elf` (671,404 B), SHA256
`d8f7586e1d0250064c248cfbb1bfe8ad8dc8021abf6f9f6e86da5d50b58d1979`.
In Lima `minicon-lnx-x86_64`, this image was named `n1`; each application ran
as `timeout 10 ./n1 -run examples/apps/NAME.c`, with stdout compared to the
host cc result. The same image rebuilt unisacc.c twice with
`-O2 -b lnx/x86_64`; N1, N2 and N3 have that same hash. These runtime checks
do not claim that the application sources themselves passed through all six
deltas; they were compiled by the resulting C compiler.
