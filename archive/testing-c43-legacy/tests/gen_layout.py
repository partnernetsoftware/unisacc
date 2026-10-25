#!/usr/bin/env python3
"""Every data layout a small tape can ask for. [A-41] [P-3]

`unisa acc` discharges the model by enumerating its FULL gold -- 6,650 keys,
every one exact.  The compiler's own pure functions deserve the same
treatment, and `zero_last` is one: initialised blobs first, zero blobs after,
each keeping its address mod 8.  It was ported to C by hand, and the port
assumed the symbols arrive in address order.  They do in every probe of the
corpus; they do not in the compiler itself, where the C back end then read
off the end of an array and every target segfaulted.

So the domain is enumerated here instead of sampled.  A tape's data is a
sequence of definitions; the interesting ones are

    kind    .bss (zeros) or .str (bytes)
    length  1, 7 or 8 -- a blob whose length is not a multiple of 8 is the
            only way the `mod 8` rule is visible
    zero    a .str of NUL bytes is stored but still all-zero, which is what
            decides which half of the layout it lands in

over sequences of one, two and three definitions, plus a second pass in
which one symbol is defined TWICE -- the shape that reorders the symbols
with respect to their addresses.

Each tape is written next to a name that says what it holds, so a failure
is a sentence rather than a seed.
"""
import itertools
import os
import sys

# (suffix, the tape line, the blob it makes)
SPECS = [
    ("b1", ".bss %s 1", "zero"),
    ("b8", ".bss %s 8", "zero"),
    ("s1", '.str %s "a"', "data"),
    ("s7", '.str %s "abcdefg"', "data"),
    ("s8", '.str %s "abcdefgh"', "data"),
    ("z8", '.str %s "\\0\\0\\0\\0\\0\\0\\0\\0"', "zero-but-stored"),
]


def tape(defs, redef):
    """A program that names every symbol (so none is dropped) and exits 0.
    `.lea` is what makes a data symbol referenced; the addresses are never
    printed, because the point is the IMAGE's bytes, not the program's."""
    L = ["_start:"]
    for i, _ in enumerate(defs):
        L.append("  .lea r0, d%d" % i)
    L.append("  imm r0, 0")
    L.append("  .exit r0")
    for i, (_, line, _k) in enumerate(defs):
        L.append(line % ("d%d" % i))
    if redef is not None:
        # the same name again, further down: a tentative definition that a
        # later one completes reaches the back end exactly like this
        _s, line, _k = defs[redef]
        L.append(line % ("d%d" % redef))
    return "\n".join(L) + "\n"


def main(out, maxn=3):
    os.makedirs(out, exist_ok=True)
    n = 0
    for k in range(1, maxn + 1):
        for combo in itertools.product(SPECS, repeat=k):
            name = "_".join(c[0] for c in combo)
            open("%s/%s.tape" % (out, name), "w").write(tape(combo, None))
            n += 1
            # redefining the first and the last covers "the repeat is the
            # earliest symbol" and "the repeat is the latest"
            if k > 1:
                for r in (0, k - 1):
                    open("%s/%s.r%d.tape" % (out, name, r), "w").write(
                        tape(combo, r))
                    n += 1
    print(n)


if __name__ == "__main__":
    main(sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 3)
