"""js2wasm — UJS-1 → browser-loadable .wasm. [LW-3]

    python3 -m ujs js2wasm prog.ujs -o prog.wasm

Default: full UJS-1 (str/list/dict/fn/...) via freestanding C VM (zig cc).
Exports: memory, main_export / run_prog, tag_of_export, i64_of_export,
         str_ptr_export, str_len_export.

"""
from __future__ import annotations

import os
import subprocess
import tempfile

from .bc_encode import encode_fn, EncodeError
from .front.compile import compile_src, CompileError
from .oracle import Oracle
from .wat_vm import pack_program

# construct/ → ujs/
_UJS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VM_C = os.path.join(_UJS, "native", "ujs_vm.c")
IC_C = os.path.join(_UJS, "native", "ujs_ic_net.c")


class Js2WasmError(Exception):
    pass


def _ensure_ic_net():
    if not os.path.isfile(IC_C):
        from .build.ic_c import emit
        emit(IC_C)


def _zig_cc_wasm(sources, out_path, defines=None):
    cmd = [
        "zig", "cc", "-target", "wasm32-freestanding", "-O2",
        "-Wl,--no-entry",
        "-Wl,--export=run_prog",
        "-Wl,--export=main_export",
        "-Wl,--export=tag_of_export",
        "-Wl,--export=i64_of_export",
        "-Wl,--export=str_len_export",
        "-Wl,--export=str_ptr_export",
        "-Wl,--export=last_ic_stub_export",
        "-Wl,--export=ujs_ic_ask",
        "-Wl,--export=host_reset",
        "-Wl,--export=host_prog_addr",
        "-Wl,--export=host_load_image",
        "-Wl,--export=host_run",
        "-Wl,--export=host_set_local",
        "-Wl,--export=host_set_global",
        "-Wl,--export=host_get_local",
        "-Wl,--export=host_get_global",
        "-Wl,--export=host_mk_null",
        "-Wl,--export=host_mk_bool",
        "-Wl,--export=host_mk_i64",
        "-Wl,--export=host_mk_f64",
        "-Wl,--export=host_mk_str",
        "-Wl,--export=f64_of_export",
        "-Wl,--export=mem_base",
        "-Wl,--export=mem_size",
        "-o", out_path,
    ]
    for d in defines or []:
        cmd.append("-D" + d)
    cmd.extend(sources)
    subprocess.check_call(cmd)


def js2wasm(src: str, out_path: str) -> dict:
    _ensure_ic_net()
    o = Oracle(drive="gold")
    try:
        fn = compile_src(src, o)
        blob = encode_fn(fn)
        image = pack_program(blob)
    except (CompileError, EncodeError, SyntaxError, AssertionError) as e:
        raise Js2WasmError(str(e)) from e

    # generate embed.c
    with tempfile.TemporaryDirectory() as td:
        emb = os.path.join(td, "embed.c")
        with open(emb, "w") as f:
            f.write('#include <stdint.h>\n')
            f.write("typedef unsigned char u8;\n")
            f.write("typedef unsigned int u32;\n")
            f.write("const u8 ujs_embed[] = {\n")
            for i, b in enumerate(image):
                if i % 16 == 0:
                    f.write("  ")
                f.write("%u," % b)
                if i % 16 == 15:
                    f.write("\n")
            f.write("\n};\n")
            f.write("const u32 ujs_embed_len = %u;\n" % len(image))
        _zig_cc_wasm([VM_C, IC_C, emb], out_path, defines=["UJS_EMBED"])

    magic = open(out_path, "rb").read(4)
    if magic != b"\0asm":
        raise Js2WasmError("not a wasm module")
    return {
        "wasm": out_path,
        "bytes": os.path.getsize(out_path),
        "image": len(image),
        "full": True,
    }


def js2wasm_file(path: str, out_path: str, **kw) -> dict:
    return js2wasm(open(path).read(), out_path, **kw)
