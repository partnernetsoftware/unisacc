"""UJS — JS 产品包根；Python 构造在 ``ujs.construct``。

浏览器 / Node / Bun::

    import { bootRuntime, wasm_run } from "ujs";  // 或 ./web/wasm_run.js

构造 / 出货::

    from ujs.construct import Runtime
    python3 -m ujs web-build
"""
__version__ = "0.3.0"

# 兼容：顶层仍可 from ujs import Runtime（指向 construct）
from .construct import (  # noqa: F401
    CompileError,
    Fn,
    Js2WasmError,
    Oracle,
    Result,
    Runtime,
    Trap,
    compile,
    js2wasm,
    js2wasm_file,
    run,
    wasm_run,
)

__all__ = [
    "__version__",
    "Result",
    "Runtime",
    "compile",
    "run",
    "wasm_run",
    "js2wasm",
    "js2wasm_file",
    "Js2WasmError",
    "Oracle",
    "Fn",
    "CompileError",
    "Trap",
]
