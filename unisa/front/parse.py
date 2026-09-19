"""Recursive-descent walker. [W-3] [W-7] [W-8]

Classic control flow.  Every table-shaped decision -- which production, which
scope action, which result type -- leaves through the oracle and comes back as
a single class name. [P-1]
"""
from ..gold import TOKS
from ..ir import Emitter, ACC, LHS, TMP, FP, SP, ARGREGS, CALLEE
from .sema import Scope, Type, VOID, I8, I32, I64, ptr, Struct

TYPEWORD = {"char": I8, "int": I32, "long": I64, "short": I32,
            "void": VOID, "unsigned": I32, "signed": I32,
            "float": I64, "double": I64}
ASSIGN_OPS = {"+=": "+", "-=": "-", "*=": "*", "/=": "/"}
# The syscalls a self-hosting compiler needs, exposed as intrinsics.  They lower
# to the tape's `.sys` gate, so the target facts (sysno, arg registers, gate)
# still come from the abi/enc tables -- nothing here is hardcoded per target.
INTRINSIC = {"__read": "read", "__write": "write", "__open": "open",
             "__close": "close", "__exit": "exit"}
# argc/argv are not syscalls -- the loader hands them over -- so they get their
# own tape ops rather than going through `.sys`.
ARGV_INTRINSIC = ("__argc", "__argv")


class CError(Exception):
    pass


