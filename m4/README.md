# m4 data

The m4 oracle (a Python package: gold -> IntNet -> four flows -> `\0asm`, over the ujs Python front end) is archived
in `archive/reference-py/m4/` with its original README.  Its .cx port is the flownet path of
`adapter/ujswasm/ujswasm.cx`.  What stays here is data that adapter reads:

- `neg/` -- an m4 negative program (`neg_path` falls back to `m4/neg/NAME`).
- `probes/` -- the m4 probe programs.

`ujswasm accepts ROOT REFDIR` compares flownet's bytes with oracle bytes recorded in REFDIR; how to record them with
the archived oracle is in `archive/reference-py/README.md` (section m4).
