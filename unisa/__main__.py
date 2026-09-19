"""unisa CLI. [U]"""
import argparse
import os
import sys

from . import uns1
from . import catalog as C
from .driver import compile_file
from .exec_target import execute
from .gold import STAGES, ALL
from .lower import lower, FAULTS
from .oracle import Oracle
from .train import train, report, build_nets, SHIP_ACC
from .vm import run as vm_run

WEIGHTS = "weights"


def _load(names=ALL, wdir=WEIGHTS):
    nets = build_nets(names)
    missing = []
    for n in names:
        p = os.path.join(wdir, n + ".f32.unisa")
        if os.path.exists(p):
            uns1.load_into(nets[n], p)
        else:
            missing.append(n)
    return nets, missing


def cmd_train(a):
    nets, _ = train(epochs=a.epochs, out=a.out, verbose=not a.quiet)
    print()
    under = report(nets)
    return 1 if under else 0


def cmd_acc(a):
    nets, missing = _load()
    if missing:
        print("no weights for: %s  (run `unisa train`)" % ", ".join(missing))
        return 1
    print("%-7s %7s %8s  %s" % ("stage", "rows", "theta", "acc (FULL gold)"))
    bad = []
    for n in ALL:
        acc, per = nets[n].evaluate(STAGES[n].corpus())
        nets[n].acc = acc
        extra = ""
        if len(per) > 1:
            extra = "   " + " ".join("%s=%.3f" % (k, v) for k, v in per.items())
        flag = "" if acc >= SHIP_ACC else "  <-- UNDERFIT"
        if flag:
            bad.append(n)
        print("%-7s %7d %8d  %.4f%s%s"
              % (n, STAGES[n].rows(), nets[n].nparams(), acc, flag, extra))
    tot = sum(nets[n].nparams() for n in ALL)
    print("%-7s %7s %8d" % ("TOTAL", "", tot))
    if bad:
        print("\nUNDERFIT: %s  -- [F-5] fix the gold key encoding, "
              "do not widen the net" % ", ".join(bad))
        return 1
    print("\nall stages 1.000 -- [P-3] discharged by enumeration over FULL gold")
    return 0


BUILT = os.path.join(WEIGHTS, "built.json")


def _built():
    from . import intnet
    if os.path.exists(BUILT):
        return intnet.load_all(BUILT)
    nets = intnet.build_all()                                      # verifies
    intnet.save_all(nets, BUILT)
    return nets


def _oracle(drive="spec"):
    if drive == "built":
        return Oracle(_built(), drive="built")
    nets, missing = _load()
    if missing:
        print("training first (weights missing: %s)" % ", ".join(missing))
        nets, _ = train(out=WEIGHTS, verbose=False)                # [U-1]
    return Oracle(nets, drive=drive)


def cmd_build(a):
    """[K-5] construct exact integer weights -- no training, no randomness."""
    from . import intnet
    from .gold import ALL
    nets = intnet.build_all()
    n = intnet.save_all(nets, a.out)
    print("%-7s %6s %8s  %s" % ("stage", "units", "maxlogit", "weights"))
    vals = set()
    for st in ALL:
        v = nets[st].weight_values()
        vals |= set(v)
        print("%-7s %6d %8s  %s" % (st, nets[st].nunits(),
                                    nets[st].maxlogit, v))
    print("%-7s %6d" % ("TOTAL", sum(nets[s].nunits() for s in ALL)))
    print("\nweight values across all nets: %s" % sorted(vals))
    print("all 11 exact by construction, verified over FULL gold")
    from . import uns2
    blob = uns2.dump(nets, STAGES)
    rep = uns2.size_report(nets, STAGES)
    nocombo = sum(v for k, v in rep.items() if k != "combo") + 16
    with open(a.pack, "wb") as f:
        f.write(blob)
    print("\n%-7s %8s" % ("stage", "UNS2 B"))
    for st in ALL:
        print("%-7s %7d B" % (st, rep[st]))
    print("%-7s %7d B   magic %s" % ("TOTAL", len(blob), blob[:4].hex()))
    print("%-7s %7d B   (default --drive spec path)" % ("no combo", nocombo))
    print("\ncache: %s (%d B, json)   ship: %s (%d B)"
          % (a.out, n, a.pack, len(blob)))
    return 0


