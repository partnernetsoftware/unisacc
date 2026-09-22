"""Public API: compile / run / wasm_run. [A]"""
from .front.compile import compile_src, CompileError
from .jtape import Fn
from .oracle import Oracle
from . import vm
from . import lower_wasm as LW
from . import value as V


class Result:
    __slots__ = ("ok", "err", "value", "locals", "globals")

    def __init__(self, ok=None, err=None, locals_=None, globals_=None):
        self.ok = ok is not None and err is None
        self.value = ok
        self.err = err
        self.locals = locals_
        self.globals = globals_

    def to_py(self):
        if not self.ok:
            return {"err": self.err}
        return {"ok": V.to_py(self.value),
                "locals": {k: V.to_py(v) for k, v in (self.locals or {}).items()},
                "globals": {k: V.to_py(v) for k, v in (self.globals or {}).items()
                            if not callable(V.to_py(v) if False else v)}}


def compile(src, oracle=None):
    return compile_src(src, oracle)


def run(fn_or_src, globals_map=None, locals_map=None, *,
        mutate_globals=False, oracle=None, ic=False, backend="jtape"):
    o = oracle or Oracle(drive="gold")
    try:
        if isinstance(fn_or_src, Fn):
            fn = fn_or_src
        else:
            fn = compile(fn_or_src, o)
        if backend == "wasm":
            prog = LW.lower(fn, o)
            val, L, G = LW.run_wasm(
                prog, globals_map, locals_map,
                mutate_globals=mutate_globals, oracle=o, ic=ic)
        else:
            val, L, G = vm.run(
                fn, globals_map, locals_map,
                mutate_globals=mutate_globals, oracle=o, ic=ic)
        return Result(ok=val, locals_=L, globals_=G)
    except (CompileError, vm.Trap, SyntaxError, AssertionError) as e:
        kind = type(e).__name__
        msg = getattr(e, "message", None) or str(e)
        if isinstance(e, vm.Trap):
            kind = e.kind
            msg = e.message
        return Result(err={"kind": kind, "message": msg})


def wasm_run(fn_or_src, globals_map=None, locals_map=None, **kw):
    """[A-1] run with wasm backend (WasmProgram; browser .wasm is LW-3)."""
    kw.setdefault("backend", "wasm")
    return run(fn_or_src, globals_map, locals_map, **kw)
