# R17-8 independent red probes

These files are outside `tests/c/` until the incorrect route is fixed. Adding
them to the ordinary differential glob now would make unrelated suites red.
The measured route results are in [matrix.tsv](matrix.tsv). The reference
compiler is built from the current source with `tests/build_ref.sh`; the model
product in the snapshot is `unisacc.com` with SHA-256 prefix `6fc72026`.

`structarg_global.c` and `structarg_local.c` print the two bytes of a
string-initialised `struct pair` passed by value. The global and local forms
expose different wrong bytes in the C reference. Replacing `{"12"}` with
`{2, 3}` makes both forms pass, so that simpler initialiser is not a useful
red probe. The existing `tests/c/b_bitfield_edges.c` isolates the Python
front end's post-decrement value error; the C reference and model product
already match the host compiler.

After each fix, require stdout and exit status to match the host compiler,
move the struct probe into `tests/c/`, remove the corresponding matrix debt,
and run the affected `ccrun`, differential and closure suites. A match on one
host alone does not establish all-target ABI behaviour.
