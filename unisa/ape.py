"""One file that runs on Linux, macOS and Windows, on x86-64 and arm64. [S-10]

The trick is Cosmopolitan's: the first bytes are read two ways.  Windows sees
`MZ` and a PE header at the offset in e_lfanew; a Unix shell sees
`MZqFpD='...'`, an assignment whose value happens to contain that header, and
then runs the script that follows.  The script picks the slice for the
machine it is on, writes it out and runs it.

So the file is:

    MZ qFpD='....'      <- DOS header; e_lfanew lives inside the quotes
    <the shell script>  <- still the DOS stub, as far as Windows cares
    <PE headers + the win/x86_64 image>
    <the lnx and osx images, one after another>

Windows on arm64 runs the x86-64 PE under its own emulation, which is how
tests/crossnative.sh already exercises that target.
"""
import struct

from .image import pe

# each slice: the shell's name for it, and our target
SLICES = (("Linux|x86_64", "lnx/x86_64"),
          ("Linux|aarch64", "lnx/arm64"),
          ("Darwin|x86_64", "osx/x86_64"),
          ("Darwin|arm64", "osx/arm64"))
PAD = 40                    # slack the script is padded back up to, so its
                            # length does not change when the numbers do


def _script(table):
    """`table`: [(os|arch, offset, length)] -- offsets are 1-based, for tail."""
    cases = []
    for (name, off, ln) in table:
        o, a = name.split("|")
        pat = o + a
        if a == "aarch64":
            pat = o + "aarch64|" + o + "arm64"
        # PLAIN decimal: BSD tail reads a leading-zero count as OCTAL, so
        # a zero-padded offset seeks to the wrong place on macOS
        cases.append('%s) o=%d n=%d;;' % (pat, off, ln))
    return ("\n".join([
        "u=$(uname -s)$(uname -m)",
        "case \"$u\" in",
        "  " + "\n  ".join(cases),
        '  *) echo "unisaccrun: no slice for $u" >&2; exit 1;;',
        "esac",
        't="${TMPDIR:-/tmp}/unisaccrun.$$"',
        'tail -c +$o "$0" | head -c $n > "$t" || exit 1',
        'chmod +x "$t"',
        '"$t" "$@"; r=$?',
        'rm -f "$t"',
        "exit $r",
    ]) + "\n").encode()


def _pad(script, want):
    """Pad the script back to `want` bytes with a comment, so that filling in
    the real offsets cannot move anything that comes after it."""
    room = want - len(script)
    assert room >= 2, (len(script), want)
    return script + b"#" + b"." * (room - 2) + b"\n"


def _stub(script):
    """The DOS stub: an open quote, then the loader's fields, then the script.

    The newline comes RIGHT after the quote, so the file's first line is
    `MZqFpD='` and nothing else -- a shell refuses to run a file whose first
    line holds a NUL, and e_lfanew at 0x3C is full of them.  Those bytes sit
    on the next line, still inside the quotes, where they are only data.
    """
    filler = b"." * 51                   # offsets 9..0x3B; 0x3C..0x3F follow
    return b"qFpD='\n" + filler + b"....'\n" + script


def build(compile_target, out):
    """`compile_target(target, stub=b"")` -> the image for that target.

    Two passes: the first learns how long everything is with a placeholder
    script, the second writes the real offsets.  The script's length cannot
    change between them, which is what the fixed-width fields are for.
    """
    imgs = {t: compile_target(t) for (_, t) in SLICES}
    # pass 1: plausible numbers, padded to a fixed length
    guess = [(name, 1 << 30, len(imgs[t])) for (name, t) in SLICES]
    want = len(_script(guess)) + PAD
    head = compile_target("win/x86_64",
                          stub=_stub(_pad(_script(guess), want)))
    base = (len(head) + 15) // 16 * 16
    off = base
    table = []
    for (name, t) in SLICES:
        table.append((name, off + 1, len(imgs[t])))    # tail -c counts from 1
        off += (len(imgs[t]) + 15) // 16 * 16
    stub = _stub(_pad(_script(table), want))
    head2 = compile_target("win/x86_64", stub=stub)
    assert len(head2) == len(head), (len(head2), len(head))
    blob = bytearray(head2.ljust(base, b"\x00"))
    for (_, t) in SLICES:
        img = imgs[t]
        blob += img
        blob += b"\x00" * ((-len(img)) % 16)
    open(out, "wb").write(bytes(blob))
    return bytes(blob)
