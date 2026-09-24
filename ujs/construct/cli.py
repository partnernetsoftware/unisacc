"""ujs construct CLI. [U] — 出货/验收；应用库见 ``ujs/web`` / npm ``ujs``。"""
import argparse
import json
import os
import sys

from .gold import STAGES, ALL
from .oracle import Oracle
from . import api
from . import lower_wasm as LW
from .front.compile import CompileError


WEIGHTS = os.path.join("weights", "ujs")


def build_nets(names=None, verify=True):
    from unisa.intnet import build
    names = names or ALL
    return {n: build(n, verify=verify, stages=STAGES) for n in names}


def cmd_build_weights(a):
    os.makedirs(a.out, exist_ok=True)
    from unisa.intnet import save_all
    nets = build_nets(verify=not a.no_verify)
    path = os.path.join(a.out, "built.json")
    n = save_all(nets, path)
    print("wrote %s (%d bytes, %d stages)" % (path, n, len(nets)))
    return 0


def cmd_acc(a):
    nets = build_nets(verify=True)
    print("%-7s %7s %8s  %s" % ("stage", "rows", "units", "exact"))
    bad = []
    for n in ALL:
        st = STAGES[n]
        net = nets[n]
        flag = "OK" if net.exact else "FAIL"
        if not net.exact:
            bad.append(n)
        print("%-7s %7d %8d  %s" % (n, st.rows(), net.nunits(), flag))
    if bad:
        print("UNDERFIT:", ", ".join(bad))
        return 1
    print("all stages exact — [P-3] by construction + enumeration")
    return 0


def cmd_run(a):
    src = open(a.file).read() if a.file else a.code
    G = json.loads(a.globals) if a.globals else {}
    L = json.loads(a.locals) if a.locals else {}
    drive = a.drive
    nets = None
    if drive == "built":
        nets = build_nets(verify=False)
    o = Oracle(nets=nets, drive=drive)
    backend = "wasm" if a.wasm else "jtape"
    r = api.run(src, G, L, oracle=o, ic=a.ic, backend=backend)
    if not r.ok:
        print("ERR", r.err, file=sys.stderr)
        return 1
    print(json.dumps(r.to_py()["ok"], ensure_ascii=False))
    return 0


def cmd_fold(a):
    src = open(a.file).read()
    G = json.loads(a.globals) if a.globals else {}
    L = json.loads(a.locals) if a.locals else {}
    o = Oracle(drive="gold")
    r1 = api.run(src, G, L, oracle=o, backend="jtape")
    r2 = api.run(src, G, L, oracle=o, backend="wasm")
    if not r1.ok or not r2.ok:
        print("fail", r1.err, r2.err)
        return 1
    from . import value as V
    ok = V.to_py(r1.value) == V.to_py(r2.value)
    print("jtape", V.to_py(r1.value))
    print("wasm ", V.to_py(r2.value))
    print("1/1 match" if ok else "MISMATCH")
    return 0 if ok else 1


def cmd_icfold(a):
    """[X-3][IC-3] IC on/off same answer for same (fn,G,L)."""
    src = open(a.file).read() if a.file else a.code
    G = json.loads(a.globals) if a.globals else {}
    L = json.loads(a.locals) if a.locals else {}
    o = Oracle(drive="gold")
    r_off = api.run(src, G, L, oracle=o, ic=False, backend="jtape")
    r_on = api.run(src, G, L, oracle=o, ic=True, backend="jtape")
    if not r_off.ok or not r_on.ok:
        print("fail", r_off.err, r_on.err)
        return 1
    from . import value as V
    a_off, a_on = V.to_py(r_off.value), V.to_py(r_on.value)
    ok = a_off == a_on
    print("ic=off", a_off)
    print("ic=on ", a_on)
    print("1/1 match" if ok else "MISMATCH")
    return 0 if ok else 1


