"""ujs2wasm — UJS → browser-loadable .wasm. [LW-3]

    python3 -m ujs ujs2wasm prog.ujs -o prog.wasm

Default: **direct** emit (jtape → WAT → wat2wasm) when ``isel``/codegen
cover the program — no C VM in the artifact. Only true surface gaps
(``restpack``/… until emit lands) fall back to the C-VM embed via zig.

Force paths: ``--mode direct`` / ``--mode vm``.

Exports (both paths): main_export, tag_of_export, i64_of_export, …
Direct also: clear_slots / run_step / host_reset / host_run / host_set_global /
host_mk_* / host_list_* / host_dict_* (aligned with ``ujs_vm.c``) so binders can
inject globals and step without a bytecode image.
"""
from __future__ import annotations

import os
import subprocess
import tempfile

from .bc_encode import encode_fn, EncodeError
from .emit_wasm import DirectEmitError, can_emit_direct, emit_wasm
from .front.compile import compile_src, CompileError
from .oracle import Oracle
from .paths import seed_vm, ujs_root, weights_dir
from .wat_vm import pack_program

_UJS = ujs_root()
VM_C = os.path.join(seed_vm(), "ujs_vm.c")
IC_C = os.path.join(weights_dir(), "ujs_ic_net.c")


class Ujs2WasmError(Exception):
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
        "-Wl,--export=host_mk_list",
        "-Wl,--export=host_list_set",
        "-Wl,--export=host_list_get",
        "-Wl,--export=host_mk_dict",
        "-Wl,--export=host_dict_set",
        "-Wl,--export=host_dict_key",
        "-Wl,--export=host_dict_val",
        "-Wl,--export=host_len",
        "-Wl,--export=f64_of_export",
        "-Wl,--export=mem_base",
        "-Wl,--export=mem_size",
        "-o", out_path,
    ]
    for d in defines or []:
        cmd.append("-D" + d)
    cmd.extend(sources)
    subprocess.check_call(cmd)


def _ujs2wasm_vm(src: str, out_path: str) -> dict:
    """Phase-1 helper: bytecode + C VM → wasm via zig."""
    _ensure_ic_net()
    o = Oracle(drive="gold")
    try:
        fn = compile_src(src, o)
        blob = encode_fn(fn)
        image = pack_program(blob)
    except (CompileError, EncodeError, SyntaxError, AssertionError) as e:
        raise Ujs2WasmError(str(e)) from e

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

    if open(out_path, "rb").read(4) != b"\0asm":
        raise Ujs2WasmError("not a wasm module")
    return {
        "wasm": out_path,
        "bytes": os.path.getsize(out_path),
        "image": len(image),
        "direct": False,
        "full": True,
    }


def _ujs2wasm_direct(src: str, out_path: str) -> dict:
    o = Oracle(drive="gold")
    try:
        fn = compile_src(src, o)
    except (CompileError, SyntaxError, AssertionError) as e:
        raise Ujs2WasmError(str(e)) from e
    ok, why = can_emit_direct(fn, o)
    if not ok:
        raise Ujs2WasmError("direct: %s" % why)
    try:
        return emit_wasm(fn, out_path, o)
    except DirectEmitError as e:
        raise Ujs2WasmError("direct emit: %s" % e) from e


def ujs2wasm(src: str, out_path: str, mode: str = "auto") -> dict:
    """Compile UJS source to ``out_path``.

    mode: ``auto`` (direct if covered else vm), ``direct``, ``vm``.
    """
    if mode not in ("auto", "direct", "vm"):
        raise Ujs2WasmError("bad mode %r" % mode)

    if mode == "vm":
        return _ujs2wasm_vm(src, out_path)

    if mode == "direct":
        return _ujs2wasm_direct(src, out_path)

    # auto: try direct when isel+emit cover; do not swallow emit failures
    o = Oracle(drive="gold")
    try:
        fn = compile_src(src, o)
    except (CompileError, SyntaxError, AssertionError) as e:
        raise Ujs2WasmError(str(e)) from e
    ok, why = can_emit_direct(fn, o)
    if ok:
        return emit_wasm(fn, out_path, o)
    return _ujs2wasm_vm(src, out_path)


def ujs2wasm_file(path: str, out_path: str, **kw) -> dict:
    return ujs2wasm(open(path).read(), out_path, **kw)
