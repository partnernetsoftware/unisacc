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
    off, offs = 0, []
    for ins in tp.code:
        offs.append(off)
        off += be.size(ins, tp.labels)
    end = off
    labels = {name: offs[pc] if pc < len(offs) else end
              for name, pc in tp.labels.items()}
    text_va, data_va = image.layout(tp.os, tp.arch, end)
    imps = image.imports(tp.os, tp.arch, end)
    # interpreter addresses are DATA_BASE-relative; shift them onto the image
    shift = data_va - DATA_BASE
    out = bytearray()
    covered = 0
    for pc, ins in enumerate(tp.code):
        b = be.encode(ins, offs[pc], labels, tp.arch, tp.syms,
                      shift, text_va, imps)
        if b is None:
            b = be.UD2 if tp.arch == "x86_64" else be.BRK
        else:
            covered += 1
        out.extend(b)
    return bytes(out), {"insns": len(tp.code), "encoded": covered,
                        "bytes": len(out), "entry": labels.get("_start", 0),
                        "text_va": text_va, "data_va": data_va}
