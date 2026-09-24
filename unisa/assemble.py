"""TargetProgram -> machine code.

Three passes, because a real `.lea` needs a real address: size the code, ask the
image module where the text and the data will land, then encode.  Instruction
sizes must not depend on the address, so address-materialising sequences are
emitted at a fixed width.
"""
from . import emit_x86, emit_arm, image
from .tape import DATA_BASE

BACKEND = {"x86_64": emit_x86, "arm64": emit_arm}


def assemble(tp):
    be = BACKEND[tp.arch]
    # Branch relaxation [S-10 #1].  Every branch starts in its long form;
    # each round lays the code out, then marks -- all at once -- every
    # branch whose SHORT form would reach its target from where it now
    # sits, and the round repeats until nothing more fits.  Shortening only
    # ever brings code closer together, so a branch that fits keeps
    # fitting: the rounds terminate, and the set they end with does not
    # depend on the order branches are looked at.  unisacc_back.c's
    # bk_assemble runs the same rounds, and closure checks the two agree
    # byte for byte.
    # Sizes are measured ONCE, in the long form; a round only subtracts what
    # each newly short branch saves and re-sums the offsets -- re-encoding
    # every instruction per round would give back the speed the assembler
    # has, on a program the size of the compiler.
    sizes = [be.size(ins, tp.labels) for ins in tp.code]
    cand = [(pc, be.short_size(ins), be.branch_target(ins))
            for pc, ins in enumerate(tp.code) if be.short_size(ins) is not None]
    short = set()
    while True:
        off, offs = 0, []
        for n in sizes:
            offs.append(off)
            off += n
        end = off
        labels = {name: offs[pc] if pc < len(offs) else end
                  for name, pc in tp.labels.items()}
        fits = [(pc, n) for pc, n, tgt in cand
                if pc not in short and
                -128 <= labels[tgt] - (offs[pc] + n) <= 127]
        if not fits:
            break
        for pc, n in fits:
            short.add(pc)
            sizes[pc] = n
    text_va, data_va = image.layout(tp.os, tp.arch, end)
    imps = image.imports(tp.os, tp.arch, end)
    # interpreter addresses are DATA_BASE-relative; shift them onto the image
    shift = data_va - DATA_BASE
    out = bytearray()
    covered = 0
    for pc, ins in enumerate(tp.code):
        b = be.encode(ins, offs[pc], labels, tp.arch, tp.syms,
                      shift, text_va, imps, pc in short)
        if b is None:
            b = be.UD2 if tp.arch == "x86_64" else be.BRK
        else:
            covered += 1
        out.extend(b)
    return bytes(out), {"insns": len(tp.code), "encoded": covered,
                        "bytes": len(out), "entry": labels.get("_start", 0),
                        "text_va": text_va, "data_va": data_va}
