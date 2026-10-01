# C99 seed constructor experiments

`net.c` is a standalone C99 implementation of the current flat-table to
threshold-network step. Build it with any C99 compiler:

```sh
cc -std=c99 -O2 -I. -o seed-net seed/net.c
./seed-net stage.tbl stage.net
```

The input is the `T/S/Q/R` table produced by `exec/c/tbl.py`; the output is
the `N/S/Q/H` network currently produced by `exec/c/net.py`. It reuses the
runtime's declared action arities in `exec/c/core.h`. It is not yet wired into
`buildcompiler.sh`, and generating the table, P3 package and APE seed still
requires Python. `tests/seedconstructcheck.py` checks bytes against Python.
