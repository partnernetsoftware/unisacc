"""UJS construct — gold / oracle / front / emit（非应用运行时）。

应用面请用 ``ujs/web``（浏览器）或 npm 包入口；此处供出货与验收：

    from ujs.construct import Runtime
    python3 -m ujs web-build
"""
__version__ = "0.3.0"

from .api import Result, Runtime, compile, run, wasm_run
from .front.compile import CompileError
from .jtape import Fn
from .js2wasm import Js2WasmError, js2wasm, js2wasm_file
from .oracle import Oracle
from .vm import Trap

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
