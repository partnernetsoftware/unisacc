"""UJS — JS 产品包根；Python 构造在 ``ujs.construct``（``seed/construct``）。

浏览器 / Node / Bun::

    import { bootRuntime, wasm_run } from "ujs";  // 或 ./practice/core/wasm_run.js

构造 / 出货::

    from ujs.construct import Runtime
    python3 -m ujs web-build   # → ujs/practice/core/
"""
__version__ = "0.3.0"

# 兼容：顶层仍可 from ujs import Runtime（指向 construct）
from .construct import (  # noqa: F401
    CompileError,
    Fn,
    Oracle,
    Result,
    Runtime,
    Trap,
    Ujs2WasmError,
    compile,
    run,
    ujs2wasm,
    ujs2wasm_file,
    wasm_run,
)

__all__ = [
    "__version__",
    "Result",
    "Runtime",
    "compile",
    "run",
    "wasm_run",
    "ujs2wasm",
    "ujs2wasm_file",
    "Ujs2WasmError",
    "Oracle",
    "Fn",
    "CompileError",
    "Trap",
]