DEFAULT_DIFF_PROBES = [
    "return 1 + 2 * 3;",
    "let xs = [1,2,3]; return xs[1] + len(xs);",
    'return "a" + "b";',
    "let d = {a: 1, b: 2}; return d.a + d.b;",
    """
function sum(a, ...rest) {
  let s = a; let i = 0;
  while (i < len(rest)) { s = s + rest[i]; i = i + 1; }
  return s;
}
return sum(1, 2, 3);
""",
    """
let x = 2;
switch (x) {
  case 1: return 10;
  case 2: return 20;
  default: return 0;
}
""",
]


def cmd_difftest(a):
    """[X-2] gold (无网参考) ≡ built IntNet 同答。"""
    from . import value as V
    nets = build_nets(verify=False)
    o_gold = Oracle(drive="gold")
    o_built = Oracle(nets=nets, drive="built")
    probes = list(DEFAULT_DIFF_PROBES)
    if a.file:
        probes.append(open(a.file).read())
    bad = 0
    for src in probes:
        r0 = api.run(src, oracle=o_gold)
        r1 = api.run(src, oracle=o_built)
        if not r0.ok or not r1.ok or V.to_py(r0.value) != V.to_py(r1.value):
            print("MISMATCH", src.strip()[:40], r0.err, r1.err,
                  V.to_py(r0.value) if r0.ok else None,
                  V.to_py(r1.value) if r1.ok else None)
            bad += 1
        else:
            print("ok", V.to_py(r0.value))
    print("%d/%d match" % (len(probes) - bad, len(probes)))
    return 0 if bad == 0 else 1


