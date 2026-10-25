"""UJS construct — gold / oracle / front / emit（非应用运行时）。

应用面请用 ``ujs/core``（浏览器）或 npm 包入口；此处供出货与验收：

    from ujs.construct import Runtime
    python3 -m ujs web-build
"""
__version__ = "0.3.0"

from .api import Result, Runtime, compile, run, wasm_run
from .front.compile import CompileError
from .jtape import Fn
from .ujs2wasm import Ujs2WasmError, ujs2wasm, ujs2wasm_file
from .oracle import Oracle
from .vm import Trap

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