class Walker:
    def __init__(self, toks, oracle):
        self.tk, self.i = toks, 0
        self.o = oracle
        self.sc = Scope(oracle)
        self.em = Emitter(oracle)
        self.lval = None          # Type whose ADDRESS is in ACC, or None
        self.frame_ix = None
        self.off = 0
        self.maxoff = 0
        self.loops = []           # (continue_label, break_label)
        self.switch = []          # dicts for the open switch statements
        self.ret_label = None

    # -- token plumbing ---------------------------------------------------
    def peek(self, k=0):
        return self.tk[min(self.i + k, len(self.tk) - 1)]

    def next(self):
        t = self.tk[self.i]
        self.i += 1
        return t

    def at(self, kind):
        return self.peek().kind == kind

    def eat(self, kind):
        if self.at(kind):
            self.next()
            return True
        return False

    def expect(self, kind):
        if not self.at(kind):
            t = self.peek()
            raise CError("line %d: expected %r, got %r" % (t.line, kind, t.text))
        return self.next()

    def tokclass(self, t=None):
        """Project the current token onto the parse table's TOK axis."""
        t = t or self.peek()
        k = t.kind
        if k == "id" and t.text in self.sc.typedefs:
            return "type"
        return k if k in TOKS else "id"

    def ask(self, nt):
        return self.o.ask("parse", (nt, self.tokclass()))       # [W-3]

    def mark(self):
        return (self.i, len(self.em.t.code))

    def rewind(self, m):
        self.i = m[0]
        del self.em.t.code[m[1]:]

    # -- declarations -----------------------------------------------------
    def declspec(self):
        """Sets self.saw_static as a side effect: a `static` local has static
        storage, not a stack slot.  The interpreter's zeroed memory hid this --
        native execution reads real stack garbage. """
        base = None
        self.saw_static = False
        while True:
            t = self.peek()
            if t.kind == "type":
                if t.text in self.sc.typedefs:
                    base = self.sc.typedefs[t.text]
                    self.next()
                    continue
                self.sc.act("top" if self.sc.depth() == 1 else "local", t)
                w = self.next().text
                if w in ("const", "static", "unsigned", "signed"):
                    if w == "static":
                        self.saw_static = True
                    base = base or I32
                    continue
                base = TYPEWORD[w]
                continue
            if t.kind == "id" and t.text in self.sc.typedefs:
                base = self.sc.typedefs[self.next().text]
                continue
            if t.kind in ("struct", "union"):
                base = self.struct_type()
                continue
            break
        if base is None:
            raise CError("line %d: type expected near %r"
                         % (self.peek().line, self.peek().text))
        return base

    def struct_type(self):
        isu = self.at("union")
        self.next()                                   # struct | union
        tag = self.next().text if self.at("id") else "anon%d" % self.i
        if self.at("{"):
            self.next()
            st = self.sc.structs.setdefault(tag, Struct(tag, isu))
            st.fields, st.size, st.is_union = {}, 0, isu
            while not self.at("}"):
                b = self.declspec()
                while True:
                    ty, nm = self.declarator(b)
                    st.add(nm, ty, self.sc.structs)
                    if not self.eat(","):
                        break
                self.expect(";")
            self.expect("}")
        self.sc.structs.setdefault(tag, Struct(tag, isu))
        return Type("struct", tag=tag)

    def declarator(self, base):
        ty = base
        while self.eat("*"):
            ty = ptr(ty)
        if self.at("(") and self.peek(1).kind == "*":
            # int (*f)(int,int) -- a pointer to function
            self.next()
            while self.eat("*"):
                pass
            name = self.expect("id").text
            self.expect(")")
            if self.at("("):
                self.skip_parens()
            return ptr(Type("fn", ret=ty)), name
        name = self.expect("id").text
        while self.at("["):
            self.next()
            n = self.const_expr()
            self.expect("]")
            ty = Type("arr", to=ty, n=n)
        return ty, name

    # -- top level --------------------------------------------------------
    def skip_parens(self):
        depth = 0
        while True:
            k = self.peek().kind
            if k == "(":
                depth += 1
            elif k == ")":
                depth -= 1
                self.next()
                if depth == 0:
                    return
                continue
            self.next()

    def unit(self):
        while True:
            p = self.ask("top")                                  # [W-3]
            if p == "end":
                break
            if p == "typedef":
                self.do_typedef()
            elif p == "enum":
                self.do_enum()
            elif p in ("struct", "global"):
                self.do_global()
            else:
                raise CError("line %d: unexpected %r at top level"
                             % (self.peek().line, self.peek().text))
        # entry: run the pointer initialisers, then main
        self.em.label("_start")
        for (g, lab) in self.em.init_ptrs:
            self.em.lea(ACC, lab)
            self.em.lea(LHS, g)
            self.em.store(LHS, 0, ACC)
        self.em.call("main")
        self.em.exit_(ACC)
        return self.em.finish()

    def do_typedef(self):
        self.expect("typedef")
        base = self.declspec()
        ty, name = self.declarator(base)
        self.sc.typedefs[name] = ty
        self.expect(";")

    def do_enum(self):
        self.expect("enum")
        if self.at("id"):
            self.next()
        self.expect("{")
        v = 0
        while not self.at("}"):
            nm = self.expect("id").text
            if self.eat("="):
                v = int(self.expect("num").val)
            self.sc.enums[nm] = v
            self.sc.declare(nm, I32, "enum", v)
            v += 1
            if not self.eat(","):
                break
        self.expect("}")
        self.expect(";")

    def do_global(self):
        base = self.declspec()
        if self.eat(";"):
            return
        while True:
            ty, name = self.declarator(base)
            p = self.ask("after_name")                           # [W-3]
            if p == "fn_sig":
                self.function(ty, name)
                return
            self.sc.act("top", self.tk[self.i - 1])
            size = ty.size(self.sc.structs)
            self.em.t.string("g_" + name, b"\x00" * max(1, size), align=8)
            sym = self.sc.declare(name, ty, "global", sym="g_" + name)
            if self.eat("="):
                self.global_init(sym, ty)
            if not self.eat(","):
                break
        self.expect(";")

    def global_init(self, sym, ty):
        t = self.peek()
        if t.kind == "str":
            self.next()
            lab = self.em.intern(t.val)
            self.em.init_ptrs.append((sym.sym, lab))
        elif t.kind == "num":
            self.next()
            w = min(8, max(1, ty.size(self.sc.structs)))
            base = self.em.t.syms[sym.sym] - 0x100
            self.em.t.data[base:base + w] = (int(t.val) & ((1 << (w * 8)) - 1)) \
                .to_bytes(w, "little")
        else:
            raise CError("line %d: only constant global initialisers" % t.line)

    def function(self, ret, name):
        self.sc.act("top", self.peek())       # lparen -> fn_name
        self.expect("(")
        params = []
        if not self.at(")"):
            while True:
                if self.at("type") and self.peek().text == "void" and \
                        self.peek(1).kind == ")":
                    self.next()
                    break
                b = self.declspec()
                ty, pn = self.declarator(b)
                params.append((ty, pn))
                if not self.eat(","):
                    break
        self.expect(")")
        self.sc.declare(name, Type("fn", ret=ret), "fn", sym=name)
        if self.eat(";"):
            return
        self.sc.push()
        self.off, self.maxoff = 0, 0
        self.ret_label = self.em.new_label("ret_" + name + "_")
        self.frame_ix = self.em.prologue(name)
        for k, (ty, pn) in enumerate(params):
            self.sc.act("param", self.peek())
            off = self.alloc(ty)
            self.sc.declare(pn, ty, "local", off)
            self.em.store(FP, -off, ARGREGS[k], min(8, ty.size(self.sc.structs))
                          if ty.kind in ("i8", "i32") else 8)
        self.block(new_scope=False)
        self.em.label(self.ret_label)
        self.em.epilogue()
        self.em.t.code[self.frame_ix].args[0] = (self.maxoff + 7) // 8 * 8
        self.sc.pop()

    def alloc(self, ty):
        sz = max(8, ty.size(self.sc.structs))
        sz = (sz + 7) // 8 * 8
        self.off += sz
        self.maxoff = max(self.maxoff, self.off)
        return self.off

    # -- statements -------------------------------------------------------
    def block(self, new_scope=True):
        self.expect("{")
        if new_scope:
            self.sc.push()
        save = self.off
        while not self.at("}"):
            self.stmt()
        self.expect("}")
        if new_scope:
            self.sc.pop()
            self.off = save

    def stmt(self):
        if self.at("id") and self.peek(1).kind == ":":           # a label
            name = self.next().text
            self.next()
            self.em.label("u_" + name)
            return
        p = self.ask("stmt")                                     # [W-3]
        if p == "block":
            self.block()
        elif p == "decl":
            self.local_decl()
        elif p == "if":
            self.if_stmt()
        elif p == "while":
            self.while_stmt()
        elif p == "for":
            self.for_stmt()
        elif p == "do":
            self.do_stmt()
        elif p == "switch":
            self.switch_stmt()
        elif p == "case":
            self.case_stmt(False)
        elif p == "default":
            self.case_stmt(True)
        elif p == "return":
            self.next()
            if not self.at(";"):
                self.rvalue()
            self.expect(";")
            self.em.jump(self.ret_label)
        elif p == "goto":
            self.next()
            self.em.jump("u_" + self.expect("id").text)
            self.expect(";")
        elif p == "break":
            self.next()
            self.expect(";")
            if not self.loops:
                raise CError("break outside loop/switch")
            self.em.jump(self.loops[-1][1])
        elif p == "continue":
            self.next()
            self.expect(";")
            self.em.jump(self.loops[-1][0])
        else:                                    # expr
            if not self.eat(";"):
                self.expr_comma()
                self.expect(";")

    def local_decl(self):
        base = self.declspec()
        static = self.saw_static
        if self.eat(";"):
            return
        while True:
            ty, name = self.declarator(base)
            self.sc.act("local", self.tk[self.i - 1])
            if static:                       # static storage, zero-initialised
                lab = "g_%s_%d" % (name, self.i)
                self.em.t.string(lab, b"\x00" *
                                 max(1, ty.size(self.sc.structs)), align=8)
                sym = self.sc.declare(name, ty, "global", sym=lab)
                if self.eat("="):
                    self.global_init(sym, ty)
                if not self.eat(","):
                    break
                continue
            off = self.alloc(ty)
            sym = self.sc.declare(name, ty, "local", off)
            if self.eat("="):
                if self.at("{"):
                    self.next()
                    elem = ty.to if ty.kind == "arr" else ty
                    esz = elem.size(self.sc.structs)
                    k = 0
                    while not self.at("}"):
                        self.rvalue()
                        self.em.store(FP, -off + k * esz, ACC, self.wid(elem))
                        k += 1
                        if not self.eat(","):
                            break
                    self.expect("}")
                else:
                    self.rvalue()
                    self.em.store(FP, -off, ACC, self.wid(ty))
            if not self.eat(","):
                break
        self.expect(";")

    def if_stmt(self):
        self.next()
        self.expect("(")
        self.rvalue()
        self.expect(")")
        els, end = self.em.new_label("else"), self.em.new_label("endif")
        self.em.jumpz(els)
        self.stmt()
        if self.at("else"):
            self.em.jump(end)
            self.em.label(els)
            self.next()
            self.stmt()
            self.em.label(end)
        else:
            self.em.label(els)

    def while_stmt(self):
        self.next()
        top, end = self.em.new_label("wtop"), self.em.new_label("wend")
        self.em.label(top)
        self.expect("(")
        self.rvalue()
        self.expect(")")
        self.em.jumpz(end)
        self.loops.append((top, end))
        self.stmt()
        self.loops.pop()
        self.em.jump(top)
        self.em.label(end)

    def do_stmt(self):
        self.next()
        top, cont, end = (self.em.new_label("dtop"), self.em.new_label("dcont"),
                          self.em.new_label("dend"))
        self.em.label(top)
        self.loops.append((cont, end))
        self.stmt()
        self.loops.pop()
        self.em.label(cont)
        self.expect("while")
        self.expect("(")
        self.rvalue()
        self.expect(")")
        self.expect(";")
        self.em.jumpz(end)
        self.em.jump(top)
        self.em.label(end)

    def for_stmt(self):
        self.next()
        self.expect("(")
        self.sc.push()
        save = self.off
        if not self.eat(";"):
            if self.tokclass() == "type":
                self.local_decl()
            else:
                self.expr_comma()
                self.expect(";")
        top, cont, end = (self.em.new_label("ftop"), self.em.new_label("fcont"),
                          self.em.new_label("fend"))
        self.em.label(top)
        if not self.at(";"):
            self.rvalue()
            self.em.jumpz(end)
        self.expect(";")
        step = self.mark()
        depth = 0
        while not (self.at(")") and depth == 0):
            if self.at("("):
                depth += 1
            elif self.at(")"):
                depth -= 1
            self.next()
        self.expect(")")
        body = self.i
        self.loops.append((cont, end))
        self.stmt()
        self.loops.pop()
        after = self.i
        self.em.label(cont)
        if step[0] != body - 1:
            self.i = step[0]
            self.expr_comma()
        self.i = after
        self.em.jump(top)
        self.em.label(end)
        self.sc.pop()
        self.off = save

    def switch_stmt(self):
        self.next()
        self.expect("(")
        self.rvalue()
        self.expect(")")
        slot = self.alloc(I64)
        self.em.store(FP, -slot, ACC)
        disp, end = self.em.new_label("sdisp"), self.em.new_label("send")
        self.em.jump(disp)
        ctx = {"slot": slot, "cases": [], "default": None}
        self.switch.append(ctx)
        self.loops.append((end, end))
        self.stmt()
        self.loops.pop()
        self.switch.pop()
        self.em.jump(end)
        self.em.label(disp)
        for val, lab in ctx["cases"]:
            self.em.load(ACC, FP, -slot)
            self.em.imm(LHS, val)
            self.em.emit(self.em.recipe("alu", "ne"), ACC, ACC, LHS)
            self.em.jumpz(lab)
        self.em.jump(ctx["default"] or end)
        self.em.label(end)

    def case_stmt(self, is_default):
        self.next()
        lab = self.em.new_label("case")
        if is_default:
            self.switch[-1]["default"] = lab
        else:
            v = self.const_expr()
            self.switch[-1]["cases"].append((v, lab))
        self.expect(":")
        self.em.label(lab)

    CPREC = [("|",), ("^",), ("&",), ("<<", ">>"), ("+", "-"),
             ("*", "/", "%")]

    def const_expr(self, level=0):
        """Constant folding for case labels and array bounds.  Array sizes are
        routinely `N * 32` or `A + B`, not bare literals."""
        if level >= len(self.CPREC):
            return self.const_atom()
        v = self.const_expr(level + 1)
        while self.peek().kind in self.CPREC[level]:
            op = self.next().kind
            r = self.const_expr(level + 1)
            if op == "+":
                v = v + r
            elif op == "-":
                v = v - r
            elif op == "*":
                v = v * r
            elif op == "/":
                v = v // r if r else 0
            elif op == "%":
                v = v % r if r else 0
            elif op == "<<":
                v = v << r
            elif op == ">>":
                v = v >> r
            elif op == "&":
                v = v & r
            elif op == "^":
                v = v ^ r
            else:
                v = v | r
        return v

    def const_atom(self):
        t = self.next()
        if t.kind == "num":
            return int(t.val)
        if t.kind == "id" and t.text in self.sc.enums:
            return self.sc.enums[t.text]
        if t.kind == "-":
            return -self.const_atom()
        if t.kind == "(":
            v = self.const_expr()
            self.expect(")")
            return v
        if t.kind == "sizeof":
            self.expect("(")
            b = self.declspec()
            while self.eat("*"):
                b = ptr(b)
            self.expect(")")
            return b.size(self.sc.structs)
        raise CError("line %d: constant expected, got %r" % (t.line, t.text))

    # -- expressions ------------------------------------------------------
    def wid(self, ty):
        return ty.size(self.sc.structs) if ty.kind in ("i8", "i32") else 8

    def load_if_lval(self):
        if self.lval is not None:
            ty = self.lval
            if ty.kind not in ("arr", "struct"):
                self.em.load(ACC, ACC, 0, self.wid(ty))
            self.lval = None
            return ty
        return None

    def rvalue(self):
        ty = self.assign()
        self.load_if_lval()
        return ty

    def expr_comma(self):
        """The comma operator -- lowest precedence, so it is NOT used where a
        comma separates things (call arguments, declarator lists)."""
        ty = self.rvalue()
        while self.at(","):
            self.next()
            ty = self.rvalue()
        return ty

    def assign(self):
        m = self.mark()
        ty = self.unary()
        if self.lval is not None:
            nxt = self.peek().kind
            if nxt == "=":
                self.next()
                aty = self.lval
                self.lval = None
                self.em.push()
                self.rvalue()
                self.em.pop(LHS)
                self.em.store(LHS, 0, ACC, self.wid(aty))
                return aty
            if nxt in ASSIGN_OPS:
                op = ASSIGN_OPS[self.next().kind]
                aty = self.lval
                self.lval = None
                self.em.push()               # address
                self.em.load(ACC, ACC, 0, self.wid(aty))
                self.em.push()               # old value
                self.rvalue()
                if op in ("+", "-") and aty.kind in ("ptr", "arr"):
                    self.scale(aty)
                if op in ("*",):
                    self.em.binop("*")
                elif op == "/":
                    self.em.divmod_("/")
                else:
                    self.em.binop(op)
                self.em.pop(LHS)
                self.em.store(LHS, 0, ACC, self.wid(aty))
                return aty
        self.rewind(m)
        self.lval = None
        return self.ternary()

    def ternary(self):
        ty = self.logic_or()
        if self.at("?"):
            self.next()
            self.load_if_lval()
            els, end = self.em.new_label("qelse"), self.em.new_label("qend")
            self.em.jumpz(els)
            t1 = self.rvalue()
            self.em.jump(end)
            self.expect(":")
            self.em.label(els)
            self.rvalue()
            self.em.label(end)
            return t1
        return ty

    def logic_or(self):
        ty = self.logic_and()
        while self.at("||"):
            self.next()
            self.load_if_lval()
            end = self.em.new_label("orend")
            skip = self.em.new_label("orrhs")
            self.em.jumpz(skip)
            self.em.imm(ACC, 1)
            self.em.jump(end)
            self.em.label(skip)
            self.rvalue()
            self.em.imm(LHS, 0)
            self.em.emit(self.em.recipe("alu", "ne"), ACC, ACC, LHS)
            self.em.label(end)
            ty = I32
        return ty

    def logic_and(self):
        ty = self.binary(0)
        while self.at("&&"):
            self.next()
            self.load_if_lval()
            end = self.em.new_label("andend")
            rhs = self.em.new_label("andrhs")
            self.em.jumpz(end)
            self.rvalue()
            self.em.imm(LHS, 0)
            self.em.emit(self.em.recipe("alu", "ne"), ACC, ACC, LHS)
            self.em.label(end)
            ty = I32
        return ty

    PREC = [("|",), ("^",), ("&",), ("==", "!="),
            ("<", ">", "<=", ">="), ("<<", ">>"), ("+", "-"), ("*", "/", "%")]

    def binary(self, level):
        if level >= len(self.PREC):
            t = self.unary()
            self.load_if_lval()
            return t
        ty = self.binary(level + 1)
        while self.peek().kind in self.PREC[level]:
            self.load_if_lval()
            op = self.next().kind
            self.em.push()
            rty = self.binary(level + 1)
            self.load_if_lval()
            res = self.sc.combine(ty, op, rty)                       # [W-4]
            if op in ("+", "-") and ty.kind in ("ptr", "arr") and \
                    rty.kind in ("i8", "i32", "i64"):
                self.scale(ty)
            if op in ("/", "%"):
                self.em.divmod_(op)
            else:
                self.em.binop(op)
            if op == "-" and ty.kind == "ptr" and rty.kind == "ptr":
                self.unscale(ty)
            ty = self.ty_from(res, ty, rty)
        return ty

    def scale(self, pty):
        n = pty.to.size(self.sc.structs)
        if n != 1:
            self.em.imm(TMP, n)
            self.em.emit(self.em.recipe("alu", "mul"), ACC, ACC, TMP)

    def unscale(self, pty):
        n = pty.to.size(self.sc.structs)
        if n != 1:
            self.em.imm(TMP, n)
            self.em.emit(".div", ACC, ACC, TMP)

    def ty_from(self, kind, t1, t2):
        if kind == "ptr":
            return t1 if t1.kind in ("ptr", "arr") else (
                t2 if t2.kind in ("ptr", "arr") else ptr(I8))
        if kind == "illegal":
            return I64
        return {"void": VOID, "i8": I8, "i32": I32, "i64": I64,
                "arr": t1, "struct": t1, "fn": t1}.get(kind, I64)

    def unary(self):
        p = self.ask("unary")                                    # [W-3]
        if p == "neg":
            self.next()
            t = self.unary()
            self.load_if_lval()
            self.em.neg()
            return t
        if p == "not":
            self.next()
            self.unary()
            self.load_if_lval()
            self.em.logical_not()
            return I32
        if p == "deref":
            self.next()
            t = self.unary()
            self.load_if_lval()
            self.lval = t.to if t.kind in ("ptr", "arr") else I64
            return self.postfix_chain(self.lval)
        if p == "addr":
            self.next()
            t = self.unary()
            if self.lval is None:
                raise CError("line %d: & needs an lvalue" % self.peek().line)
            self.lval = None
            return ptr(t)
        if p == "prim" and self.at("(") and self.tokclass(self.peek(1)) == "type":
            self.next()                                   # a cast
            base = self.declspec()
            while self.eat("*"):
                base = ptr(base)
            self.expect(")")
            self.unary()
            self.load_if_lval()
            if base.kind in ("i8", "i32"):
                self.em.truncate(base.size(self.sc.structs))
            return self.postfix_chain(base)
        if p == "sizeof":
            self.next()
            self.sc.act("sizeof", self.peek(1))
            if self.at("(") and self.tokclass(self.peek(1)) == "type":
                self.next()
                base = self.declspec()
                while self.eat("*"):
                    base = ptr(base)
                self.expect(")")
                n = base.size(self.sc.structs)
            else:
                m = self.mark()
                t = self.unary()
                n = (t or I64).size(self.sc.structs)
                self.rewind(m)
                self.unary_skip()
                self.lval = None
            self.em.imm(ACC, n)
            return I64
        return self.primary()

    def unary_skip(self):
        depth = 0
        while True:
            k = self.peek().kind
            if depth == 0 and k in (";", ",", ")", "]"):
                break
            if k in ("(", "["):
                depth += 1
            elif k in (")", "]"):
                depth -= 1
            self.next()

    def primary(self):
        t = self.peek()
        if t.kind == "(":
            self.next()
            ty = self.assign()
            self.expect(")")
            return self.postfix_chain(ty)
        if t.kind == "num":
            self.next()
            self.em.imm(ACC, int(t.val))
            return self.postfix_chain(I32)
        if t.kind == "str":
            self.next()
            self.em.lea(ACC, self.em.intern(t.val))
            return self.postfix_chain(ptr(I8))
        if t.kind == "id":
            name = t.text
            if self.peek(1).kind == "(":
                self.next()
                return self.call(name)
            self.sc.act("expr", t)
            self.next()
            s = self.sc.lookup(name)
            if s is None:
                raise CError("line %d: unknown identifier %r" % (t.line, name))
            if s.kind == "enum":
                self.em.imm(ACC, s.off)
                return self.postfix_chain(I32)
            if s.kind == "fn":                # function designator -> address
                self.em.lea(ACC, s.sym)
                return self.postfix_chain(ptr(s.ty))
            if s.kind == "global":
                self.em.lea(ACC, s.sym)
            else:
                self.em.imm(TMP, s.off)
                self.em.emit(self.em.recipe("alu", "sub"), ACC, FP, TMP)
            self.lval = s.ty
            return self.postfix_chain(s.ty)
        raise CError("line %d: unexpected %r" % (t.line, t.text))

    def postfix_chain(self, ty):
        while True:
            p = self.ask("postfix")                              # [W-3]
            if p == "done":
                return ty
            if p == "index":
                self.next()
                base = self.lval if self.lval is not None else ty
                if base.kind == "arr" and self.lval is not None:
                    pass                     # address already in ACC
                else:
                    self.load_if_lval()
                self.lval = None
                self.em.push()
                self.rvalue()
                elem = base.to if base.kind in ("ptr", "arr") else I8
                n = elem.size(self.sc.structs)
                if n != 1:
                    self.em.imm(TMP, n)
                    self.em.emit(self.em.recipe("alu", "mul"), ACC, ACC, TMP)
                self.em.pop(LHS)
                self.em.emit(self.em.recipe("alu", "add"), ACC, LHS, ACC)
                self.expect("]")
                self.lval = elem
                ty = elem
            elif p == "field":
                arrow = self.peek().kind == "->"
                self.next()
                self.sc.act("field", self.peek())
                fname = self.expect("id").text
                if arrow:
                    self.load_if_lval()
                    sty = ty.to
                else:
                    sty = self.lval if self.lval is not None else ty
                    self.lval = None
                st = self.sc.structs[sty.tag]
                fty, foff = st.fields[fname]
                if foff:
                    self.em.imm(TMP, foff)
                    self.em.emit(self.em.recipe("alu", "add"), ACC, ACC, TMP)
                self.lval = fty
                ty = fty
            elif p == "inc":
                op = self.next().kind
                aty = self.lval
                self.lval = None
                self.em.push()
                self.em.load(ACC, ACC, 0, self.wid(aty))
                self.em.push()
                step = aty.to.size(self.sc.structs) if aty.kind == "ptr" else 1
                self.em.imm(ACC, step)
                self.em.binop("+" if op == "++" else "-")
                self.em.pop(LHS)
                self.em.store(LHS, 0, ACC, self.wid(aty))
                self.em.imm(TMP, step)
                self.em.emit(self.em.recipe("alu",
                                            "sub" if op == "++" else "add"),
                             ACC, ACC, TMP)
                ty = aty
            elif p == "call":
                raise CError("line %d: call on non-identifier" % self.peek().line)
            else:
                return ty

    def call(self, name):
        self.expect("(")
        if name == "printf":
            return self.printf()
        if name in INTRINSIC:
            return self.intrinsic(INTRINSIC[name])
        if name in ARGV_INTRINSIC:
            if name == "__argc":
                self.expect(")")
                self.em.emit(".argc", ACC)
                return I64
            self.rvalue()
            self.expect(")")
            self.em.emit(".argv", ACC, ACC)
            return ptr(I8)
        s0 = self.sc.lookup(name)
        if s0 is not None and s0.kind != "fn":
            if s0.kind == "global":
                self.em.lea(CALLEE, s0.sym)
            else:
                self.em.imm(TMP, s0.off)
                self.em.emit(self.em.recipe("alu", "sub"), CALLEE, FP, TMP)
        args = []
        while not self.at(")"):
            self.rvalue()
            self.em.push()
            args.append(1)
            if not self.eat(","):
                break
        self.expect(")")
        s = self.sc.lookup(name)
        indirect = s is not None and s.kind != "fn"
        if len(args) > len(ARGREGS):
            raise CError("line %d: at most %d arguments"
                         % (self.peek().line, len(ARGREGS)))
        # Args are popped straight into their own registers, highest first, so
        # a pop can never clobber an argument that is already placed.
        for k in range(len(args) - 1, -1, -1):
            self.em.pop(ARGREGS[k])
            self.em.arg(k, ARGREGS[k])
        if indirect:
            self.em.load(CALLEE, CALLEE, 0)      # CALLEE held the address
            self.em.call_reg(CALLEE)
        else:
            self.em.call(name)
        rt = I64
        if s is not None:
            t = s.ty
            if t.kind == "ptr" and t.to is not None and t.to.kind == "fn":
                rt = t.to.ret
            elif t.kind == "fn":
                rt = t.ret
        return self.postfix_chain(rt)

    def intrinsic(self, op):
        """__read/__write/__open/__close/__exit -> the `.sys` gate."""
        n = 0
        while not self.at(")"):
            self.rvalue()
            self.em.push()
            n += 1
            if not self.eat(","):
                break
        self.expect(")")
        if n > 3:
            raise CError("line %d: %s takes at most 3 arguments"
                         % (self.peek().line, op))
        for k in range(n - 1, -1, -1):
            self.em.pop(ARGREGS[k])
        for k in range(n, 3):
            self.em.imm(ARGREGS[k], 0)
        self.em.emit(".sys", op, ARGREGS[0], ARGREGS[1], ARGREGS[2])
        return I64

    def printf(self):
        """[W-9] desugared against the static format string."""
        t = self.expect("str")
        fmt = t.val
        i = 0
        lit = ""
        while i < len(fmt):
            c = fmt[i]
            if c != "%":
                lit += c
                i += 1
                continue
            spec = fmt[i + 1]
            i += 2
            if spec == "%":
                lit += "%"
                continue
            self.em.write_literal(lit)
            lit = ""
            self.expect(",")
            self.rvalue()
            if spec == "d" or spec == "i" or spec == "l":
                if spec == "l":
                    i += 1 if i < len(fmt) and fmt[i] == "d" else 0
                self.em.print_int()
            elif spec == "u":
                self.em.mask32()
                self.em.print_int()
            elif spec == "s":
                self.em.print_str()
            elif spec == "c":
                self.em.print_char()
            else:
                raise CError("printf: unsupported %%%s" % spec)
        self.em.write_literal(lit)
        self.expect(")")
        self.em.imm(ACC, 0)
        return I32


def compile_tokens(toks, oracle):
    w = Walker(toks, oracle)
    tape = w.unit()
    if "main" not in tape.labels:
        raise CError("no main()")
    return tape
