"""LB-1 reference: deterministic loader requirements, never a host resolver.
The domain is Tape ACCEPTED BY TAPECHECK, with explicit target/program mode.
parse() is a reference convenience, not extra acceptance/rejection obligations
owned by this stage. Unvalidated input is outside the equivalence claim. Python is
only a reference oracle for the finite stage, not its product execution path.
"""
import struct
from .tape import parse
TARGETS = {1: "lnx/x86_64", 2: "lnx/arm64", 3: "osx/x86_64", 4: "osx/arm64"}

def produce(frame):
    if len(frame) < 10 or frame[:8] != b"USLSYM1\n":
        raise ValueError("lower symbols: invalid explicit frame")
    target, mode = frame[8:10]
    if target not in TARGETS:
        raise ValueError("lower symbols: target not allowed")
    if mode != 0:
        raise ValueError("lower symbols: unsupported mode")
    tape = parse(frame[10:].decode("latin-1"))
    slots = set()
    indirect = 0
    for ins in tape.code:
        if ins.op == ".hostaddr":
            slot = ins.args[1]
            if not 0 <= slot < 4:
                raise ValueError("hostaddr index must be 0..3")
            slots.add(slot)
        elif ins.op == ".hostcall":
            indirect = 1
    return (b"UNISYM1\n" + bytes((target, mode, len(slots), indirect)) +
            b"".join(struct.pack("<IBBBB", slot + 1, slot, 1, 0, 0)
                     for slot in sorted(slots)))