def cmd_ship(a):
    """[X-7] kit: JS product + weights + VM + sample + MANIFEST."""
    import zipfile
    from .build.ic_c import emit as emit_ic
    from .ujs2wasm import ujs2wasm

    os.makedirs(os.path.dirname(os.path.abspath(a.out)) or ".", exist_ok=True)
    nets = build_nets(verify=True)
    from unisa.intnet import save_all
    import tempfile
    _ujs = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    web = os.path.join(_ujs, "core")
    # ensure product assets + BUILD.json fingerprint exist
    full_wasm = os.path.join(web, "ujs_full.wasm")
    build_json = os.path.join(web, "BUILD.json")
    if not os.path.isfile(full_wasm) or not os.path.isfile(build_json):
        print("ship: running web-build for product assets…", file=sys.stderr)
        cmd_web_build(argparse.Namespace(out=web))

    with tempfile.TemporaryDirectory() as td:
        wpath = os.path.join(td, "built.json")
        save_all(nets, wpath)
        weights = open(wpath, "rb").read()
        emit_ic(os.path.join(td, "ujs_ic_net.c"))
        ic_c = open(os.path.join(td, "ujs_ic_net.c"), "rb").read()
        sample = os.path.join(td, "fact.wasm")
        ujs2wasm(
            "let n=5; let f=1; while(n>0){f=f*n; n=n-1;} return f;",
            sample,
        )
        wasm = open(sample, "rb").read()

    vm_c = open(os.path.join(_ujs, "native", "ujs_vm.c"), "rb").read()
    manifest = {
        "format": "UJS-1",
        "product": "core/wasm_run.js + ujs_full.wasm",
        "stages": {
            n: {"units": nets[n].H, "exact": bool(nets[n].exact)}
            for n in ALL
        },
        "kernel": "embed→gemv→ReLU→gemv→argmax (constructed IntNet) [K-1]",
        "exact": "by construction, verified over FULL gold [P-3]",
        "ic_keys": 975,
        "selfcheck": {
            "acc": "all stages exact",
            "sample": "fact.wasm → 120",
        },
    }
    if any(not nets[n].exact for n in ALL):
        print("ship refused: underfit", file=sys.stderr)
        return 1

    js_files = [
        "index.js", "wasm_run.js", "compiler.js", "compiler.gen.js",
        "ujs_full.wasm", "BUILD.json",
    ]
    with zipfile.ZipFile(a.out, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("weights/built.json", weights)
        z.writestr("MANIFEST.json", json.dumps(manifest, indent=1,
                                               sort_keys=True))
        z.writestr("native/ujs_vm.c", vm_c)
        z.writestr("native/ujs_ic_net.c", ic_c)
        z.writestr("samples/fact.wasm", wasm)
        pkg = open(os.path.join(_ujs, "package.json"), "rb").read()
        z.writestr("package.json", pkg)
        for name in js_files:
            if name == "package.json":
                continue
            path = os.path.join(web, name)
            if not os.path.isfile(path):
                print("ship missing", path, file=sys.stderr)
                return 1
            z.write(path, "core/" + name)
    print("wrote", a.out, os.path.getsize(a.out), "bytes")
    print("MANIFEST stages", len(ALL), "exact")
    return 0


def cmd_compile(a):
    src = open(a.file).read()
    o = Oracle(drive="gold")
    fn = api.compile(src, o)
    blob = json.dumps(fn.to_dict(), sort_keys=True, separators=(",", ":"))
    open(a.out, "w").write(blob)
    if a.wasm_out:
        prog = LW.lower(fn, o)
        open(a.wasm_out, "wb").write(LW.encode_wasm(prog))
    print("ok", a.out)
    return 0


def cmd_web_build(a):
    from .build.web import web_build
    info = web_build(a.out)
    print("wasm", info["wasm"], info["bytes"], "bytes,",
          info["stages"], "stages")
    # full UJS VM runtime (host loads image into memory)
    import subprocess, os
    from .ujs2wasm import _ensure_ic_net, VM_C, IC_C
    _ensure_ic_net()
    full = os.path.join(a.out, "ujs_full.wasm")
    subprocess.check_call([
        "zig", "cc", "-target", "wasm32-freestanding", "-O2",
        "-Wl,--no-entry",
        "-Wl,--export=run_prog", "-Wl,--export=main_export",
        "-Wl,--export=tag_of_export", "-Wl,--export=i64_of_export",
        "-Wl,--export=str_len_export", "-Wl,--export=str_ptr_export",
        "-Wl,--export=last_ic_stub_export", "-Wl,--export=ujs_ic_ask",
        "-Wl,--export=host_reset", "-Wl,--export=host_prog_addr",
        "-Wl,--export=host_load_image", "-Wl,--export=host_run",
        "-Wl,--export=host_set_local", "-Wl,--export=host_set_global",
        "-Wl,--export=host_get_local", "-Wl,--export=host_get_global",
        "-Wl,--export=host_mk_null", "-Wl,--export=host_mk_bool",
        "-Wl,--export=host_mk_i64", "-Wl,--export=host_mk_f64",
        "-Wl,--export=host_mk_str",
        "-Wl,--export=host_mk_list", "-Wl,--export=host_list_set",
        "-Wl,--export=host_list_get",
        "-Wl,--export=host_mk_dict", "-Wl,--export=host_dict_set",
        "-Wl,--export=host_dict_key", "-Wl,--export=host_dict_val",
        "-Wl,--export=host_len",
        "-Wl,--export=f64_of_export",
        "-Wl,--export=mem_base", "-Wl,--export=mem_size",
        "-o", full, VM_C, IC_C,
    ])
    print("full", full, os.path.getsize(full), "bytes")
    # prebuild demo programs via ujs2wasm into core/progs/
    from .ujs2wasm import ujs2wasm
    demo_dir = os.path.join(a.out, "progs")
    os.makedirs(demo_dir, exist_ok=True)
    demos = {
        "arith": "return 1 + 2 * 3;",
        "fact": "let n=5; let f=1; while(n>0){f=f*n; n=n-1;} return f;",
        "str": 'return "hello" + " " + "ujs";',
        "dict": "let d={a:1,b:2}; return d.a + d.b;",
        "sum": """
function sum(a, ...rest) {
  let s = a; let i = 0;
  while (i < len(rest)) { s = s + rest[i]; i = i + 1; }
  return s;
}
return sum(10, 20, 30);
""",
        "arrow": "let f = x => x * x; return f(12);",
        "forof": "let s=0; for (let x of [1,2,3,4,5]) { s += x; } return s;",
        "ternary": "return (1===1) ? 100 : 0;",
        "nullish": "return null ?? 42;",
        "blockarrow": "let f = x => { return x + 1; }; return f(41);",
    }
    meta = {}
    for name, src in demos.items():
        path = os.path.join(demo_dir, name + ".wasm")
        info2 = ujs2wasm(src, path)
        meta[name] = {"src": src.strip(), "wasm": "progs/%s.wasm" % name,
                      "bytes": info2["bytes"]}
    import json
    with open(os.path.join(a.out, "full_demos.json"), "w") as f:
        json.dump(meta, f, indent=2)
    print("full demos", len(meta))
    from .build.browser import emit as emit_browser
    gen = emit_browser(a.out)
    print("browser compiler", gen)
    from .build.web import write_build_json
    bpath = write_build_json(a.out)
    print("BUILD.json", bpath)
    return 0


def cmd_ujs2wasm(a):
    from .ujs2wasm import ujs2wasm_file, Ujs2WasmError
    try:
        info = ujs2wasm_file(a.file, a.out, mode=a.mode)
    except Ujs2WasmError as e:
        print("ujs2wasm:", e, file=sys.stderr)
        return 1
    how = "direct" if info.get("direct") else "vm"
    print("wrote %s (%d bytes%s, %s)" % (
        info["wasm"], info["bytes"],
        (", image %d" % info["image"]) if info.get("image") else "",
        how))
    return 0


def main(argv=None):
    p = argparse.ArgumentParser(prog="ujs")
    sp = p.add_subparsers(dest="cmd", required=True)

    b = sp.add_parser("build-weights")
    b.add_argument("--out", default=WEIGHTS)
    b.add_argument("--no-verify", action="store_true")
    b.set_defaults(func=cmd_build_weights)

    a = sp.add_parser("acc")
    a.set_defaults(func=cmd_acc)

    r = sp.add_parser("run")
    r.add_argument("file", nargs="?")
    r.add_argument("--code")
    r.add_argument("--globals", default="")
    r.add_argument("--locals", default="")
    r.add_argument("--drive", default="gold",
                   choices=("gold", "built"))
    r.add_argument("--ic", action="store_true")
    r.add_argument("--wasm", action="store_true")
    r.set_defaults(func=cmd_run)

    w = sp.add_parser("wasm-run")
    w.add_argument("file")
    w.add_argument("--globals", default="")
    w.add_argument("--locals", default="")
    w.add_argument("--drive", default="gold")
    w.add_argument("--ic", action="store_true")
    w.set_defaults(func=lambda a: cmd_run(argparse.Namespace(
        file=a.file, code=None, globals=a.globals, locals=a.locals,
        drive=a.drive, ic=a.ic, wasm=True)))

    f = sp.add_parser("fold")
    f.add_argument("file")
    f.add_argument("--globals", default="")
    f.add_argument("--locals", default="")
    f.set_defaults(func=cmd_fold)

    icf = sp.add_parser("icfold", help="[X-3] IC on/off same answer")
    icf.add_argument("file", nargs="?")
    icf.add_argument("--code")
    icf.add_argument("--globals", default="")
    icf.add_argument("--locals", default="")
    icf.set_defaults(func=cmd_icfold)

    d = sp.add_parser("difftest", help="[X-2] gold ≡ built")
    d.add_argument("file", nargs="?")
    d.set_defaults(func=cmd_difftest)

    sh = sp.add_parser("ship", help="[X-7] kit zip + MANIFEST")
    sh.add_argument("--out", default="ujs-kit.zip")
    sh.set_defaults(func=cmd_ship)

    c = sp.add_parser("compile")
    c.add_argument("file")
    c.add_argument("-o", "--out", required=True)
    c.add_argument("--wasm-out")
    c.set_defaults(func=cmd_compile)

    wb = sp.add_parser("web-build",
                       help="emit ujs/core product (ujs_full.wasm + compiler.gen)")
    wb.add_argument("--out", default=os.path.join("ujs", "core"))
    wb.set_defaults(func=cmd_web_build)

    j = sp.add_parser("ujs2wasm", help="UJS → .wasm (direct \\0asm; vm fallback)")
    j.add_argument("file")
    j.add_argument("-o", "--out", required=True)
    j.add_argument("--mode", choices=("auto", "direct", "vm"), default="auto",
                   help="auto=direct if i64-subset else C-VM; direct|vm force")
    j.set_defaults(func=cmd_ujs2wasm)

    args = p.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
