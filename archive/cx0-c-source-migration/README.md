# Retired C-to-CX adapter template

`plan.cx.in` is the former wrapper used when `src/cxmod.c` maintained the parser
and `tools/cx0-extract.cx` generated the complete CX module. It is retained as
historical input and has no active generation role.

The maintained implementation is now `cx/cx0/plan.cx`, an actual module used by
CX0 consumers. The tool projects its marked private parser body, plus the small
`cx/cx0/compat-prefix.c.in`, into `src/cxmod.c`. Run the tool with `--check` to
verify exact compatibility bytes. The live C projection remains in `src/`
because the installed product, classic reference and host compiler drivers
still read it. This archive does not retire those consumers or change their
runtime behavior.
