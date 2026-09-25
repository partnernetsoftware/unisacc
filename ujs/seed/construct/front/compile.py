"""UJS compiler: src → Fn (jtape). Decisions via Oracle. [W]"""
from ..jtape import Fn, Ins
from ..oracle import Oracle
from .lex import lex


class CompileError(Exception):
    pass


class Compiler:
    def __init__(self, src, oracle=None):
        self.src = src
        self.o = oracle or Oracle(drive="gold")
        self.toks = lex(src, self.o)
        self.i = 0
        self.code = []
        self.strings = []
        self.str_ix = {}
        self.localslot = []
        self.local_set = set()
        self.scopes = [set()]  # nested local name sets
        self.loop_stack = []   # (break_patches, continue_target)
        self.functions = {}    # name -> Fn (nested)

    def cur(self):
        return self.toks[self.i]

    def at(self, *kinds):
        return self.cur().kind in kinds

    def eat(self, kind=None):
        t = self.cur()
        if kind is not None and t.kind != kind:
            raise CompileError("expected %s got %s" % (kind, t.kind))
        self.i += 1
        return t

    def emit(self, op, a=None, b=None, c=None):
        self.code.append(Ins(op, a, b, c))
        return len(self.code) - 1

    def str_id(self, s):
        if s not in self.str_ix:
            self.str_ix[s] = len(self.strings)
            self.strings.append(s)
        return self.str_ix[s]

    def bind(self, name):
        act = self.o.ask("scope", ("block" if len(self.scopes) > 1 else "mod",
                                   "bind"))
        if act == "illegal":
            raise CompileError("cannot bind %s" % name)
        if name not in self.local_set:
            self.localslot.append(name)
            self.local_set.add(name)
        self.scopes[-1].add(name)

    def resolve_load(self, name):
        for s in reversed(self.scopes):
            if name in s:
                return ("l", name)
        return ("g", name)

    def tokclass(self):
        return self.cur().kind

    def ask(self, nt):
        return self.o.ask("parse", (nt, self.tokclass()))

    def ask_type(self, t1, op, t2="null"):
        return self.o.ask("type", (t1, op, t2))

    def ask_shape(self, ty, keysig="empty"):
        return self.o.ask("shape", (ty, keysig))

    def ask_ic(self, shape, op, guard="type_ok"):
        from .. import catalog as C
        if op not in C.IC_OPS:
            op = "add"
        if shape not in C.SHAPES:
            shape = "s_i64"
        return self.o.ask("ic", (shape, op, guard))

    def ask_isel(self, jop):
        from .. import catalog as C
        if jop not in C.JOPS:
            jop = "add"
        return self.o.ask("isel", (jop,))

    def note_binop(self, op):
        """Side-channel asks so web heat shows full inference path."""
        ty = self.ask_type("i64", op if op in (
            "+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=",
            "&&", "||", "in") else "+", "i64")
        sh = self.ask_shape("i64" if ty in ("i64", "bool", "illegal") else ty)
        icop = {"+": "add", "-": "sub", "*": "mul", "/": "div", "%": "mod",
                "<": "lt", "==": "eq", "in": "in"}.get(op, "add")
        self.ask_ic(sh, icop, "type_ok")
        self.ask_isel({"+": "add", "-": "sub", "*": "mul", "/": "div", "%": "mod",
                       "<": "lt", ">": "gt", "<=": "le", ">=": "ge",
                       "==": "eq", "!=": "ne", "&&": "and", "||": "or",
                       "in": "in"}.get(op, "add"))

    def compile(self):
        while not self.at("eof"):
            self.statement()
        if not self.code or self.code[-1].op != "ret":
            self.emit("const", "null", None)
            self.emit("ret")
        return Fn(self.src, self.code, self.localslot, self.strings,
                  meta={"nparams": 0})

    def statement(self):
        p = self.ask("stmt")
        if p == "block":
            return self.block()
        if p == "let":
            return self.stmt_let()
        if p == "if":
            return self.stmt_if()
        if p == "while":
            return self.stmt_while()
        if p == "for":
            return self.stmt_for()
        if p == "switch":
            return self.stmt_switch()
        if p == "return":
            return self.stmt_return()
        if p == "break":
            return self.stmt_break()
        if p == "continue":
            return self.stmt_continue()
        if p == "fn":
            return self.stmt_function()
        # expr stmt
        self.expr()
        if self.at(";"):
            self.eat(";")
        self.emit("drop")

    def block(self):
        self.eat("{")
        self.scopes.append(set())
        while not self.at("}", "eof"):
            self.statement()
        self.eat("}")
        self.scopes.pop()

    def stmt_let(self):
        self.eat("let")
        while True:
            name = self.eat("id").text
            self.bind(name)
            if self.at("="):
                self.eat("=")
                self.expr()
            else:
                self.emit("const", "null", None)
            self.emit("store_l", name)
            if self.at(","):
                self.eat(",")
                continue
            break
        if self.at(";"):
            self.eat(";")

    def stmt_if(self):
        self.eat("if")
        self.eat("(")
        self.expr()
        self.eat(")")
        jz = self.emit("jumpz", 0)
        self.statement()
        if self.at("else"):
            self.eat("else")
            jmp = self.emit("jump", 0)
            self.code[jz].a = len(self.code)
            self.statement()
            self.code[jmp].a = len(self.code)
        else:
            self.code[jz].a = len(self.code)

    def stmt_while(self):
        self.eat("while")
        head = len(self.code)
        self.eat("(")
        self.expr()
        self.eat(")")
        jz = self.emit("jumpz", 0)
        breaks = []
        self.loop_stack.append((breaks, head))
        self.statement()
        self.emit("jump", head)
        self.code[jz].a = len(self.code)
        end = len(self.code)
        for bi in breaks:
            self.code[bi].a = end
        self.loop_stack.pop()

    def stmt_for(self):
        # for (let i = 0; ...)  |  for (let x of xs)
        self.eat("for")
        self.eat("(")
        self.scopes.append(set())
        if (self.at("let") and self.i + 2 < len(self.toks)
                and self.toks[self.i + 1].kind == "id"
                and self.toks[self.i + 2].kind == "of"):
            self.eat("let")
            name = self.eat("id").text
            self.eat("of")
            self.bind(name)
            arr = "__a%d" % len(self.localslot)
            ixn = "__i%d" % len(self.localslot)
            self.bind(arr)
            self.bind(ixn)
            self.expr()
            self.emit("store_l", arr)
            self.eat(")")
            self.emit("const", "i64", 0)
            self.emit("store_l", ixn)
            head = len(self.code)
            self.emit("load_l", ixn)
            self.emit("load_l", arr)
            self.emit("ic_enter", "len")
            self.emit("len")
            self.emit("lt")
            jz = self.emit("jumpz", 0)
            jmp_body = self.emit("jump", 0)
            incr = len(self.code)
            self.emit("load_l", ixn)
            self.emit("const", "i64", 1)
            self.emit("add")
            self.emit("store_l", ixn)
            self.emit("jump", head)
            body = len(self.code)
            self.code[jmp_body].a = body
            self.emit("load_l", arr)
            self.emit("load_l", ixn)
            self.emit("ic_enter", "idx")
            self.emit("idx")
            self.emit("store_l", name)
            breaks = []
            self.loop_stack.append((breaks, incr))
            self.statement()
            self.emit("jump", incr)
            end = len(self.code)
            self.code[jz].a = end
            for bi in breaks:
                self.code[bi].a = end
            self.loop_stack.pop()
            self.scopes.pop()
            return

        if self.at("let"):
            self.stmt_let()
        elif not self.at(";"):
            self.expr()
            self.emit("drop")
            self.eat(";")
        else:
            self.eat(";")
        head = len(self.code)
        if not self.at(";"):
            self.expr()
        else:
            self.emit("const", "bool", True)
        self.eat(";")
        jz = self.emit("jumpz", 0)
        jmp_body = self.emit("jump", 0)
        step = len(self.code)
        if not self.at(")"):
            self.expr()
            self.emit("drop")
        self.eat(")")
        self.emit("jump", head)
        body = len(self.code)
        self.code[jmp_body].a = body
        breaks = []
        self.loop_stack.append((breaks, step))
        self.statement()
        self.emit("jump", step)
        end = len(self.code)
        self.code[jz].a = end
        for bi in breaks:
            self.code[bi].a = end
        self.loop_stack.pop()
        self.scopes.pop()

    def stmt_switch(self):
        """Compare-chain switch (table form remains a jtape op for lower)."""
        self.eat("switch")
        self.eat("(")
        self.expr()
        self.eat(")")
        self.eat("{")
        self.bind("__sw")
        self.emit("store_l", "__sw")
        breaks = []
        self.loop_stack.append((breaks, None))
        end_patches = []

        while not self.at("}"):
            if self.at("case"):
                self.eat("case")
                t = self.cur()
                if t.kind == "num":
                    key = ("i64" if isinstance(t.val, int) else "f64",
                           self.eat("num").val)
                elif t.kind == "str":
                    key = ("str", self.eat("str").val)
                elif t.kind in ("true", "false"):
                    key = ("bool", self.eat().val)
                elif t.kind == "null":
                    self.eat("null")
                    key = ("null", None)
                else:
                    raise CompileError("case label")
                self.eat(":")
                self.emit("load_l", "__sw")
                if key[0] == "null":
                    self.emit("const", "null", None)
                elif key[0] == "bool":
                    self.emit("const", "bool", key[1])
                elif key[0] == "str":
                    self.emit("const", "str", self.str_id(key[1]))
                else:
                    self.emit("const", key[0], key[1])
                self.emit("eq")
                jz = self.emit("jumpz", 0)
                while not self.at("case", "default", "}"):
                    if self.at("break"):
                        self.eat("break")
                        if self.at(";"):
                            self.eat(";")
                        breaks.append(self.emit("jump", 0))
                        break
                    self.statement()
                end_patches.append(self.emit("jump", 0))
                self.code[jz].a = len(self.code)
            elif self.at("default"):
                self.eat("default")
                self.eat(":")
                while not self.at("case", "default", "}"):
                    if self.at("break"):
                        self.eat("break")
                        if self.at(";"):
                            self.eat(";")
                        breaks.append(self.emit("jump", 0))
                        break
                    self.statement()
                end_patches.append(self.emit("jump", 0))
            else:
                raise CompileError("in switch")
        self.eat("}")
        end = len(self.code)
        for p in end_patches + breaks:
            self.code[p].a = end
        self.loop_stack.pop()

    def stmt_return(self):
        self.eat("return")
        if self.at(";") or self.at("}"):
            self.emit("const", "null", None)
        else:
            self.expr()
        if self.at(";"):
            self.eat(";")
        self.emit("ret")

    def stmt_break(self):
        self.eat("break")
        if self.at(";"):
            self.eat(";")
        if not self.loop_stack:
            raise CompileError("break outside loop/switch")
        self.loop_stack[-1][0].append(self.emit("jump", 0))

    def stmt_continue(self):
        self.eat("continue")
        if self.at(";"):
            self.eat(";")
        if not self.loop_stack or self.loop_stack[-1][1] is None:
            raise CompileError("continue outside loop")
        self.emit("jump", self.loop_stack[-1][1])

    def stmt_function(self):
        self.eat("function")
        name = self.eat("id").text
        self.eat("(")
        params = []
        rest = None
        while not self.at(")"):
            if self.at("..."):
                self.eat("...")
                rest = self.eat("id").text
                break
            params.append(self.eat("id").text)
            if self.at(","):
                self.eat(",")
        self.eat(")")
        # compile body as separate Fn
        body_start = self.i
        # reuse nested Compiler for body block
        if not self.at("{"):
            raise CompileError("function body")
        # extract body source roughly via tokens — compile with nested
        nested = Compiler(self.src, self.o)
        nested.toks = self.toks
        nested.i = self.i
        nested.scopes = [set(params + ([rest] if rest else []))]
        nested.local_set = set(nested.scopes[0])
        nested.localslot = list(params) + ([rest] if rest else [])
        nested.eat("{")
        while not nested.at("}", "eof"):
            nested.statement()
        nested.eat("}")
        if not nested.code or nested.code[-1].op != "ret":
            nested.emit("const", "null", None)
            nested.emit("ret")
        self.i = nested.i
        fn = Fn(self.src, nested.code, nested.localslot, nested.strings,
                meta={"nparams": len(params), "rest": rest, "name": name})
        # store as const fn in globals via local binding
        self.bind(name)
        # encode fn as host object in meta pool
        ix = len(self.strings)  # abuse: keep fns in meta
        self.meta_fns = getattr(self, "meta_fns", {})
        self.meta_fns[name] = fn
        # emit: load from a special const — use print-like host later
        # For VM: const kind 'fn' with name key resolved at link
        self.emit("const", "fn", name)
        self.emit("store_l", name)
        _ = body_start

    # ---- expressions (Pratt) --------------------------------------------
    PREC = {
        "||": 1, "??": 1, "&&": 2,
        "==": 3, "!=": 3,
        "<": 4, ">": 4, "<=": 4, ">=": 4, "in": 4,
        "+": 5, "-": 5,
        "*": 6, "/": 6, "%": 6,
    }

    def expr(self, min_prec=0):
        self.assign()

    def assign(self):
        self.ternary()
        if self.at("=", "+=", "-=", "*=", "/="):
            op = self.eat().kind
            if not self.code:
                raise CompileError("bad assign")
            last = self.code.pop()
            # drop trailing ic_enter on idx
            if last.op == "idx" and self.code and self.code[-1].op == "ic_enter":
                self.code.pop()
            if op != "=":
                # reload LHS then rhs then binop
                if last.op == "load_l":
                    self.emit("load_l", last.a)
                elif last.op == "load_g":
                    self.emit("load_g", last.a)
                else:
                    raise CompileError("compound assign needs name")
                self.ternary()
                self.emit("ic_enter", {"+=": "+", "-=": "-", "*=": "*", "/=": "/"}[op])
                self.emit({"+=": "add", "-=": "sub", "*=": "mul", "/=": "div"}[op])
            else:
                self.ternary()
            if last.op == "load_l":
                self.emit("store_l", last.a)
                self.emit("load_l", last.a)
            elif last.op == "load_g":
                self.emit("store_g", last.a)
                self.emit("load_g", last.a)
            elif last.op == "idx":
                self.emit("ic_enter", "setidx")
                self.emit("setidx")
            else:
                raise CompileError("bad assign target %s" % last.op)

    def ternary(self):
        self.binary(0)
        if self.at("?"):
            self.eat("?")
            jz = self.emit("jumpz", 0)
            self.assign()
            jmp = self.emit("jump", 0)
            self.code[jz].a = len(self.code)
            self.eat(":")
            self.assign()
            self.code[jmp].a = len(self.code)

    def binary(self, min_prec):
        self.unary()
        while self.cur().kind in self.PREC and self.PREC[self.cur().kind] >= min_prec:
            op = self.eat().kind
            self.binary(self.PREC[op] + 1)
            if op in ("+", "-", "*", "/", "%"):
                self.note_binop(op)
                self.emit("ic_enter", op)
                self.emit({"+": "add", "-": "sub", "*": "mul",
                           "/": "div", "%": "mod"}[op])
            elif op in ("<", ">", "<=", ">=", "==", "!="):
                self.note_binop(op)
                if op in ("<", "=="):
                    self.emit("ic_enter", op)
                self.emit({"<": "lt", ">": "gt", "<=": "le", ">=": "ge",
                           "==": "eq", "!=": "ne"}[op])
            elif op == "in":
                self.note_binop("in")
                self.emit("ic_enter", "in")
                self.emit("in")
            elif op == "&&":
                self.note_binop("&&")
                self.emit("and")
            elif op == "||":
                self.note_binop("||")
                self.emit("or")
            elif op == "??":
                # stack: left, right — use typeof == "null"
                self.ask_type("null", "==", "str")
                self.ask_isel("typeof")
                tr = "__nr%d" % len(self.localslot)
                tl = "__nl%d" % len(self.localslot)
                self.bind(tr)
                self.bind(tl)
                self.emit("store_l", tr)
                self.emit("store_l", tl)
                self.emit("load_l", tl)
                self.emit("typeof")
                self.emit("const", "str", self.str_id("null"))
                self.emit("eq")
                jz = self.emit("jumpz", 0)   # not-null → left
                self.emit("load_l", tr)
                jmp = self.emit("jump", 0)
                self.code[jz].a = len(self.code)
                self.emit("load_l", tl)
                self.code[jmp].a = len(self.code)

    def unary(self):
        p = self.ask("unary")
        if p == "neg":
            self.eat("-")
            self.unary()
            self.emit("const", "i64", -1)
            self.emit("mul")
            return
        if p == "not":
            self.eat("!")
            self.unary()
            self.emit("not")
            return
        if p == "spread":
            self.eat("...")
            self.unary()
            self.emit("spread")
            return
        if p == "typeof":
            self.eat("typeof")
            self.unary()
            self.emit("typeof")
            return
        self.postfix()

    def postfix(self):
        self.primary()
        while True:
            p = self.ask("postfix")
            if p == "index":
                self.eat("[")
                self.expr()
                self.eat("]")
                self.emit("ic_enter", "idx")
                self.emit("idx")
            elif p == "call":
                self.eat("(")
                argc = 0
                spread = False
                while not self.at(")"):
                    if self.at("..."):
                        if spread:
                            raise CompileError("multiple spreads in call")
                        self.eat("...")
                        self.expr()  # leave list on stack
                        spread = True
                        if self.at(","):
                            raise CompileError("spread must be last call argument")
                        break
                    self.expr()
                    argc += 1
                    if self.at(","):
                        self.eat(",")
                self.eat(")")
                self.emit("ic_enter", "call")
                # 128+n = CALL_SPREAD with n fixed args before the list
                self.emit("call", (128 + argc) if spread else argc)
            elif p == "dot":
                self.eat(".")
                name = self.eat("id").text
                self.emit("ic_enter", "dot")
                self.emit("dot", name)
            else:
                break

    def primary(self):
        p = self.ask("primary")
        if p == "ident":
            name = self.eat("id").text
            # arrow: x => expr  or  x => { stmts }
            if self.at("=>"):
                return self._arrow_fn([name])
            # intrinsic len(x) / keys(x)
            if name in ("len", "keys") and self.at("("):
                self.eat("(")
                self.expr()
                self.eat(")")
                if name == "len":
                    self.emit("ic_enter", "len")
                self.emit(name)
                return
            where, n = self.resolve_load(name)
            self.emit("load_l" if where == "l" else "load_g", n)
            return
        if p == "number":
            t = self.eat("num")
            if isinstance(t.val, float):
                self.emit("const", "f64", t.val)
            else:
                self.emit("const", "i64", t.val)
            return
        if p == "string":
            t = self.eat("str")
            self.emit("const", "str", self.str_id(t.val))
            return
        if p == "null":
            self.eat("null")
            self.emit("const", "null", None)
            return
        if p == "true":
            self.eat("true")
            self.emit("const", "bool", True)
            return
        if p == "false":
            self.eat("false")
            self.emit("const", "bool", False)
            return
        if p == "list":
            self.eat("[")
            n = 0
            while not self.at("]"):
                self.expr()
                n += 1
                if self.at(","):
                    self.eat(",")
            self.eat("]")
            self.emit("mklist", n)
            return
        if p == "dict":
            self.eat("{")
            n = 0
            while not self.at("}"):
                if self.at("str"):
                    k = self.eat("str").val
                elif self.at("id"):
                    k = self.eat("id").text
                else:
                    raise CompileError("dict key")
                self.emit("const", "str", self.str_id(k))
                self.eat(":")
                self.expr()
                n += 1
                if self.at(","):
                    self.eat(",")
            self.eat("}")
            self.emit("mkdict", n)
            return
        if p == "paren":
            # (a, b) => body  |  (expr)
            self.eat("(")
            if self.at(")"):
                self.eat(")")
                if self.at("=>"):
                    return self._arrow_fn([])
                raise CompileError("empty paren")
            # collect ids until ) then check =>
            if self.at("id") or self.at("..."):
                save_i = self.i
                params = []
                rest = None
                ok_params = True
                while not self.at(")"):
                    if self.at("..."):
                        self.eat("...")
                        rest = self.eat("id").text
                        params.append(rest)
                        break
                    if not self.at("id"):
                        ok_params = False
                        break
                    params.append(self.eat("id").text)
                    if self.at(","):
                        self.eat(",")
                    elif not self.at(")"):
                        ok_params = False
                        break
                if ok_params and self.at(")"):
                    self.eat(")")
                    if self.at("=>"):
                        return self._arrow_fn(params, rest=rest)
                self.i = save_i
            self.expr()
            self.eat(")")
            return
        raise CompileError("primary near %s" % self.cur().kind)

    def _arrow_fn(self, params, rest=None):
        self.eat("=>")
        name = "__arrow%d" % len(getattr(self, "meta_fns", {}))
        nested = Compiler("", self.o)
        nested.toks = self.toks
        nested.i = self.i
        nested.scopes = [set()]
        for p in params:
            nested.bind(p)
        if self.at("{"):
            nested.eat("{")
            nested.scopes.append(set())
            while not nested.at("}"):
                nested.statement()
            nested.eat("}")
            nested.scopes.pop()
        else:
            nested.expr()
            nested.emit("ret")
        if not nested.code or nested.code[-1].op != "ret":
            nested.emit("const", "null", None)
            nested.emit("ret")
        self.i = nested.i
        from ..jtape import Fn
        fn = Fn(self.src, nested.code, nested.localslot, nested.strings,
                meta={"nparams": len(params) - (1 if rest else 0),
                      "rest": rest, "name": name,
                      "fns": getattr(nested, "meta_fns", {})})
        # nparams: if rest, params include rest name
        nparams = len(params) - (1 if rest else 0)
        fn.meta["nparams"] = nparams
        self.meta_fns = getattr(self, "meta_fns", {})
        # merge strings
        for s in nested.strings:
            self.str_id(s)
        self.meta_fns[name] = fn
        self.emit("const", "fn", name)
        return


def compile_src(src, oracle=None):
    c = Compiler(src, oracle)
    fn = c.compile()
    fn.meta["fns"] = getattr(c, "meta_fns", {})
    return fn