def cmd_tape(a):
    o = _oracle(a.drive)
    try:
        t = compile_file(a.file, o, a.target)
    except Exception as e:
        print("%s: %s" % (type(e).__name__, e))
        return 1
    sys.stdout.write(t.to_text())
    return 0


def cmd_run(a):
    o = _oracle(a.drive)
    try:
        t = compile_file(a.file, o, a.target)
    except Exception as e:
        print("%s: %s" % (type(e).__name__, e))
        return 1
    if a.fold:
        return _fold(t, o, a)
    out, code, steps = vm_run(t, argv=[a.file] + a.args)
    sys.stdout.buffer.write(out)
    sys.stdout.flush()
    if a.verbose:
        print("\n[%s  %d steps  exit %d  %s]"
              % (a.target, steps, code, o.summary()), file=sys.stderr)
    return code


def _fold(t, o, a):
    """[A-1] [A-2] lower + execute per target, then compare all six."""
    ref_out, ref_code, _ = vm_run(t)
    rows, ok = [], 0
    for tgt in C.TARGETS:
        tp = lower(t, tgt, o, fault=a.fault, drive=a.drive)
        out, code, steps, trap = execute(tp)
        good = trap is None and (out, code) == (ref_out, ref_code)
        ok += good
        rows.append((tgt, out, code, steps, trap, good))
    for (tgt, out, code, steps, trap, good) in rows:
        note = trap if trap else ("" if good else "differs from tape reference")
        print("  %-14s %-5s %-24s exit=%-3d %6d ops  %s"
              % (tgt, "ok" if good else "FAIL",
                 repr(out.decode("utf8", "replace")), code, steps, note))
    if a.verbose or ok != 6:
        print("  reference (tape vm): %r exit=%d  %s"
              % (ref_out.decode("utf8", "replace"), ref_code, o.summary()))
    print("%d/6 match" % ok)
    return 0 if ok == 6 else 2                                    # [U-4]


def cmd_compile(a):
    from .assemble import assemble
    from . import image
    o = _oracle(a.drive)
    try:
        if a.from_tape:
            from .tape import parse as tparse
            with open(a.file, encoding="latin-1") as f:
                t = tparse(f.read())
        else:
            t = compile_file(a.file, o, a.target)
    except Exception as e:
        print("%s: %s" % (type(e).__name__, e))
        return 1
    tp = lower(t, a.target, o, drive=a.drive)
    text, st = assemble(tp)
    from .tape import DATA_BASE
    data = image.relocate(tp, tp.data, st["data_va"] - DATA_BASE)
    img = image.build(tp, text, data, st["entry"])
    with open(a.out, "wb") as f:                                  # [D-5]
        f.write(img)
    print("%s  %s  %d B  (text %d B, %d/%d insns encoded, magic %s)"
          % (a.out, a.target, len(img), st["bytes"], st["encoded"],
             st["insns"], img[:4].hex()))
    return 0


def cmd_vm(a):
    """Run a tape file directly -- the reference machine. [TP-4]"""
    from .tape import parse as tparse
    with open(a.file, encoding="latin-1") as f:
        t = tparse(f.read())
    out, code, steps = vm_run(t, argv=[a.file] + a.args)
    sys.stdout.buffer.write(out)
    sys.stdout.flush()
    if a.verbose:
        print("\n[%d steps  exit %d]" % (steps, code), file=sys.stderr)
    return code


def cmd_lower(a):
    o = _oracle(a.drive)
    os_, arch = a.target.split("/")
    from .lower import facts
    f = facts(o, a.op, os_, arch, a.drive)
    print("%s  @  %s" % (a.op, a.target))
    for k in ("form", "symbol", "gate", "sysno", "arg0", "arg1", "arg2",
              "ret", "tls"):
        print("  %-7s %s" % (k, f[k]))
    if f["form"] == "winapi":
        print("  %-7s %s" % ("winapi", C.WINAPI.get(a.op)))
    b = C.BYTES.get((a.op, arch))
    if b:
        print("  %-7s %s" % ("bytes", " ".join("%02x" % x for x in b)))
    return 0


