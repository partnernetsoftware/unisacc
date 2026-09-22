"""UJS — wasm_run(code|fn, globals, locals). See ujs/prd.md."""
__version__ = "0.1.0"

from .api import compile, run, wasm_run

__all__ = ["compile", "run", "wasm_run", "__version__"]