def cmd_emit_kernel(a):
    """[K-5] write the decision layer as C: model data + integer kernel."""
    from . import ckernel
    os.makedirs(a.out, exist_ok=True)
    nets = _built()
    n, nc = ckernel.emit_self(nets, os.path.join(a.out, "unisa_self.c"))
    ckernel.emit_core(nets, os.path.join(a.out, "unisa_core.c"))
    print("%s/unisa_self.c  model blob %d B, %d self-test decisions"
          % (a.out, n, nc))
    print("%s/unisa_core.c  model + kernel, no main (for unisacc.c)" % a.out)
    return 0


def cmd_dump(a):
    nets, missing = _load()
    if missing:
        print("no weights for: %s" % ", ".join(missing))
        return 1
    os.makedirs(a.out, exist_ok=True)
    dt = uns1.NAME_DT[a.dtype]
    tot = 0
    for n in ALL:
        p = os.path.join(a.out, "%s.%s.unisa" % (n, a.dtype))
        tot += uns1.save_net(nets[n], p, dtype=dt)
        print("%-24s %7d B" % (os.path.basename(p), os.path.getsize(p)))
    print("%-24s %7d B" % ("TOTAL", tot))
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(prog="unisa")
    sub = ap.add_subparsers(dest="cmd", required=True)

    t = sub.add_parser("train")
    t.add_argument("--epochs", type=int, default=90)
    t.add_argument("--out", default=WEIGHTS)
    t.add_argument("--quiet", action="store_true")
    t.set_defaults(fn=cmd_train)

    a = sub.add_parser("acc")
    a.set_defaults(fn=cmd_acc)

    tp = sub.add_parser("tape")
    tp.add_argument("file")
    tp.add_argument("--target", default="lnx/x86_64")
    tp.add_argument("--drive", default="spec", choices=["gold", "spec", "combo", "built"])
    tp.set_defaults(fn=cmd_tape)

    r = sub.add_parser("run")
    r.add_argument("file")
    r.add_argument("--target", default="lnx/x86_64")
    r.add_argument("--fold", action="store_true")
    r.add_argument("--drive", default="spec", choices=["gold", "spec", "combo", "built"])
    r.add_argument("--fault", default=None, choices=list(FAULTS))
    r.add_argument("-v", "--verbose", action="store_true")
    # NOT argparse.REMAINDER: that swallows `--fold` into the program's argv
    r.add_argument("args", nargs="*", help="argv for the compiled program")
    r.set_defaults(fn=cmd_run)

    cp = sub.add_parser("compile")
    cp.add_argument("file")
    cp.add_argument("-o", "--out", default="a.out")
    cp.add_argument("--target", default="lnx/x86_64")
    cp.add_argument("--drive", default="spec", choices=["gold", "spec", "combo", "built"])
    cp.add_argument("--from-tape", action="store_true",
                    help="input is a .tape, not C (lower + assemble only)")
    cp.set_defaults(fn=cmd_compile)

    vm = sub.add_parser("vm")
    vm.add_argument("file")
    vm.add_argument("args", nargs=argparse.REMAINDER)
    vm.add_argument("-v", "--verbose", action="store_true")
    vm.set_defaults(fn=cmd_vm)

    lw = sub.add_parser("lower")
    lw.add_argument("op")
    lw.add_argument("--target", default="lnx/x86_64")
    lw.add_argument("--drive", default="spec", choices=["gold", "spec", "combo", "built"])
    lw.set_defaults(fn=cmd_lower)

    bw = sub.add_parser("build-weights")
    bw.add_argument("--out", default=BUILT)
    bw.add_argument("--pack", default=os.path.join(WEIGHTS, "built.uns2"))
    bw.set_defaults(fn=cmd_build)

    ek = sub.add_parser("emit-kernel")
    ek.add_argument("--out", default="kernel")
    ek.set_defaults(fn=cmd_emit_kernel)

    d = sub.add_parser("dump-weights")
    d.add_argument("--dtype", default="i8", choices=["i8", "f32"])
    d.add_argument("--out", default=WEIGHTS)
    d.set_defaults(fn=cmd_dump)

    args = ap.parse_args(argv)
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
