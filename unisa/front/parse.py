"""Recursive-descent walker. [W-3] [W-7] [W-8]

Classic control flow.  Every table-shaped decision -- which production, which
scope action, which result type -- leaves through the oracle and comes back as
a single class name. [P-1]
"""
from ..gold import TOKS
from ..ir import (Emitter, ACC, LHS, TMP, FP, SP, ARGREGS, CALLEE, WCHAR,
                  wide_bytes)
from .sema import (Scope, Type, VOID, I8, I16, I32, I64,
                   U8, U16, U32, U64, is_unsigned, is_narrow,
                   F32, F64, FLOATS, ptr, Struct)
from .lex import FNum
# r0..r5 carry a syscall's arguments (r6 is the frame pointer, r7 the stack)
SYSREGS = ("r0", "r1", "r2", "r3", "r4", "r5")
from ..gold import STAGES as _STAGES
PFCONVS = _STAGES["pfconv"].fields[0][1]

# A declaration specifier is a SET of words, not the last one seen:
# `long int` is long and `short int` is short.  Resolving word by word made
# sizeof(long int) == 4.  [E-34]
TYPEWORD = ("char", "int", "long", "short", "void",
            "unsigned", "signed", "float", "double")
ASSIGN_OPS = {"+=": "+", "-=": "-", "*=": "*", "/=": "/", "%=": "%",
              "&=": "&", "|=": "|", "^=": "^", "<<=": "<<", ">>=": ">>"}
# The syscalls a self-hosting compiler needs, exposed as intrinsics.  They lower
# to the tape's `.sys` gate, so the target facts (sysno, arg registers, gate)
# still come from the abi/enc tables -- nothing here is hardcoded per target.
INTRINSIC = {"__read": "read", "__write": "write", "__open": "open",
             "__close": "close", "__exit": "exit",
             "__mprotect": "mprotect", "__munmap": "munmap",
             "__lseek": "lseek", "__unlink": "unlink", "__rename": "rename"}
# six arguments, so the six-register gate: a compiler that runs what it
# compiles maps memory, and mmap takes six
INTRINSIC6 = {"__mmap": "mmap"}
# argc/argv are not syscalls -- the loader hands them over -- so they get their
# own tape ops rather than going through `.sys`.
ARGV_INTRINSIC = ("__argc", "__argv")
# Variadic, and callable before anything declares them.  Calls are resolved at
# the END of the walk, so a call may precede its definition -- harmless for an
# ordinary function, but for these the CALLING CONVENTION differs (every
# argument on the tape stack, [W-13]), and a definition that arrives later
# cannot fix a call that was already emitted the other way.
VARIADIC_LIBC = ("printf", "fprintf", "sprintf", "snprintf")


def _basety(words):
    """Resolve a declaration-specifier word list to a base type."""
    if "void" in words:
        return VOID
    if "double" in words:
        return F64                  # long double is double on both our ABIs'
    if "float" in words:
        return F32
    u = "unsigned" in words
    if "long" in words:
        return U64 if u else I64
    if "short" in words:
        return U16 if u else I16
    if "char" in words:
        return U8 if u else I8
    return U32 if u else I32


def _wide(t):
    """A wide string literal carries CODE POINTS; a narrow one carries the
    source bytes.  Same token kind, because the grammar cannot tell them
    apart -- only the initialiser and the pointer type can."""
    return isinstance(t.val, list)


# the unsigned kinds narrower than a register, and their widths: a value of
# one of these is kept zero-extended in the register (see binary)
NARROW_UNS = {"u8": 1, "u16": 2, "u32": 4}


def _littype(text, v):
    """C99 6.4.4.1: an integer constant takes the first type in its list that
    can hold it, and a HEX constant's list includes the unsigned types.  That
    is why `0xffffffff` is `unsigned int` and `4294967295` is `long` -- and
    why `(int)-1 != 0xffffffff` is false."""
    body = text.rstrip("uUlL")
    suf = text[len(body):].lower()
    hexish = body[:2].lower() == "0x" or (len(body) > 1 and body[0] == "0")
    lng = "l" in suf
    if "u" in suf:
        return U32 if (not lng and v <= 0xFFFFFFFF) else U64
    if not lng and v <= 0x7FFFFFFF:
        return I32
    if hexish and not lng and v <= 0xFFFFFFFF:
        return U32
    if v <= 0x7FFFFFFFFFFFFFFF:
        return I64
    return U64


class CError(Exception):
    pass


class CErrors(CError):
    """More than one error from one walk [S-15 C3]: each carries the token
    it was raised at; the driver locates them one by one."""
    def __init__(self, errors):
        CError.__init__(self, "%d errors" % len(errors))
        self.errors = errors


class Walker:
    def __init__(self, toks, oracle):
        self.tk, self.i = toks, 0
        self.o = oracle
        self.sc = Scope(oracle)
        self.em = Emitter(oracle)
        self._lval = None         # Type whose ADDRESS is in ACC, or None
        self._pre = None          # a leftmost operand assign() already parsed
        self.lbits = None         # ...and where in that word, for a bit-field
        self.vla_saves = []       # per open block: the slot holding its SP
        self.vla_size = {}        # a VLA's name -> the slot with its size
        self.vla_dims = []
        self.frame_ix = None
        self.off = 0
        self.maxoff = 0
        self.loops = []           # (continue, break, vla depth)
        self.errors = []          # every CError this walk recovered from
        self.switch = []          # dicts for the open switch statements
        self.ret_label = None     # set while a function body is being walked
        self.called = {}          # name -> line, checked once the WALK ends
        self.pending = {}         # ...this unit's share, before renaming
        self.ret_label = None
        # `declspec` sets this; `enum E *e;` reaches do_global WITHOUT one,
        # so it has to exist from the start.
        self.saw_static = False
        # One Walker walks EVERY translation unit, so that a call in one file
        # can reach a definition in another without a linker.  The price is
        # that file-scope `static` must stop meaning "the whole program":
        # those names get a per-file suffix.  A single-file compile keeps the
        # empty suffix, so its tape is byte-for-byte what it always was --
        # which matters, because `bootstrap.sh` compares the Python front
        # end's tape against unisacc's own.
        self.unit_tag = ""
        self.unit_start = 0       # first instruction index of this file
        self.renames = {}         # this file's static name -> unique name

    # A bit-field lvalue is an address PLUS a (bit offset, width, signed),
    # and the two must never drift apart -- so producing any lvalue clears
    # the bit part, and only the field access puts it back.
    @property
    def lval(self):
        return self._lval

    @lval.setter
    def lval(self, ty):
        self._lval = ty
        self.lbits = None

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

    # [W-5] the scope table DECIDES where a declared name binds.  The walker
    # knows which context it is in; the table answers what a name there
    # becomes, and that answer picks the storage the symbol gets -- a data
    # label or a frame slot.  An answer that is not a binding stops the
    # compile: the table and the walker disagree about what this is.
    BIND = {"bind_global": "global", "bind_local": "local",
            "bind_param": "local"}

    def bind(self, ctx, tok):
        a = self.sc.act(ctx, tok, declared=True)
        if a not in self.BIND:
            raise CError("line %d: scope: %r in %s is %s, not a binding"
                         % (tok.line, tok.text, ctx, a))
        return self.BIND[a]

    def want(self, ctx, tok, expect, declared=False):
        a = self.sc.act(ctx, tok, declared)
        if a != expect:
            raise CError("line %d: scope: %r in %s is %s, expected %s"
                         % (tok.line, tok.text, ctx, a, expect))
        return a

    def istype(self, t):
        """Does this token start a type name?  `struct`/`union`/`enum` are
        their own token kinds, so `(struct S *)x` and `(struct S){1,2}` both
        need them here as well as `type`."""
        return (self.tokclass(t) == "type"
                or t.kind in ("struct", "union", "enum"))

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
        words = []
        self.saw_static = False
        while True:
            t = self.peek()
            if t.kind == "type":
                if t.text in self.sc.typedefs:
                    if base is not None or words:
                        break       # the type is already named -- see below
                    base = self.sc.typedefs[t.text]
                    self.next()
                    continue
                self.want("top" if self.sc.depth() == 1 else "local", t,
                          "type_name")
                w = self.next().text
                if w == "static":
                    self.saw_static = True
                    continue
                if w in ("const", "volatile", "register", "auto",
                         "extern", "inline"):
                    continue
                words.append(w)
                continue
            if t.kind == "id" and t.text in self.sc.typedefs:
                # A typedef name can only START the specifier list.  Once the
                # type is named, the next typedef name is the thing being
                # DECLARED -- which is how `typedef enum {...} h;` redefines
                # an outer `h` in an inner scope.  Absorbing it left the
                # declarator empty and the error pointed at the semicolon.
                if base is not None or words:
                    break
                base = self.sc.typedefs[self.next().text]
                continue
            if t.kind in ("struct", "union"):
                base = self.struct_type()
                continue
            if t.kind == "enum":
                base = self.enum_type()
                continue
            break
        if words:
            base = _basety(words)
        if base is None:
            raise CError("line %d: type expected near %r"
                         % (self.peek().line, self.peek().text))
        return base

    def struct_type(self):
        isu = self.at("union")
        self.next()                                   # struct | union
        name = self.next().text if self.at("id") else "anon%d" % self.i
        tag = self.sc.tag_bind(name, self.at("{"), self.i)
        if self.at("{"):
            self.next()
            st = self.sc.structs.setdefault(tag, Struct(tag, isu))
            st.fields, st.size, st.is_union = {}, 0, isu
            while not self.at("}"):
                was_enum = self.at("enum")
                self.enum_neg = False
                b = self.declspec()
                if self.at(";"):
                    # an anonymous member: `struct { int x; };` -- its fields
                    # are addressed as if they were the parent's own, so splice
                    # them in at the offset this member would have had
                    self.next()
                    st.embed(b, self.sc.structs)
                    continue
                while True:
                    if self.at(":"):          # `int : 3` -- unnamed padding
                        self.next()
                        st.add_bits("", b, self.const_expr(), True,
                                    self.sc.structs)
                        if not self.eat(","):
                            break
                        continue
                    ty, nm = self.declarator(b)
                    if self.at(":"):
                        self.next()
                        w = self.const_expr()
                        unit = ty.size(self.sc.structs) * 8
                        if ty.kind in ("ptr", "arr", "struct", "fn") \
                                or not 0 <= w <= unit:
                            raise CError("line %d: %r is not a bit-field type "
                                         "or %d is a bad width"
                                         % (self.peek().line, ty, w))
                        signed = not (is_unsigned(ty.kind)
                                      or (was_enum and not self.enum_neg))
                        st.add_bits(nm, ty, w, signed, self.sc.structs)
                    else:
                        st.add(nm, ty, self.sc.structs)
                    if not self.eat(","):
                        break
                self.expect(";")
            self.expect("}")
            st.finish()
        self.sc.structs.setdefault(tag, Struct(tag, isu))
        return Type("struct", tag=tag)

    def declarator(self, base, named=True):
        """The full C99 declarator grammar, built inside out.

        The shapes do not come from a fixed list -- they nest.  `int
        (*f(int, int))(int, int)` is a function returning a pointer to a
        function, and `int (*p[4])(int)` is an array of four such pointers.
        So each level parses its own pointers and suffixes and hands back a
        wrapper for the level outside it to feed."""
        # where the declared name's OWN parameter list was, if the
        # declarator swallowed it -- see do_global
        self.params_at = None
        self.vla_dims = []       # token spans of any non-constant bounds
        name, wrap = self._declarator(named, top=True)
        ty = wrap(base)
        if not named and ty.kind == "fn":
            # a parameter declared as a function is a pointer to one
            # (C99 6.7.5.3p8)
            ty = ptr(ty)
        return ty, name

    def _paren_is_declarator(self):
        """At a `(`: does it open a nested declarator or a parameter list?

        `int (*f)(int)` and `int f(int)` differ only in what follows the
        parenthesis."""
        t = self.peek(1)
        if t.kind in (")", "..."):
            return False                     # `()` -- an empty parameter list
        if self.istype(t):
            return False                     # `(int, char *)` -- parameters
        return t.kind in ("*", "(", "id")

    def _declarator(self, named, top=False):
        nstar = 0
        while True:
            if self.eat("*"):
                nstar += 1
                continue
            # `char * const p` -- a qualifier on the POINTER, not the pointee
            if self.at("type") and self.peek().text in ("const", "volatile",
                                                        "restrict"):
                self.next()
                continue
            break
        name, inner = "", None
        if self.at("(") and self._paren_is_declarator():
            self.next()
            name, inner = self._declarator(named)
            self.expect(")")
        elif self.at("id"):
            self.name_tok = self.peek()
            name = self.next().text
        elif named:
            # a prototype may name no parameter: `int f(int, char *);`
            self.name_tok = self.peek()
            name = self.expect("id").text
        sufs = []
        while True:
            if self.at("["):
                self.next()
                # C99 6.7.5.2: a parameter's array declarator may carry
                # qualifiers and `static` inside the brackets
                while self.at("type") and self.peek().text in (
                        "const", "volatile", "restrict", "static"):
                    self.next()
                if self.at("*") and self.peek(1).kind == "]":
                    self.next()          # `[*]`: an unspecified VLA bound in
                    sufs.append(("arr", 0))   # a prototype -- just a pointer
                elif self.at("]"):
                    sufs.append(("arr", 0))
                else:
                    m = self.mark()
                    try:
                        n = self.const_expr()
                        if not self.at("]"):
                            raise CError("line %d: junk after an array bound"
                                         % self.peek().line)
                        sufs.append(("arr", n))
                    except CError:
                        # C99 6.7.5.2: a bound that is not a constant makes
                        # this a variable-length array.  We cannot evaluate it
                        # here -- no code may be emitted mid-declarator -- so
                        # remember the token span and let local_decl come back
                        # for it.
                        self.rewind(m)
                        start, d = self.i, 0
                        while self.i < len(self.tk):
                            k = self.peek().kind
                            if k == "[":
                                d += 1
                            elif k == "]":
                                if d == 0:
                                    break
                                d -= 1
                            self.next()
                        self.vla_dims.append((start, self.i))
                        sufs.append(("arr", -len(self.vla_dims)))
                self.expect("]")
            elif self.at("(") and not (top and named and inner is None):
                # The OUTERMOST named declarator's own parameter list is not
                # ours: `int f(int)` is a definition or a prototype, and
                # [W-3]'s `after_name` is what decides between them.  Every
                # inner level takes its own, which is what makes
                # `int (*f(int, int))(int, int)` parse.
                if self.params_at is None:
                    self.params_at = self.i   # the innermost one is the name's
                va = self._has_ellipsis()
                self.skip_parens()
                sufs.append(("fn", 1 if va else 0))
            else:
                break

        def wrap(t):
            for _ in range(nstar):
                t = ptr(t)
            # `int a[2][3]` is 2 of (3 of int), so the suffixes apply from
            # the right
            for kind, n in reversed(sufs):
                t = Type("arr", to=t, n=n) if kind == "arr" \
                    else Type("fn", ret=t, n=n)
            return inner(t) if inner is not None else t
        return name, wrap

    # -- top level --------------------------------------------------------
    def _has_ellipsis(self):
        """At a `(`: does this parameter list end in `...`?  A call through a
        pointer has to use the same convention the callee compiled with."""
        depth, j = 0, self.i
        while j < len(self.tk):
            k = self.tk[j].kind
            if k == "(":
                depth += 1
            elif k == ")":
                depth -= 1
                if depth == 0:
                    return False
            elif k == "..." and depth == 1:
                return True
            j += 1
        return False

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
        self.tu()
        return self.finish_program()

    MAXERR = 20                  # as the C front end's -ferror-limit default

    def tu(self):
        """One translation unit.  An error does not end the walk: it is
        recorded at its token, the cursor moves past the construct it was
        in, the walker's per-function state is dropped, and the next
        top-level construct is walked [S-15 C3].  The C front end does the
        same with a parked cursor; here the exception unwinds for us."""
        while True:
            start = self.i
            try:
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
            except CError as e:
                if not hasattr(e, "tok") and self.tk:
                    e.tok = self.tk[min(self.i, len(self.tk) - 1)]
                self.errors.append(e)
                if len(self.errors) >= self.MAXERR:
                    break
                self._resync(start)
        self.seal_unit()

    def _resync(self, start):
        """The token after the construct that began at `start`: a balanced
        brace block (and the `;` that closes a struct's), or a `;` at depth
        0.  Then the state a broken function would have left behind."""
        depth, i = 0, start
        while i < len(self.tk):
            k = self.tk[i].kind
            if k == "{":
                depth += 1
            elif k == "}":
                depth -= 1
                if depth <= 0:
                    i += 1
                    if i < len(self.tk) and self.tk[i].kind == ";":
                        i += 1
                    break
            elif k == ";" and depth == 0:
                i += 1
                break
            i += 1
        self.i = i
        while len(self.sc.stack) > 1:
            self.sc.pop()
        self._lval = self._pre = self.lbits = None
        self.loops, self.vla_saves, self.vla_dims = [], [], []
        self.vla_size = {}

    def seal_unit(self):
        """Rewrite this file's own references to its statics.

        A static that was DECLARED before use already carries its unique name
        -- the call site read it off the symbol.  One used before it is
        declared does not, and C lets that happen.  The fix is local: only
        the instructions this file emitted can mean this file's static."""
        for nm, line in self.pending.items():
            # The call site wrote the name it could see.  A static this file
            # declared later is renamed below, so the name we must find a
            # definition for is the renamed one -- forgetting this left a
            # multi-unit program looking for `memcpy` while the definition it
            # had just been handed was called `memcpy_u0`.
            self.called.setdefault(self.renames.get(nm, nm), line)
        self.pending = {}
        if not self.renames:
            return
        for ins in self.em.t.code[self.unit_start:]:
            if ins.op == "call":              # `call NAME`
                ins.args[0] = self.renames.get(ins.args[0], ins.args[0])
            elif ins.op == ".lea":            # `.lea r, SYMBOL`
                ins.args[1] = self.renames.get(ins.args[1], ins.args[1])

    def finish_program(self):
        # entry: run the pointer initialisers, then main
        self.em.label("_start")
        for (g, lab, off) in self.em.init_ptrs:
            self.em.lea(ACC, lab)
            self.em.lea(LHS, g)
            self.em.store(LHS, off, ACC)
        self._argv_stub()
        self.em.call("main")
        # A return from main is exit(status) (C99 5.1.2.2.3): when the
        # program carries our <stdlib.h>, `exit` is a function in it -- the
        # one that runs the atexit handlers -- so the stub returns through
        # it rather than leaving by .exit with the handlers unrun.
        for nm in ("exit", "exit_u0"):
            if nm in self.em.t.labels:
                self.em.call(nm)
                break
        self.em.exit_(ACC)
        for nm, line in self.called.items():
            if nm not in self.em.t.labels:
                # there is no linker here: a function has to be defined in the
                # translation unit, or come from one of our own headers
                raise CError("line %d: undefined function %r" % (line, nm))
        return self.em.finish()

    def _argv_stub(self):
        """main(argc, argv) gets them in r0, r1: argv is an ARRAY, and the
        machine only answers `.argv rd, k` one element at a time, so `_start`
        builds it.  unisacc emits the same stub."""
        t = self.em.t
        t.string("__argvv", b"\x00" * 32768, align=8)       # 4096 slots, bss
        t.emit(".argc", "r0")
        t.emit(".lea", "r1", "__argvv")
        t.emit("imm", "r2", 0)
        t.label("__argv_top")
        t.emit("slt64", "r3", "r2", "r0")
        t.emit("jumpz", "r3", "__argv_done")
        t.emit(".argv", "r4", "r2")
        t.emit("imm", "r5", 8)
        t.emit("mul64", "r5", "r2", "r5")
        t.emit("add64", "r5", "r1", "r5")
        t.emit("store64", "r5", 0, "r4")
        t.emit("imm", "r5", 1)
        t.emit("add64", "r2", "r2", "r5")
        t.emit("jump", "__argv_top")
        t.label("__argv_done")

    def do_typedef(self):
        self.expect("typedef")
        base = self.declspec()
        ty, name = self.declarator(base)
        self.sc.typedef(name, ty)
        self.expect(";")

    def enum_type(self):
        """`enum [tag] [{ ... }]` -- an enumerated type is an int.  The
        enumerators go into scope; the tag is remembered so `enum E e;` can
        name the type later."""
        self.expect("enum")
        tag = None
        if self.at("id"):
            tag = self.next().text
            self.sc.enum_tags.add(tag)
        if not self.eat("{"):
            # a reference to an existing tag
            self.enum_neg = self.sc.enum_neg.get(tag, False)
            return I32
        v, neg = 0, False
        while not self.at("}"):
            nm = self.expect("id").text
            if self.eat("="):
                v = self.const_expr()
            self.sc.enums[nm] = v
            self.sc.declare(nm, I32, "enum", v)
            neg = neg or v < 0
            v += 1
            if not self.eat(","):
                break
        self.expect("}")
        # An enum bit-field's signedness is implementation-defined; like gcc
        # we make it unsigned unless some enumerator is negative.  Get this
        # wrong and a value with bit 7 set reads back negative out of an
        # 8-bit field, which is exactly what corpus 00218 checks.
        if tag:
            self.sc.enum_neg[tag] = neg
        self.enum_neg = neg
        return I32

    def do_enum(self):
        self.enum_type()
        if self.eat(";"):                    # a bare enum declaration
            return
        self.do_global(base=I32)             # `enum E { A } v;` declares v

    def mangle(self, name):
        """`static` is file scope: give it a name no other file can spell."""
        if not self.unit_tag:
            return name
        self.renames[name] = name + self.unit_tag
        self.renames["g_" + name] = "g_" + name + self.unit_tag
        return name + self.unit_tag

    def do_global(self, base=None):
        if base is None:
            base = self.declspec()
        static = self.saw_static     # a nested declspec will clear it
        if self.eat(";"):
            return
        while True:
            ty, name = self.declarator(base)
            p = self.ask("after_name")                           # [W-3]
            if p == "fn_sig":
                # Normally the parameter list is still ahead of us and `ty` is
                # the return type.  But a function that RETURNS a function
                # pointer wears its own parameters in the middle --
                # `int (*f(int, int))(int, int)` -- so the declarator has
                # already taken them; rewind to them and come back for the
                # body.
                sym = self.mangle(name) if static else name
                if ty.kind == "fn" and self.params_at is not None:
                    body, self.i = self.i, self.params_at
                    r = self.function(ty.ret, name, body=body, sym=sym)
                else:
                    r = self.function(ty, name, sym=sym)
                if r == "proto":
                    # `int f(int), g(int), a;` -- the list goes on
                    if not self.eat(","):
                        break
                    continue
                return
            gkind = self.bind("top", self.name_tok)
            init = self.eat("=")
            if init and ty.kind == "arr" and ty.n == 0:
                ty = Type("arr", to=ty.to, n=self._init_count(ty.to))
            size = ty.size(self.sc.structs)
            lab = "g_" + (self.mangle(name) if static else name)
            self.em.t.string(lab, b"\x00" * max(1, size), align=8)
            sym = self.sc.declare(name, ty, gkind, sym=lab)
            if init:
                self.global_init(sym, ty)
            if not self.eat(","):
                break
        self.expect(";")

    def _members(self, ty):
        """The DIRECT children of an aggregate: (offset, type, bits).

        `bits` is None except for a bit-field, which has no address of its
        own -- an initialiser has to OR it into the storage unit instead of
        writing over it."""
        if ty.kind == "arr":
            esz = ty.to.size(self.sc.structs)
            return [(i * esz, ty.to, None) for i in range(ty.n)]
        st = self.sc.structs[ty.tag]
        return [(o, t, b) for (o, t), b in zip(st.order, st.obits)]

    def _elems(self, ty):
        """The scalar slots of `ty`, in declaration order: (offset, type)."""
        if ty.kind == "arr":
            esz = ty.to.size(self.sc.structs)
            for i in range(ty.n):
                for (o, t) in self._elems(ty.to):
                    yield (i * esz + o, t)
        elif ty.kind == "struct":
            st = self.sc.structs[ty.tag]
            for (_, (fty, foff)) in st.fields.items():
                for (o, t) in self._elems(fty):
                    yield (foff + o, t)
        else:
            yield (0, ty)

    def _aggr_paren(self):
        """Does the `(` at the cursor open an aggregate initialiser?

        An aggregate member facing `(` is usually `(struct S){...}` or
        `((struct S){...})` -- but under brace elision it can equally be the
        first scalar of a flat list, as in `PT a[] = { ((I)4 + (I)2), ... }`,
        where the member is `I c[2]` and the parenthesis is just arithmetic.
        Telling them apart needs the matching `)`."""
        if not self.at("("):
            return False
        j, n = self.i, 0
        while j < len(self.tk):
            k = self.tk[j].kind
            if k == "(":
                n += 1
            elif k == ")":
                n -= 1
                if n == 0:
                    return (j + 1 < len(self.tk)
                            and self.tk[j + 1].kind == "{")
            elif k == "{":
                return True            # the brace is inside the parentheses
            j += 1
        return False

    def _braced_str(self, elem):
        """The token of `{ "..." }` when that is a character array's WHOLE
        initialiser, else None.

        C99 6.7.8p14: an array of character type may be initialised by a
        string literal, *optionally enclosed in braces*.  Read as an ordinary
        brace group instead, `char s[] = {"abc"}` is an array of ONE, and the
        program silently compares a truncated string."""
        if not (self.at("{") and self.peek(1).kind == "str"
                and self.peek(2).kind == "}" and elem is not None):
            return None
        t = self.peek(1)
        if _wide(t):
            ok = elem.kind in ("i32", "u32") \
                and elem.size(self.sc.structs) == WCHAR
        else:
            ok = elem.size(self.sc.structs) == 1
        return t if ok else None

    def _init_count(self, elem=None):
        """How many elements the initialiser at the cursor supplies, so an
        unsized `int a[] = {...}` can be given its length.  A string counts its
        own NUL, which is why `char s[] = "abc"` is 4 bytes."""
        t = self.peek()
        if t.kind == "str":
            return len(t.val) + 1
        st = self._braced_str(elem)
        if st is not None:
            return len(st.val) + 1
        if t.kind != "{":
            return 1
        depth, idx, hi, j = 0, -1, 0, self.i
        nested = False
        while j < len(self.tk):
            k = self.tk[j].kind
            if k == "{":
                depth += 1
                if depth == 1 and self.tk[j + 1].kind != "}":
                    idx = 0
                    hi = 1
                elif depth == 2:
                    nested = True       # each element brought its own braces
            elif k == "}":
                depth -= 1
                if depth == 0:
                    return self._elided(elem, hi, nested)
            elif k == "," and depth == 1 and self.tk[j + 1].kind != "}":
                # a TRAILING comma adds no element: C99 allows it, and
                # `{1, 2, 3,}` is three long, not four
                idx += 1
                hi = max(hi, idx + 1)
            elif k == "[" and depth == 1 and self.tk[j - 1].kind in ("{", ","):
                # `[2] = v` moves the cursor, so the array is at least that
                # long: `{5, [2] = 2, 3}` has four elements
                if self.tk[j + 1].kind == "num":
                    idx = int(self.tk[j + 1].val)
                    hi = max(hi, idx + 1)
            j += 1
        return hi

    def _elided(self, elem, items, nested):
        """Brace elision: `PT a[] = {1,2,3, 4,5,6}` is TWO PTs, not six.

        C99 6.7.8p17 lets an aggregate element take its share of a flat list,
        so counting top-level commas is only right when each element brought
        its own braces (or is a string filling a char array)."""
        if elem is None or nested or elem.kind not in ("arr", "struct"):
            return items
        if self._is_charr(elem) or self._is_wcharr(elem):
            return items                     # each item is a whole string
        per = len(list(self._elems(elem)))
        if per <= 1:
            return items
        return -(-items // per)              # ceil

    def const_init(self, sym, ty, at=0):
        """Write a CONSTANT initialiser into the data image.

        One loop handles every shape: walk the DIRECT members, and let each
        member's own call decide whether it faces a brace group of its own or
        takes its share of a flat list.  `{1, 2, 3, {4, 5}}` needs both in the
        same list, which a "braced or flat" flag cannot express."""
        base = self.em.t.syms[sym] - 0x100
        if ty.kind == "arr" and self._braced_str(ty.to) is not None:
            self.next()                           # `{`  [C99 6.7.8p14]
            self.const_init(sym, ty, at)
            self.expect("}")
            return
        t = self.peek()
        if t.kind == "str" and not _wide(t) and self._is_charr(ty):
            self.next()
            raw = (t.val.encode("latin-1") + b"\x00")[:ty.size(self.sc.structs)]
            self.em.t.data[base + at:base + at + len(raw)] = raw
            return
        if t.kind == "str" and _wide(t) and self._is_wcharr(ty):
            self.next()
            raw = wide_bytes(t.val)[:ty.size(self.sc.structs)]
            self.em.t.data[base + at:base + at + len(raw)] = raw
            return
        if ty.kind in ("arr", "struct") and self._aggr_paren() \
                and not self.istype(self.peek(1)):
            self.next()                           # `((struct S){...})`
            self.const_init(sym, ty, at)
            self.expect(")")
            return
        if ty.kind in ("arr", "struct") and self._aggr_paren() \
                and self.istype(self.peek(1)):
            # `(struct S){...}` used as a value: the same bytes, written here
            self.next()
            self.abstract_type()
            self.expect(")")
            self.const_init(sym, ty, at)
            return
        if ty.kind in ("arr", "struct"):
            braced = self.eat("{")
            mem = self._members(ty)
            k, first = 0, True
            while True:
                # a braced list runs to its `}`; a flat one takes exactly this
                # aggregate's share and leaves the rest to the caller
                if (self.at("}") if braced else k >= len(mem)):
                    break
                if not first:
                    if not self.eat(","):
                        break
                    if braced and self.at("}"):
                        break
                mem, k = self._designator(ty, mem, k)
                if k >= len(mem):
                    raise CError("line %d: too many initialisers"
                                 % self.peek().line)
                off, mty, bf = mem[k]
                if bf is not None:
                    self._const_bits(sym, mty, at + off, bf)
                else:
                    self.const_init(sym, mty, at + off)
                k += 1
                first = False
                if self._is_union(ty):
                    break
            if braced:
                self.expect("}")
            return
        if t.kind == "{":                         # C99 6.7.8p11: a scalar
            self.next()                           # may be braced
            self.const_init(sym, ty, at)
            self.eat(",")
            self.expect("}")
            return
        if t.kind == "str":                       # char *p = "..."
            self.next()
            self.em.init_ptrs.append(
                (sym, self.em.intern_wide(t.val) if _wide(t)
                 else self.em.intern(t.val), at))
            return
        if t.kind == "&" and self.i + 2 < len(self.tk) \
                and self.tk[self.i + 1].kind == "(" \
                and self.istype(self.tk[self.i + 2]):
            self.em.init_ptrs.append((sym, self.static_compound(), at))
            return
        lab = self._addr_of()
        if lab is not None:                       # int *p = &g;  char *q = arr;
            self.em.init_ptrs.append((sym, lab, at))
            return
        if self.isflt(ty) or self._fconst_ahead():
            v = self.fconst_value(ty)
        else:
            v = self.const_expr()
        w = min(8, max(1, ty.size(self.sc.structs)))
        self.em.t.data[base + at:base + at + w] = \
            (v & ((1 << (w * 8)) - 1)).to_bytes(w, "little")

    def _is_union(self, ty):
        st = self.sc.structs.get(ty.tag) if ty.kind == "struct" else None
        return st is not None and st.is_union

    def _is_charr(self, ty):
        return ty.kind == "arr" and ty.to.size(self.sc.structs) == 1

    def _is_wcharr(self, ty):
        return ty.kind == "arr" and ty.to.size(self.sc.structs) == WCHAR \
            and ty.to.kind in ("i32", "u32")

    def _addr_of(self):
        """`&g` or a bare array/function name: the address of a global.  It is
        not a constant we can write into the image, because the image may be
        relocated, so it joins the `_start` pointer initialisers."""
        j = self.i + (1 if self.at("&") else 0)
        if j >= len(self.tk) or self.tk[j].kind != "id":
            return None
        if self.tk[j + 1].kind not in (",", "}", ";"):
            return None
        sy = self.sc.lookup(self.tk[j].text)
        # a function name is an address too: `struct S s = { &zero };` puts a
        # code label in the slot, which `.lea` resolves like any other
        if sy is None or sy.kind not in ("global", "fn"):
            return None
        self.i = j + 1
        return sy.sym

    def _is_union(self, ty):
        st = self.sc.structs.get(ty.tag) if ty.kind == "struct" else None
        return st is not None and st.is_union

    def _is_charr(self, ty):
        return ty.kind == "arr" and ty.to.size(self.sc.structs) == 1

    def _field_index(self, ty, name):
        if ty.kind != "struct":
            raise CError("line %d: .%s in a non-struct initialiser"
                         % (self.peek().line, name))
        oi = self.sc.structs[ty.tag].oindex
        if name in oi:
            return oi[name]
        raise CError("line %d: no field %r" % (self.peek().line, name))

    def _designator(self, ty, slots, k):
        """C99 6.7.8: `.field =` and `[index] =` reposition the cursor.  A
        designator always names a DIRECT member, so it also switches a flat
        list back to the member view."""
        if self.at("."):
            self.next()
            nm = self.expect("id").text
            self.expect("=")
            return self._members(ty), self._field_index(ty, nm)
        if self.at("["):
            self.next()
            i = self.const_expr()
            self.expect("]")
            self.expect("=")
            return self._members(ty), i
        return slots, k

    def global_init(self, sym, ty):
        try:
            self.const_init(sym.sym, ty)
        except CError:
            raise
        except Exception as e:
            raise CError("line %d: only constant global initialisers (%s)"
                         % (self.peek().line, e))

    def function(self, ret, name, body=None, sym=None):
        sym = sym or name          # differs only for a file-scope `static`
        self.want("top", self.peek(), "fn_name")      # lparen -> fn_name
        self.expect("(")
        params, vararg = [], False
        if not self.at(")"):
            while True:
                if self.at("..."):
                    # fine in a prototype -- `printf` is an intrinsic -- but a
                    # DEFINITION would have no way to reach the extra arguments
                    self.next()
                    vararg = True
                    break
                if self.at("type") and self.peek().text == "void" and \
                        self.peek(1).kind == ")":
                    self.next()
                    break
                b = self.declspec()
                ty, pn = self.declarator(b, named=False)
                if ty.kind == "arr":
                    ty = ptr(ty.to)     # C99 6.7.5.3p7: a parameter of array
                params.append((ty, pn, self.name_tok))  # adjusted to ptr [E-33]
                if not self.eat(","):
                    break
        self.expect(")")
        if body is not None:
            self.i = body                    # back to where the body starts
        self.sc.declare(name, Type("fn", ret=ret, n=1 if vararg else 0,
                                   params=[p[0] for p in params]),
                        "fn", sym=sym)
        if self.at(";") or self.at(","):
            return "proto"                   # the caller owns the separator
        self.sc.push()
        self.off, self.maxoff = 0, 0
        self.fn_ret = ret
        self.fn_nfixed = len(params) + (1 if ret.kind == "struct" else 0)
        self.fn_vararg = vararg
        self.ret_label = self.em.new_label("ret_" + sym + "_")
        self.frame_ix = self.em.prologue(sym)
        # More arguments than there are argument registers: ALL of them go on
        # the tape stack instead, pushed in source order, so arg[n-1] sits
        # just above the return address.  The callee knows n -- it is its own
        # parameter count -- so no shuffling is needed at either end. [W-13]
        # A function returning a struct takes a hidden first argument: the
        # address the caller wants the result written to. [W-14]
        sret = ret.kind == "struct"
        self.sret_off = 0
        base_k = 1 if sret else 0
        # A variadic function always takes its arguments on the stack: that is
        # the only layout where the extra ones have addresses. [W-15]
        stacked = vararg or len(params) + base_k > len(ARGREGS)
        if sret:
            self.sret_off = self.alloc(I64)
            if stacked:
                self.em.load(ACC, FP, 16)          # the hidden arg is first
                self.em.store(FP, -self.sret_off, ACC)
            else:
                self.em.store(FP, -self.sret_off, ARGREGS[0])
        pending = []                 # struct parameters, copied after the spill
        for k, (ty, pn, ptok) in enumerate(params):
            pkind = self.bind("param", ptok)
            off = self.alloc(ty)
            self.sc.declare(pn, ty, pkind, off)
            w = (min(8, ty.size(self.sc.structs))
                 if is_narrow(ty.kind) else 8)
            if ty.kind == "struct":
                # By value: what arrives is the address of the caller's copy,
                # and the callee copies it into its own slot so the parameter
                # behaves like any other local [W-14].  The copy CANNOT happen
                # here: `blockcopy` works through r0-r2, and r1/r2 are still
                # holding the arguments that have not been spilled yet.  So
                # the address goes to a slot now and the copy happens once
                # every argument is safely in the frame.
                #
                #   static int f(R a, R b)      -- b came in r1
                #
                # copying `a` clobbered it, and `b` was copied from `a`.
                tmp = self.alloc(I64)
                pending.append((tmp, off, ty))
                if stacked:
                    self.em.load(ACC, FP, 16 + 8 * (k + base_k))
                    self.em.store(FP, -tmp, ACC)
                else:
                    self.em.store(FP, -tmp, ARGREGS[k + base_k])
            elif stacked:
                self.em.load(ACC, FP, 16 + 8 * (k + base_k))
                self.em.store(FP, -off, ACC, w)
            else:
                self.em.store(FP, -off, ARGREGS[k + base_k], w)
        for (tmp, off, ty) in pending:
            self.em.load(LHS, FP, -tmp)
            self.em.imm(ACC, off)
            self.em.emit(self.em.recipe("alu", "sub"), ACC, FP, ACC)
            self.em.blockcopy(ACC, LHS, ty.size(self.sc.structs))
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
        self.vla_saves.append(None)
        while not self.at("}"):
            self.stmt()
        self.expect("}")
        self.vla_restore(len(self.vla_saves) - 1)
        self.vla_saves.pop()
        if new_scope:
            self.sc.pop()
            self.off = save

    def vla_restore(self, depth):
        """Put the stack pointer back, for every open block from the innermost
        down to `depth`.  A `break` or `continue` leaves more than one."""
        for k in range(len(self.vla_saves) - 1, depth - 1, -1):
            off = self.vla_saves[k]
            if off is not None:
                self.em.load(SP, FP, -off, 8)

    def stmt(self):
        if self.at("id") and self.peek(1).kind == ":":           # a label
            name = self.next().text
            self.next()
            self.em.label("u_" + name)
            return self.labelled()
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
                rt = self.rvalue()
                fr = getattr(self, "fn_ret", None)
                self.convto(rt, fr)
                # C99 6.8.6.4p3: the value is converted to the function's
                # type -- a uint32_t function must not hand back 64 bits of
                # whatever the arithmetic left above its width
                if fr is not None and not self.isflt(fr) and not self.isflt(rt):
                    if is_unsigned(fr.kind) and fr.kind != "u64":
                        self.em.zext(fr.size(self.sc.structs))
                    elif is_narrow(fr.kind):
                        self.em.truncate(fr.size(self.sc.structs))
                if getattr(self, "fn_ret", None) is not None \
                        and self.fn_ret.kind == "struct":
                    # ACC is the address of the value; copy it where the
                    # caller asked [W-14]
                    self.em.load(LHS, FP, 0 - self.sret_off)
                    self.em.blockcopy(LHS, ACC,
                                      self.fn_ret.size(self.sc.structs))
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
            # leaving one or more blocks: any VLA in them goes with us
            self.vla_restore(self.loops[-1][2])
            self.em.jump(self.loops[-1][1])
        elif p == "continue":
            self.next()
            self.expect(";")
            self.vla_restore(self.loops[-1][2])
            self.em.jump(self.loops[-1][0])
        else:                                    # expr
            if not self.eat(";"):
                self.expr_comma()
                self.expect(";")

    def local_decl(self):
        if self.at("typedef"):
            # C99 6.7p1: typedef is a storage class, so it is a declaration
            # like any other and may appear inside a block.
            self.do_typedef()
            return
        base = self.declspec()
        static = self.saw_static
        if self.eat(";"):
            return
        while True:
            ty, name = self.declarator(base)
            dims = self.vla_dims
            lkind = self.bind("local", self.name_tok)
            if static:                       # static storage, zero-initialised
                lab = "g_%s_%d%s" % (name, self.i, self.unit_tag)
                init = self.eat("=")
                if init and ty.kind == "arr" and ty.n == 0:
                    ty = Type("arr", to=ty.to, n=self._init_count(ty.to))
                self.em.t.string(lab, b"\x00" *
                                 max(1, ty.size(self.sc.structs)), align=8)
                sym = self.sc.declare(name, ty, "global", sym=lab)
                if init:
                    self.global_init(sym, ty)
                if not self.eat(","):
                    break
                continue
            if self.at("("):
                # a prototype inside a block: `int f1(char *);` declares f1,
                # it does not define a variable
                self.skip_parens()
                self.sc.declare(name, Type("fn", ret=ty), "fn", sym=name)
                if not self.eat(","):
                    break
                continue
            if ty.kind == "arr" and ty.n < 0:
                self.vla_decl(name, ty, dims)
                if not self.eat(","):
                    break
                continue
            init = self.eat("=")
            if init and ty.kind == "arr" and ty.n == 0:
                ty = Type("arr", to=ty.to, n=self._init_count(ty.to))
            off = self.alloc(ty)
            sym = self.sc.declare(name, ty, lkind, off)
            if init:
                self.local_init(ty, off)
            if not self.eat(","):
                break
        self.expect(";")

    def vla_decl(self, name, ty, dims):
        """C99 6.7.5.2: a variable-length array.

        Its storage cannot be in the fixed frame, so it comes off the tape
        stack at the point of declaration: two hidden slots hold the base
        address and the byte size (`sizeof` on a VLA is a runtime load), and
        the enclosing block puts the stack pointer back when it ends."""
        if ty.to.kind == "arr" and ty.to.n < 0:
            raise CError("line %d: a multi-dimensional VLA is not supported"
                         % self.peek().line)
        if self.at("="):
            raise CError("line %d: a VLA may not be initialised"
                         % self.peek().line)
        esz = ty.to.size(self.sc.structs)
        base_off = self.alloc(I64)          # the base address
        size_off = self.alloc(I64)          # the byte size
        # the bound, evaluated HERE, where the declaration is
        back = self.i
        self.i = dims[-ty.n - 1][0]
        self.rvalue()
        self.i = back
        if esz != 1:
            self.em.imm(TMP, esz)
            self.em.emit(self.em.recipe("alu", "mul"), ACC, ACC, TMP)
        # `sizeof` is the EXACT size, so record it before rounding
        self.em.store(FP, -size_off, ACC, 8)
        self.em.imm(TMP, 7)                 # arm64 faults on an unaligned
        self.em.emit(self.em.recipe("alu", "add"), ACC, ACC, TMP)
        self.em.imm(TMP, ~7 & 0xFFFFFFFFFFFFFFFF)   # 64-bit access, so the
        self.em.emit(self.em.recipe("alu", "and"), ACC, ACC, TMP)  # base
        # must stay 8-aligned
        self.vla_mark()                     # remember SP before we move it
        self.em.emit(self.em.recipe("alu", "sub"), SP, SP, ACC)
        self.em.store(FP, -base_off, SP, 8)
        sym = self.sc.declare(name, ty, "local", base_off)
        self.vla_size[sym.name] = size_off

    def vla_mark(self):
        """Save the stack pointer, once per block, just before the first
        variable-length array in it moves SP.  Nothing else has moved SP
        since the block opened -- every other local is in the fixed frame --
        so this is the same value the block started with."""
        if self.vla_saves and self.vla_saves[-1] is not None:
            return
        off = self.alloc(I64)
        self.em.store(FP, -off, SP, 8)
        self.vla_saves[-1] = off

    def _const_bits(self, sym, ty, at, bf):
        """A bit-field in a constant initialiser: OR it into the unit that is
        already there, because its neighbours may have been written first."""
        v = self.const_expr()
        bo, w, _ = bf
        unit = ty.size(self.sc.structs)
        base = self.em.t.syms[sym] - 0x100 + at
        cur = int.from_bytes(self.em.t.data[base:base + unit], "little")
        m = ((1 << w) - 1) << bo
        cur = (cur & ~m | (v << bo) & m) & ((1 << (unit * 8)) - 1)
        self.em.t.data[base:base + unit] = cur.to_bytes(unit, "little")

    def local_init(self, ty, off):
        """Same walk as const_init, but each element is a full expression and
        the result is stored rather than baked into the image."""
        if ty.kind == "arr" and self._braced_str(ty.to) is not None:
            self.next()                           # `{`  [C99 6.7.8p14]
            self.local_init(ty, off)
            self.expect("}")
            return
        t = self.peek()
        if t.kind == "str" and not _wide(t) and self._is_charr(ty):
            self.next()
            raw = (t.val.encode("latin-1") + b"\x00")[:ty.size(self.sc.structs)]
            for i, b in enumerate(raw):
                self.em.imm(ACC, b)
                self.em.store(FP, -off + i, ACC, 1)
            return
        if t.kind == "str" and _wide(t) and self._is_wcharr(ty):
            self.next()
            n = ty.size(self.sc.structs) // WCHAR
            for i, c in enumerate((list(t.val) + [0])[:n]):
                self.em.imm(ACC, c)
                self.em.store(FP, -off + i * WCHAR, ACC, WCHAR)
            return
        # These two called const_init with names this function does not have
        # -- a NameError waiting for the first local initialised this way.
        if ty.kind in ("arr", "struct") and self._aggr_paren() \
                and not self.istype(self.peek(1)):
            self.next()                           # `((struct S){...})`
            self.local_init(ty, off)
            self.expect(")")
            return
        if ty.kind in ("arr", "struct") and self._aggr_paren() \
                and self.istype(self.peek(1)):
            # `(struct S){...}` used as a value: the same bytes, written here
            self.next()
            self.abstract_type()
            self.expect(")")
            self.local_init(ty, off)
            return
        if ty.kind == "struct" and not self.at("{"):
            # C99 6.7.8p13: an automatic struct may be initialised by an
            # EXPRESSION of its type -- `struct s t = f();` -- which copies
            # the whole object.  Anything else here is brace elision.
            m = self.mark()
            t = self.assign()
            if t is not None and t.kind == "struct":
                self.lval = None
                self.em.imm(LHS, off)
                self.em.emit(self.em.recipe("alu", "sub"), LHS, FP, LHS)
                self.em.blockcopy(LHS, ACC, ty.size(self.sc.structs))
                return
            self.rewind(m)
            self.lval = None
        if ty.kind in ("arr", "struct"):
            braced = self.eat("{")
            if braced:
                # C99 6.7.8p21: what the list does not mention is ZERO.  A
                # frame slot holds whatever the last call left there -- the
                # interpreter's fresh stack hid this, and a native image
                # read the leftovers of __init.
                self.em.emit(self.em.recipe("mem", "zero"), FP, -off,
                             ty.size(self.sc.structs))
            mem = self._members(ty)
            k, first = 0, True
            while True:
                # a braced list runs to its `}`; a flat one takes exactly this
                # aggregate's share and leaves the rest to the caller
                if (self.at("}") if braced else k >= len(mem)):
                    break
                if not first:
                    if not self.eat(","):
                        break
                    if braced and self.at("}"):
                        break
                mem, k = self._designator(ty, mem, k)
                if k >= len(mem):
                    raise CError("line %d: too many initialisers"
                                 % self.peek().line)
                eoff, ety, bf = mem[k]
                if bf is not None:
                    self.rvalue()
                    self.em.imm(LHS, -(off - eoff))
                    self.em.emit(self.em.recipe("alu", "add"), LHS, FP, LHS)
                    self.em.push(LHS)
                    self.em.bits_set(bf[0], bf[1], bf[2], self.wid(ety))
                else:
                    self.local_init(ety, off - eoff)
                k += 1
                first = False
                if self._is_union(ty):
                    break
            if braced:
                self.expect("}")
            return
        if self.at("{"):                          # a braced scalar
            self.next()
            self.local_init(ty, off)
            self.eat(",")
            self.expect("}")
            return
        self.convto(self.rvalue(), ty)            # C99 6.7.8p11
        self.em.store(FP, -off, ACC, self.wid(ty))

    def if_stmt(self):
        self.next()
        self.expect("(")
        self.truthy(self.rvalue())
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
        self.truthy(self.rvalue())
        self.expect(")")
        self.em.jumpz(end)
        self.loops.append((top, end, len(self.vla_saves)))
        self.stmt()
        self.loops.pop()
        self.em.jump(top)
        self.em.label(end)

    def do_stmt(self):
        self.next()
        top, cont, end = (self.em.new_label("dtop"), self.em.new_label("dcont"),
                          self.em.new_label("dend"))
        self.em.label(top)
        self.loops.append((cont, end, len(self.vla_saves)))
        self.stmt()
        self.loops.pop()
        self.em.label(cont)
        self.expect("while")
        self.expect("(")
        self.truthy(self.rvalue())
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
            self.truthy(self.rvalue())
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
        self.loops.append((cont, end, len(self.vla_saves)))
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
        # C99 6.8.6.2p1: `continue` belongs to the enclosing ITERATION
        # statement, never to a switch.  Pushing `end` as the continue target
        # too meant `for (...) { switch (x) { case 0: continue; } rest; }` ran
        # `rest` -- the loop's continue point is the only right answer, and a
        # switch that is not inside a loop has none.
        cont = self.loops[-1][0] if self.loops else end
        self.loops.append((cont, end, len(self.vla_saves)))
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
        self.labelled()

    def labelled(self):
        """C99 6.8.1: a label prefixes a STATEMENT.  Returning after the
        label made `switch(x) case 1: return 1;` emit the `return` after the
        switch instead of inside it, so it ran whatever x was.  [E-35]

        A label with nothing after it is not C99, but the compound form
        `case 1: }` is common enough in the wild to tolerate."""
        if not self.at("}"):
            self.stmt()

    def _prec(self, kind):
        """a binary operator's binding strength, 1 (||) to 10 (* / %), 0 for
        anything else -- the `prec` table's answer, asked once per kind [I1]"""
        memo = self.__dict__.setdefault("_precmemo", {})
        if kind not in memo:
            from ..gold import PREC_OPS
            y = self.o.ask("prec", (kind if kind in PREC_OPS else "other",))
            memo[kind] = 0 if y == "none" else int(y)
        return memo[kind]

    def const_expr(self, level=0):
        """Constant folding for case labels and array bounds.  Array sizes are
        routinely `N * 32` or `A + B`, not bare literals -- and the full
        conditional ladder has to be here, because an expression this cannot
        fold becomes a VARIABLE-LENGTH array.  `int a[1 && 1]` is not one."""
        if level == 0:
            v = self.const_expr(1)
            if self.at("?"):
                self.next()
                a = self.const_expr(0)
                self.expect(":")
                b = self.const_expr(0)
                return a if v else b
            return v
        if level > 10:
            return self.const_atom()
        v = self.const_expr(level + 1)
        while self._prec(self.peek().kind) == level:
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
            elif op == "|":
                v = v | r
            elif op == "&&":
                v = 1 if (v and r) else 0
            elif op == "||":
                v = 1 if (v or r) else 0
            elif op == "==":
                v = 1 if v == r else 0
            elif op == "!=":
                v = 1 if v != r else 0
            elif op == "<":
                v = 1 if v < r else 0
            elif op == ">":
                v = 1 if v > r else 0
            elif op == "<=":
                v = 1 if v <= r else 0
            else:
                v = 1 if v >= r else 0
        return v

    # -- floating constant expressions (C99 6.6p7-8) -------------------------
    # A static object's initialiser is folded here, so it must be folded
    # exactly as the running program would compute it: each value carries its
    # kind, "i" (integer), "d" (double) or "s" (float), and the usual
    # arithmetic conversions apply operator by operator.
    def _fconst_ahead(self):
        """does the constant expression starting here contain a float?"""
        depth, j = 0, self.i
        while j < len(self.tk):
            t = self.tk[j]
            if t.kind in ("(", "[", "{"):
                depth += 1
            elif t.kind in (")", "]", "}"):
                if depth == 0:
                    return False
                depth -= 1
            elif t.kind in (",", ";") and depth == 0:
                return False
            elif t.kind == "num" and isinstance(t.val, FNum):
                return True
            elif t.kind == "type" and t.text in ("float", "double"):
                return True
            j += 1
        return False

    def fconst_value(self, ty):
        """the stored bits of a constant initialiser of type ty"""
        from ..fp import bd, bs
        k, v = self.fconst(0)
        if ty.kind == "f64":
            return bd(float(v))
        if ty.kind == "f32":
            return bs(float(v))
        return int(v) if k == "i" else int(float(v))   # truncation, 6.3.1.4

    @staticmethod
    def _fround(k, v):
        from ..fp import bs, s
        return s(bs(v)) if k == "s" else v

    def fconst(self, level):
        if level == 0:
            c = self.fconst(1)
            if self.at("?"):
                self.next()
                a = self.fconst(0)
                self.expect(":")
                b = self.fconst(0)
                return a if c[1] else b
            return c
        ops = {1: ("+", "-"), 2: ("*", "/")}
        if level > 2:
            return self.fconst_atom()
        a = self.fconst(level + 1)
        while self.peek().kind in ops[level]:
            op = self.next().kind
            b = self.fconst(level + 1)
            if a[0] == "i" and b[0] == "i":
                x, y = a[1], b[1]
                r = x + y if op == "+" else x - y if op == "-" else \
                    x * y if op == "*" else (int(x / y) if y else 0)
                a = ("i", r)
                continue
            k = "d" if "d" in (a[0], b[0]) else "s"
            x, y = float(a[1]), float(b[1])
            if op == "/":
                from ..fp import _div
                r = _div(x, y)
            else:
                r = x + y if op == "+" else x - y if op == "-" else x * y
            a = (k, self._fround(k, r))
        return a

    def fconst_atom(self):
        t = self.next()
        if t.kind == "num":
            if isinstance(t.val, FNum):
                from ..fp import s
                return ("s", s(t.val.bits)) if t.val.f32 else ("d", float(t.val))
            return ("i", int(t.val))
        if t.kind == "id" and t.text in self.sc.enums:
            return ("i", self.sc.enums[t.text])
        if t.kind == "-":
            k, v = self.fconst_atom()
            return (k, -v)
        if t.kind == "+":
            return self.fconst_atom()
        if t.kind == "(":
            if self.istype(self.peek()):          # a cast
                cty = self.abstract_type()
                self.expect(")")
                k, v = self.fconst_atom()
                if cty.kind == "f64":
                    return ("d", float(v))
                if cty.kind == "f32":
                    return ("s", self._fround("s", float(v)))
                return ("i", int(v))
            v = self.fconst(0)
            self.expect(")")
            return v
        raise CError("line %d: constant expected, got %r" % (t.line, t.text))

    def const_atom(self):
        t = self.next()
        if t.kind == "num":
            return int(t.val)
        if t.kind == "id" and t.text in self.sc.enums:
            return self.sc.enums[t.text]
        if t.kind == "-":
            return -self.const_atom()
        if t.kind == "+":
            return self.const_atom()
        if t.kind == "!":
            return 0 if self.const_atom() else 1
        if t.kind == "~":
            return ~self.const_atom()
        if t.kind == "(":
            if self.istype(self.peek()):          # a cast in a constant
                self.abstract_type()              # expression: the value is
                self.expect(")")                  # unchanged, we fold in i64
                return self.const_atom()
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
        if ty.kind == "f32":
            return 4
        return ty.size(self.sc.structs) if is_narrow(ty.kind) else 8

    # -- floating point: conversions at every place C converts -------------
    @staticmethod
    def isflt(ty):
        return ty is not None and ty.kind in FLOATS

    def convto(self, frm, to):
        """ACC holds a value of type `frm`; make it a `to` (C99 6.3.1.4-5).
        Integer-to-integer is left to the store's width, as it always was."""
        if frm is None or to is None:
            return
        if self.isflt(frm) or self.isflt(to):
            self.em.conv(frm.kind, to.kind)

    def truthy(self, ty):
        """ACC holds a value about to be tested: a float becomes 0 or 1."""
        if self.isflt(ty):
            self.em.ftruth(ty.kind)

    def fone(self, ty):
        """the bits of 1.0 in a floating type"""
        from ..fp import bd, bs
        return bd(1.0) if ty.kind == "f64" else bs(1.0)

    def load_if_lval(self):
        if self.lval is not None and self.lbits is not None:
            ty, (bo, w, sg) = self.lval, self.lbits
            self.em.bits_get(bo, w, sg, self.wid(ty))
            self.lval = None
            return ty
        if self.lval is not None:
            ty = self.lval
            if ty.kind not in ("arr", "struct"):
                self.em.load(ACC, ACC, 0, self.wid(ty))
                if is_unsigned(ty.kind):
                    self.em.zext(self.wid(ty))
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
        if self._pre is not None:              # parsed already, by primary()
            ty, self._pre = self._pre, None
        else:
            ty = self.unary()
        if self.lval is not None:
            nxt = self.peek().kind
            if nxt == "=":
                self.next()
                aty, bits = self.lval, self.lbits
                self.lval = None
                self.em.push()
                rt = self.rvalue()
                self.convto(rt, aty)        # C99 6.5.16.1p2
                if bits is not None:
                    bo, w, sg = bits
                    self.em.bits_set(bo, w, sg, self.wid(aty))
                    return aty
                self.em.pop(LHS)
                if aty.kind == "struct":
                    # `b = a` copies the whole object; both sides are
                    # addresses, because load_if_lval leaves aggregates alone
                    self.em.blockcopy(LHS, ACC, aty.size(self.sc.structs))
                else:
                    self.em.store(LHS, 0, ACC, self.wid(aty))
                return aty
            if nxt in ASSIGN_OPS:
                op = ASSIGN_OPS[self.next().kind]
                aty, bits = self.lval, self.lbits
                self.lval = None
                self.em.push()               # address
                if bits is not None:
                    self.em.bits_get(bits[0], bits[1], bits[2], self.wid(aty))
                else:
                    self.em.load(ACC, ACC, 0, self.wid(aty))
                    if is_unsigned(aty.kind):
                        self.em.zext(self.wid(aty))
                self.em.push()               # old value
                rt = self.rvalue()
                if self.isflt(aty) or self.isflt(rt):
                    # `x op= y` is `x = x op y` (6.5.16.2p3): in the common
                    # type, then converted back to x's
                    ck = self.sc.combine(aty, op, rt)            # [W-4]
                    cty = F64 if ck == "f64" else (F32 if ck == "f32" else aty)
                    self.convto(rt, cty)
                    self.em.pop(LHS)
                    self.em.push(ACC)
                    self.em.emit("mov", ACC, LHS)
                    self.convto(aty, cty)
                    self.em.pop(LHS)
                    self.em.push(ACC)
                    self.em.emit("mov", ACC, LHS)
                    self.em.fbinop(op, cty.kind)
                    self.convto(cty, aty)
                elif op in ("+", "-") and aty.kind in ("ptr", "arr"):
                    self.scale(aty)
                # `x op= y` is done in the common type (6.5.16.2p3):
                # unsigned when the table says so -- `h >>= 1` on a u64
                # used to shift the sign in -- then masked to x's width
                ck = self.sc.combine(aty, "+", rt)                   # [W-4]
                uns = ck in ("u8", "u16", "u32", "u64")
                wid = {"u8": 1, "u16": 2, "u32": 4}.get(ck, 8)
                if self.isflt(aty) or self.isflt(rt):
                    pass
                elif op in ("/", "%"):
                    self.em.divmod_(op, uns, wid)
                else:
                    self.em.binop(op, uns, wid)
                if aty.kind in NARROW_UNS:
                    self.em.zext(NARROW_UNS[aty.kind])
                if bits is not None:
                    self.em.bits_set(bits[0], bits[1], bits[2], self.wid(aty))
                    return aty
                self.em.pop(LHS)
                self.em.store(LHS, 0, ACC, self.wid(aty))
                return aty
        # Not an assignment: the unary just parsed is the LEFTMOST operand of
        # the conditional expression, so hand it down instead of rewinding
        # and parsing it again.  The rewind re-parsed every operand once per
        # enclosing parenthesis -- 2^depth -- and a 52-line c-testsuite
        # program whose macros nest parentheses took 16 s to compile.
        self._pre = ty
        return self.ternary()

    def ternary(self):
        ty = self.logic_or()
        if self.at("?"):
            self.next()
            self.load_if_lval()
            self.truthy(ty)
            els, end = self.em.new_label("qelse"), self.em.new_label("qend")
            self.em.jumpz(els)
            m = self.mark()
            t1 = self.rvalue()
            self.em.jump(end)
            self.expect(":")
            self.em.label(els)
            t2 = self.rvalue()
            if (self.isflt(t1) or self.isflt(t2)) and t1.kind != t2.kind:
                # C99 6.5.15p5: the arithmetic operands meet in their common
                # type.  The first arm was emitted before the second's type
                # was known, so emit both again, converting each.
                ck = self.sc.combine(t1, "+", t2)                # [W-4]
                cty = F64 if ck == "f64" else F32
                self.rewind(m)
                self.convto(self.rvalue(), cty)
                self.em.jump(end)
                self.expect(":")
                self.em.label(els)
                self.convto(self.rvalue(), cty)
                t1 = cty
            self.em.label(end)
            return t1
        return ty

    def logic_or(self):
        ty = self.logic_and()
        while self.at("||"):
            self.next()
            self.load_if_lval()
            self.truthy(ty)
            end = self.em.new_label("orend")
            skip = self.em.new_label("orrhs")
            self.em.jumpz(skip)
            self.em.imm(ACC, 1)
            self.em.jump(end)
            self.em.label(skip)
            t2 = self.logic_and()
            self.load_if_lval()
            self.truthy(t2)
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
            self.truthy(ty)
            end = self.em.new_label("andend")
            rhs = self.em.new_label("andrhs")
            self.em.jumpz(end)
            t2 = self.binary(0)
            self.load_if_lval()
            self.truthy(t2)
            self.em.imm(LHS, 0)
            self.em.emit(self.em.recipe("alu", "ne"), ACC, ACC, LHS)
            self.em.label(end)
            ty = I32
        return ty

    def binary(self, level):
        # level 0 is `|` (prec 3): || and && are the ladder above this one
        if level >= 8:
            if self._pre is not None:            # parsed already, by assign
                t, self._pre = self._pre, None
            else:
                t = self.unary()
            self.load_if_lval()
            return t
        ty = self.binary(level + 1)
        while self._prec(self.peek().kind) == level + 3:
            self.load_if_lval()
            op = self.next().kind
            self.em.push()
            rty = self.binary(level + 1)
            self.load_if_lval()
            res = self.sc.combine(ty, op, rty)                       # [W-4]
            if self.isflt(ty) or self.isflt(rty):
                ty = self.fbinary(op, ty, rty, res)
                continue
            if op in ("+", "-") and ty.kind in ("ptr", "arr") and \
                    (rty.kind in ("i64", "u64") or is_narrow(rty.kind)):
                self.scale(ty)
            if op == "+" and rty.kind in ("ptr", "arr") and \
                    (ty.kind in ("i64", "u64") or is_narrow(ty.kind)):
                # `n + p`: the INTEGER is on the stack; scale it there
                self.em.pop(LHS)
                self.em.push(ACC)
                self.em.emit("mov", ACC, LHS)
                self.scale(rty)
                self.em.pop(LHS)
                self.em.push(ACC)
                self.em.emit("mov", ACC, LHS)
            # Signedness and wrap width both come from the TABLE: the `+` row
            # is the usual arithmetic conversion, so an unsigned result there
            # is an unsigned operation.  This used to be a hand-written copy
            # of the same rule (`unsigned_result`); enumeration over all 169
            # type pairs showed the two agree everywhere, so the copy is gone
            # -- a table-shaped decision belongs to the net, not to code.
            ck = self.sc.combine(ty, "+", rty)                       # [W-4]
            uns = ck in ("u8", "u16", "u32", "u64")
            wid = {"u8": 1, "u16": 2, "u32": 4}.get(ck, 8)
            if op in ("/", "%"):
                self.em.divmod_(op, uns, wid)
            else:
                self.em.binop(op, uns, wid)
            # C99 6.5.6p9: the difference of two pointers counts ELEMENTS.
            # An array operand decays to a pointer first, so `p - arr` is
            # the same subtraction -- it used to answer in bytes.
            if op == "-" and ty.kind in ("ptr", "arr") and \
                    rty.kind in ("ptr", "arr"):
                self.unscale(ty)
            # A u8/u16/u32 value in a register is always zero-extended: the
            # OPERANDS of a narrow unsigned op were masked (narrow_pair),
            # the RESULT was not, and `21 - (v & 1023)` -- int minus u32,
            # so a u32 -- stayed a negative 64-bit value when it was widened
            # to long.  The eight-class fuzz [S-15 A3] found it, in both
            # front ends at once.
            if res in NARROW_UNS:
                self.em.zext(NARROW_UNS[res])
            ty = self.ty_from(res, ty, rty)
        return ty

    def fbinary(self, op, ty, rty, res):
        """lhs (of type ty) on the stack, rhs (rty) in ACC, one of them
        floating.  Both go to the common type the TYPE TABLE names -- the `+`
        row is the usual arithmetic conversion -- and the op is done there."""
        if res == "illegal":
            raise CError("line %d: %s on a floating operand"
                         % (self.peek().line, op))
        ck = self.sc.combine(ty, "+", rty)                       # [W-4]
        cty = F64 if ck == "f64" else F32
        self.convto(rty, cty)                 # rhs, in ACC
        self.em.pop(LHS)                      # lhs, converted in place:
        self.em.push(ACC)                     # swap it into ACC and back
        self.em.emit("mov", ACC, LHS)
        self.convto(ty, cty)
        self.em.pop(LHS)
        self.em.push(ACC)
        self.em.emit("mov", ACC, LHS)
        self.em.fbinop(op, cty.kind)
        if op in ("+", "-", "*", "/"):
            return cty
        return I32                            # a comparison is an int

    def scale(self, pty):
        n = pty.to.size(self.sc.structs)
        if n != 1:
            self.em.imm(TMP, n)
            self.em.emit(self.em.recipe("alu", "mul"), ACC, ACC, TMP)

    def unscale(self, pty):
        n = pty.to.size(self.sc.structs)
        if n != 1:
            self.em.imm(TMP, n)
            self.em.emit3("alu", "div", ACC, ACC, TMP)

    def ty_from(self, kind, t1, t2):
        if kind == "ptr":
            return t1 if t1.kind in ("ptr", "arr") else (
                t2 if t2.kind in ("ptr", "arr") else ptr(I8))
        if kind == "illegal":
            return I64
        return {"void": VOID, "i8": I8, "i16": I16, "i32": I32, "i64": I64,
                "u8": U8, "u16": U16, "u32": U32, "u64": U64,
                "arr": t1, "struct": t1, "fn": t1}.get(kind, I64)

    def unary(self):
        p = self.ask("unary")                                    # [W-3]
        if p == "preinc":
            op = self.next().kind                # ++x is x += 1, then x
            t = self.unary()
            if self.lval is None:
                raise CError("line %d: %s needs an lvalue"
                             % (self.peek().line, op))
            ty, bits = self.lval, self.lbits
            self.lval = None
            self.em.push()                       # the address
            if bits is not None:
                self.em.bits_get(bits[0], bits[1], bits[2], self.wid(ty))
            else:
                self.em.load(ACC, ACC, 0, self.wid(ty))
                if is_unsigned(ty.kind):
                    self.em.zext(self.wid(ty))
            if self.isflt(ty):
                self.em.push(ACC)
                self.em.imm(ACC, self.fone(ty))
                self.em.fbinop("+" if op == "++" else "-", ty.kind)
            else:
                step = ty.to.size(self.sc.structs) if ty.kind == "ptr" else 1
                self.em.imm(LHS, step)
                self.em.emit(self.em.recipe("alu",
                                            "add" if op == "++" else "sub"),
                             ACC, ACC, LHS)
                if ty.kind in NARROW_UNS:
                    self.em.zext(NARROW_UNS[ty.kind])
            if bits is not None:
                self.em.bits_set(bits[0], bits[1], bits[2], self.wid(ty))
                return ty
            self.em.pop(LHS)
            self.em.store(LHS, 0, ACC, self.wid(ty))
            return ty
        if p == "bnot":
            self.next()
            t = self.unary()
            self.load_if_lval()
            self.em.bitnot()
            if t.kind == "u32":               # ~x of a u32 is a u32
                self.em.zext(4)
                return U32
            return U64 if t.kind == "u64" else I64
        if p == "neg":
            self.next()
            t = self.unary()
            self.load_if_lval()
            if self.isflt(t):
                self.em.fneg(t.kind)
            else:
                self.em.neg()
                if t.kind == "u32":           # -x of a u32 wraps in 32 bits
                    self.em.zext(4)
            return t
        if p == "uplus":
            # Unary `+` is a no-op but for the integer promotion, which the
            # binary rules apply anyway -- so, like `neg`, it just yields its
            # operand.  It shows up in real code mostly through `##` pastes
            # and defensive macros: `60 + +3`.
            self.next()
            t = self.unary()
            self.load_if_lval()
            return t
        if p == "not":
            self.next()
            t = self.unary()
            self.load_if_lval()
            self.truthy(t)
            self.em.logical_not()
            return I32
        if p == "deref":
            self.next()
            t = self.unary()
            self.load_if_lval()
            if t.kind == "ptr" and t.to is not None and t.to.kind == "fn":
                # `*f` on a function pointer is the function itself, and a
                # function designator IS its address -- there is nothing to
                # load (C99 6.5.3.2p4)
                self.lval = None
                return self.postfix_chain(t)
            self.lval = t.to if t.kind in ("ptr", "arr") else I64
            return self.postfix_chain(self.lval)
        if p == "addr":
            self.next()
            # `&f` where f is a function: there is no lvalue to take, the name
            # already denotes an address (C99 6.3.2.1p4)
            if self.at("id"):
                sy = self.sc.lookup(self.peek().text)
                if sy is not None and sy.kind == "fn":
                    self.next()
                    self.em.lea(ACC, sy.sym)
                    self.lval = None
                    return ptr(sy.ty)
            t = self.addr_operand()
            if self.lval is None:
                raise CError("line %d: & needs an lvalue" % self.peek().line)
            if self.lbits is not None:
                # C99 6.5.3.2p1: a bit-field has no address -- what ACC holds
                # is the storage unit's, which is not the same object
                raise CError("line %d: & on a bit-field"
                             % self.peek().line)
            self.lval = None
            return ptr(t)
        if p == "prim" and self.at("(") and self.istype(self.peek(1)):
            self.next()                                   # a cast ...
            base = self.abstract_type()
            self.expect(")")
            if self.at("{"):                              # ... or a compound
                return self.compound_literal(base)        # literal, C99 6.5.2.5
            t = self.unary()
            self.load_if_lval()
            self.convto(t, base)
            if self.isflt(base) or self.isflt(t):
                pass
            elif is_unsigned(base.kind):
                self.em.zext(base.size(self.sc.structs))
            elif is_narrow(base.kind):
                self.em.truncate(base.size(self.sc.structs))
            return self.postfix_chain(base)
        if p == "sizeof":
            self.next()
            # the TABLE decides whether `sizeof (` opens a type name
            a = self.sc.act("sizeof", self.peek(1)) if self.at("(") else None
            if a == "type_name":
                self.next()
                base = self.abstract_type()
                self.expect(")")
                n = base.size(self.sc.structs)
            else:
                m = self.mark()
                t = self.unary()
                n = (t or I64).size(self.sc.structs)
                end = self.i          # where the operand really ends
                vla = t is not None and t.kind == "arr" and t.n < 0
                self.rewind(m)        # drop the code it emitted ...
                self.i = end          # ... but keep the position [E-30]
                self.lval = None
                if vla:
                    # C99 6.5.3.4p2: `sizeof` a VLA is evaluated at run time
                    self.em.load(ACC, FP, -self.vla_size[self._vla_name(m)], 8)
                    return I64
            self.em.imm(ACC, n)
            return I64
        return self.primary()

    def _vla_name(self, m):
        """The identifier `sizeof` was applied to, so its size slot can be
        found.  Only a bare name is supported -- `sizeof (a[0])` on a VLA
        element is a constant anyway."""
        for j in range(m[0], len(self.tk)):
            if self.tk[j].kind == "id" and self.tk[j].text in self.vla_size:
                return self.tk[j].text
        raise CError("line %d: sizeof on a VLA expression" % self.peek().line)

    def primary(self):
        t = self.peek()
        if t.kind == "(":
            self.next()
            # Parentheses do not destroy an lvalue: `(*p)++` and `(x) = 1` are
            # ordinary C, and `(*matchlength)++` is how half of tiny-regex-c
            # is written.  The binary ladder loads at its innermost level, so
            # an expression that is a bare unary has to be recognised before
            # it goes in -- try that first and roll the emitter back if the
            # parenthesis turns out to hold more than one operand.
            ty = self.unary()
            if self.at(")") and self.lval is not None:
                self.next()
                return self.postfix_chain(ty)
            # More than one operand: what unary() parsed is the LEFTMOST of
            # them, so hand it to assign() instead of rewinding -- the rewind
            # parsed every operand again per enclosing parenthesis, 2^depth.
            self._pre = ty
            ty = self.assign()
            self.expect(")")
            return self.postfix_chain(ty)
        if t.kind == "num":
            self.next()
            if isinstance(t.val, FNum):
                # a floating constant is its bit pattern (C99 6.4.4.2p4:
                # double unless suffixed f)
                self.em.imm(ACC, t.val.bits)
                return self.postfix_chain(F32 if t.val.f32 else F64)
            self.em.imm(ACC, int(t.val))
            return self.postfix_chain(_littype(t.text, int(t.val)))
        if t.kind == "str":
            self.next()
            if _wide(t):
                self.em.lea(ACC, self.em.intern_wide(t.val))
                return self.postfix_chain(ptr(I32))
            self.em.lea(ACC, self.em.intern(t.val))
            return self.postfix_chain(ptr(I8))
        if t.kind == "id":
            name = t.text
            if self.peek(1).kind == "(":
                self.next()
                return self.call(name)
            self.want("expr", t, "lookup")
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
            elif s.ty.kind == "arr" and s.ty.n < 0:
                # a VLA lives on the tape stack; the frame slot holds its
                # ADDRESS, so load that instead of computing one
                self.em.load(ACC, FP, -s.off, 8)
                self.lval = None
                return self.postfix_chain(s.ty)
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
                self.want("field", self.peek(), "field", declared=True)
                fname = self.expect("id").text
                if arrow:
                    self.load_if_lval()
                    sty = ty.to
                else:
                    sty = self.lval if self.lval is not None else ty
                    self.lval = None
                st = self.sc.structs.get(sty.tag) if sty.tag else None
                if st is None or fname not in st.fields:
                    raise CError("line %d: %r has no member %r"
                                 % (self.peek().line, sty, fname))
                fty, foff = st.fields[fname]
                if foff:
                    self.em.imm(TMP, foff)
                    self.em.emit(self.em.recipe("alu", "add"), ACC, ACC, TMP)
                self.lval = fty
                self.lbits = st.bits.get(fname)
                ty = fty
            elif p == "inc":
                op = self.next().kind
                aty = self.lval
                if aty is None:
                    raise CError("line %d: %s needs an lvalue"
                                 % (self.peek().line, op))
                bits = self.lbits
                self.lval = None
                self.em.push()
                if bits is not None:
                    self.em.bits_get(bits[0], bits[1], bits[2], self.wid(aty))
                else:
                    self.em.load(ACC, ACC, 0, self.wid(aty))
                if self.isflt(aty):
                    # (x + 1) - 1 is not x in floating point: keep the old
                    # value itself.  Stack: address, old value.
                    self.em.push()
                    self.em.push()
                    self.em.imm(ACC, self.fone(aty))
                    self.em.fbinop("+" if op == "++" else "-", aty.kind)
                    self.em.load(LHS, SP, 8)
                    self.em.store(LHS, 0, ACC, self.wid(aty))
                    self.em.pop(ACC)
                    self.em.frame(-8)
                    ty = aty
                    continue
                self.em.push()
                step = aty.to.size(self.sc.structs) if aty.kind == "ptr" else 1
                self.em.imm(ACC, step)
                self.em.binop("+" if op == "++" else "-")
                if bits is not None:
                    self.em.bits_set(bits[0], bits[1], bits[2], self.wid(aty))
                else:
                    self.em.pop(LHS)
                    self.em.store(LHS, 0, ACC, self.wid(aty))
                self.em.imm(TMP, step)
                self.em.emit(self.em.recipe("alu",
                                            "sub" if op == "++" else "add"),
                             ACC, ACC, TMP)
                ty = aty
            elif p == "call":
                ty = self.call_value(ty)
            else:
                return ty

    def abstract_type(self):
        """A type name with no identifier: `int *`, `char[8]`,
        `void (*)(void)`.  Same grammar as a declarator, minus the name."""
        base = self.declspec()
        ty, _ = self.declarator(base, named=False)
        return ty

    def addr_operand(self):
        """The operand of `&`, peeling redundant parentheses.

        `&(p->b)` has to keep its lvalue, and a parenthesised expression
        normally reaches `binary()`, whose innermost level calls
        `load_if_lval()` -- it loads the value and the address is gone.  That
        is why `offsetof` did not compile."""
        # a `(` that starts a type is a cast or a compound literal, not a
        # redundant parenthesis -- `&(struct P *)0` must not be peeled
        if self.at("(") and not self.istype(self.peek(1)):
            m = self.mark()
            self.next()
            t = self.addr_operand()
            if self.lval is not None and self.at(")"):
                self.next()
                return t
            self.rewind(m)
            self.lval = None
        return self.unary()

    def static_compound(self):
        """`&(struct S){1, 2}` in a STATIC initialiser.  At file scope the
        literal has static storage duration, so it becomes an anonymous global
        and the slot gets its address at `_start` like any other pointer."""
        self.expect("&")
        self.expect("(")
        ty = self.abstract_type()
        self.expect(")")
        if ty.kind == "arr" and ty.n == 0:
            ty = Type("arr", to=ty.to, n=self._init_count(ty.to))
        lab = "g_cl%d" % self.i
        self.em.t.string(lab, b"\x00" * max(1, ty.size(self.sc.structs)),
                         align=8)
        self.const_init(lab, ty)
        return lab

    def compound_literal(self, ty):
        """`(struct S){1, 2}` -- an unnamed object with the enclosing block's
        storage duration, initialised in place.  It is an LVALUE, so `&` and
        `.field` work on it, which is the whole point of the construct."""
        if self.ret_label is None:
            raise CError("line %d: compound literal outside a function"
                         % self.peek().line)
        if ty.kind == "arr" and ty.n == 0:
            ty = Type("arr", to=ty.to, n=self._init_count(ty.to))
        off = self.alloc(ty)
        self.local_init(ty, off)
        self.em.imm(TMP, off)
        self.em.emit(self.em.recipe("alu", "sub"), ACC, FP, TMP)
        self.lval = ty
        return self.postfix_chain(ty)

    def call_value(self, ty):
        """A call whose callee is an EXPRESSION, not a name: `go()()`, or
        `p->fn()`.  The address is already in ACC, so it goes on the tape stack
        underneath the arguments and comes back off last."""
        self.expect("(")
        self.load_if_lval()
        self.em.push()                          # the callee, below the args
        args = 0
        while not self.at(")"):
            self.rvalue()
            self.em.push()
            args += 1
            if not self.eat(","):
                break
        self.expect(")")
        fty = ty.to if ty is not None and ty.kind == "ptr" else ty
        variadic = fty is not None and fty.kind == "fn" and fty.n == 1
        if args > len(ARGREGS) or variadic:
            # the callee sits above the block; reverse the block so arg[k] is
            # at [FP+16+8k], then fetch the callee from above it [W-13]
            for i in range(args // 2):
                j = args - 1 - i
                self.em.load(TMP, SP, 8 * i)
                self.em.load(LHS, SP, 8 * j)
                self.em.store(SP, 8 * i, LHS)
                self.em.store(SP, 8 * j, TMP)
            self.em.load(CALLEE, SP, 8 * args)
            self.em.call_reg(CALLEE)
            self.em.frame(-8 * (args + 1))
        else:
            for k in range(args - 1, -1, -1):
                self.em.pop(ARGREGS[k])
                self.em.arg(k, ARGREGS[k])
            self.em.pop(CALLEE)
            self.em.call_reg(CALLEE)
        rt = I64
        if ty is not None:
            if ty.kind == "ptr" and ty.to is not None and ty.to.kind == "fn":
                rt = ty.to.ret or I64
            elif ty.kind == "fn":
                rt = ty.ret or I64
        return rt

    def call(self, name):
        self.expect("(")
        if name == "printf" and self.at("str") \
                and not self._rt_format(self.peek().val):
            return self.printf()          # [W-9] static format string
        if name in ("va_start", "va_arg", "va_end"):
            return self.va(name)
        if name in ("__builtin_sqrt", "__builtin_sqrtf"):
            # the hardware square root: correctly rounded on both ISAs, and
            # what <math.h>'s sqrt is (C99 F.9.4.5)
            fty = F32 if name.endswith("f") else F64
            self.convto(self.rvalue(), fty)
            self.expect(")")
            self.em.emit(self.em.recipe("fpu", "dsqrt" if fty is F64
                                        else "ssqrt"), ACC, ACC)
            return self.postfix_chain(fty)
        if name in INTRINSIC:
            return self.intrinsic(INTRINSIC[name])
        if name in INTRINSIC6:
            return self.intrinsic(INTRINSIC6[name])
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
        indirect = s0 is not None and s0.kind != "fn"
        if indirect:
            # Push the POINTER VALUE, do not keep it in a register: a nested
            # call while the arguments are being evaluated would clobber it.
            if s0.kind == "global":
                self.em.lea(ACC, s0.sym)
            else:
                self.em.imm(TMP, s0.off)
                self.em.emit(self.em.recipe("alu", "sub"), ACC, FP, TMP)
            self.em.load(ACC, ACC, 0)
            self.em.push()
        sret_ty = None
        s_pre = self.sc.lookup(name)
        if s_pre is not None and s_pre.ty.kind == "fn" \
                and s_pre.ty.ret is not None and s_pre.ty.ret.kind == "struct":
            sret_ty = s_pre.ty.ret
        args = []
        if sret_ty is not None:
            # the hidden destination goes first, so it is argument 0
            toff = self.alloc(sret_ty)
            self.em.imm(ACC, toff)
            self.em.emit(self.em.recipe("alu", "sub"), ACC, FP, ACC)
            self.em.push()
            args.append(1)
        pty = s0.ty if s0 is not None else None
        if pty is not None and pty.kind == "ptr" and pty.to is not None:
            pty = pty.to
        plist = pty.params if pty is not None and pty.kind == "fn" else None
        k = 0
        while not self.at(")"):
            at = self.rvalue()
            self.argconv(at, plist, k)
            self.em.push()
            args.append(1)
            k += 1
            if not self.eat(","):
                break
        self.expect(")")
        s = s0
        fty = s.ty if s is not None else None
        if fty is not None and fty.kind == "ptr" and fty.to is not None:
            fty = fty.to
        variadic = fty is not None and fty.kind == "fn" and fty.n == 1
        if fty is None and name in VARIADIC_LIBC:
            variadic = True
        stacked = len(args) > len(ARGREGS) or variadic
        if stacked:
            # Pushing in source order leaves arg[n-1] nearest the return
            # address.  Reverse the block so arg[k] is always at [FP+16+8k],
            # a layout the callee can index without knowing n -- which is
            # what varargs will need. [W-13]
            n = len(args)
            for i in range(n // 2):
                j = n - 1 - i
                self.em.load(TMP, SP, 8 * i)
                self.em.load(LHS, SP, 8 * j)
                self.em.store(SP, 8 * i, LHS)
                self.em.store(SP, 8 * j, TMP)
        if not stacked:
            # Args are popped straight into their own registers, highest
            # first, so a pop can never clobber one already placed.
            for k in range(len(args) - 1, -1, -1):
                self.em.pop(ARGREGS[k])
                self.em.arg(k, ARGREGS[k])
        if indirect:
            if stacked:
                self.em.load(CALLEE, SP, 8 * len(args))
            else:
                self.em.pop(CALLEE)
            self.em.call_reg(CALLEE)
        else:
            tgt = s0.sym if s0 is not None and s0.kind == "fn" else name
            self.pending.setdefault(tgt, self.peek().line)
            self.em.call(tgt)
        if stacked:
            self.em.frame(-8 * (len(args) + (1 if indirect else 0)))
        if sret_ty is not None:
            self.em.imm(ACC, toff)
            self.em.emit(self.em.recipe("alu", "sub"), ACC, FP, ACC)
            self.lval = None
            return self.postfix_chain(sret_ty)
        rt = I64
        if s is not None:
            t = s.ty
            if t.kind == "ptr" and t.to is not None and t.to.kind == "fn":
                rt = t.to.ret
            elif t.kind == "fn":
                rt = t.ret
        return self.postfix_chain(rt)

    def argconv(self, at, plist, k):
        """An argument to parameter k: converted to the parameter's type when
        a prototype names one, else the default argument promotions -- float
        becomes double (C99 6.5.2.2p6-7), which is what `...` receives."""
        if plist is not None and k < len(plist):
            self.convto(at, plist[k])
        elif at is not None and at.kind == "f32":
            self.convto(at, F64)

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
        wide = op in INTRINSIC6.values()
        m = 6 if wide else 3
        if n > m:
            raise CError("line %d: %s takes at most %d arguments"
                         % (self.peek().line, op, m))
        regs = SYSREGS[:m]
        for k in range(n - 1, -1, -1):
            self.em.pop(regs[k])
        for k in range(n, m):
            self.em.imm(regs[k], 0)
        self.em.emit(".sys6" if wide else ".sys", op, *regs)
        return I64

    def va(self, which):
        """va_start / va_arg / va_end.  An argument list this subset can walk
        exists only because a variadic function takes everything on the stack:
        arg[k] is at [FP + 16 + 8k], so the first variadic one is at a known
        offset and `va_list` is just a pointer to it. [W-15]"""
        if which == "va_end":
            self.rvalue()
            self.expect(")")
            self.em.imm(ACC, 0)
            return I32
        if which == "va_start":
            if not getattr(self, "fn_vararg", False):
                raise CError("line %d: va_start outside a variadic function"
                             % self.peek().line)
            self.unary()                       # the va_list lvalue
            if self.lval is None:
                raise CError("line %d: va_start needs an lvalue"
                             % self.peek().line)
            self.lval = None
            self.em.push()
            if self.eat(","):
                self.rvalue()                  # the last named parameter
            self.expect(")")
            self.em.imm(ACC, 16 + 8 * self.fn_nfixed)
            self.em.emit(self.em.recipe("alu", "add"), ACC, FP, ACC)
            self.em.pop(LHS)
            self.em.store(LHS, 0, ACC)
            self.em.imm(ACC, 0)
            return I32
        # va_arg(ap, T): read *ap as T, then step ap on by one slot
        self.unary()
        if self.lval is None:
            raise CError("line %d: va_arg needs an lvalue" % self.peek().line)
        self.lval = None
        self.expect(",")
        ty = self.abstract_type()
        self.expect(")")
        self.em.push()                         # &ap
        self.em.load(ACC, ACC, 0)              # ap
        self.em.push()                         # ap
        self.em.load(ACC, ACC, 0, self.wid(ty))
        if is_unsigned(ty.kind):
            self.em.zext(self.wid(ty))
        self.em.pop(LHS)                       # ap
        self.em.push()                         # value
        self.em.imm(ACC, 8)
        self.em.emit(self.em.recipe("alu", "add"), ACC, LHS, ACC)
        self.em.pop(LHS)                       # value
        self.em.push()                         # ap + 8
        self.em.emit("mov", TMP, LHS)          # keep the value
        self.em.pop(ACC)                       # ap + 8
        self.em.pop(LHS)                       # &ap
        self.em.store(LHS, 0, ACC)
        self.em.emit("mov", ACC, TMP)
        return ty

    @staticmethod
    def _rt_format(fmt):
        """Does this literal format need the RUNTIME formatter in <stdio.h>?
        The walker desugars integers and strings; a floating conversion, a
        sign flag, `#`, a `*` width, or zero padding of a signed value go to
        _u_vfmt, which does them (and exactly -- see _u_ffmt)."""
        if not isinstance(fmt, str):
            return False
        i = 0
        while i < len(fmt):
            if fmt[i] != "%":
                i += 1
                continue
            i += 1
            if i < len(fmt) and fmt[i] == "%":
                i += 1
                continue
            flags = ""
            while i < len(fmt) and fmt[i] in "-+ #0":
                flags += fmt[i]
                i += 1
            if i < len(fmt) and fmt[i] == "*":
                return True
            while i < len(fmt) and (fmt[i].isdigit() or fmt[i] == "."):
                i += 1
            if i < len(fmt) and fmt[i] == "*":
                return True
            mods = ""
            while i < len(fmt) and fmt[i] in "hlLzjt":
                mods += fmt[i]
                i += 1
            spec = fmt[i] if i < len(fmt) else ""
            if spec in "fFeEgGaA" or any(f in flags for f in "+ #"):
                return True
            # a 64-bit unsigned conversion: the desugared %u is 32 bits and
            # prints signed, so %lu of 2^63 and up came out wrong
            if spec in "uxXo" and any(m in mods for m in "lzjt"):
                return True
            if "0" in flags and spec in "di":
                return True
            i += 1
        return False

    def _fmt_parts(self, fmt):
        """Split a format string into literals and conversions."""
        parts, lit, i = [], "", 0
        while i < len(fmt):
            if fmt[i] != "%":
                lit += fmt[i]
                i += 1
                continue
            i += 1
            if i < len(fmt) and fmt[i] == "%":
                lit += "%"
                i += 1
                continue
            left = zero = False
            while i < len(fmt) and fmt[i] in "-+ #0":
                left |= fmt[i] == "-"
                zero |= fmt[i] == "0"
                i += 1
            width = 0
            while i < len(fmt) and fmt[i].isdigit():
                width = width * 10 + int(fmt[i])
                i += 1
            prec = None
            if i < len(fmt) and fmt[i] == ".":
                i += 1
                prec = 0
                while i < len(fmt) and fmt[i].isdigit():
                    prec = prec * 10 + int(fmt[i])
                    i += 1
            while i < len(fmt) and fmt[i] in "hlLzjt":
                i += 1
            if i >= len(fmt):
                raise CError("printf: format ends in a conversion")
            spec = fmt[i]
            i += 1
            parts.append((lit, spec, width, left, zero, prec))
            lit = ""
        parts.append((lit, None, 0, False, False, None))
        return parts

    def printf(self):
        """[W-9] desugared against the static format string.

        Every argument is evaluated BEFORE anything is written, into a frame
        slot of its own.  Interleaving them with the output was observable:
        `printf("a %d\n", f())` printed `a ` before calling f, so a call that
        printed something of its own came out in the wrong order."""
        t = self.expect("str")
        parts = self._fmt_parts(t.val)
        slots = []
        for (_, spec, _, _, _, _) in parts:
            if spec is None:
                break
            self.expect(",")
            self.rvalue()
            off = self.alloc(I64)
            self.em.store(FP, -off, ACC)
            slots.append(off)
        self.expect(")")
        k = 0
        for (lit, spec, width, left, zero, prec) in parts:
            self.em.write_literal(lit)
            if spec is None:
                break
            self.em.load(ACC, FP, 0 - slots[k])
            k += 1
            if prec is not None and spec in "diuxXop":
                # C99 7.19.6.1p5: for an integer conversion the precision is
                # the MINIMUM number of digits -- `%.2x` of 3 is "03".  That
                # is zero padding, and it is what a hex dump is made of.
                if width > prec:
                    raise CError(
                        "line %d: printf %%%d.%d%s -- a field wider than the "
                        "precision needs spaces outside the zeros, which the "
                        "desugared printf does not emit"
                        % (self.peek().line, width, prec, spec))
                width, zero, left = prec, True, False
            if spec not in PFCONVS:
                raise CError("printf: unsupported %%%s" % spec)
            how = self.o.ask("pfconv", (spec,))       # which routine [C-10]
            if how == "u32":
                self.em.mask32()
                how = "int"
            if how == "str":
                self.em.print_field("str", width, left, False, prec)
            elif how == "chr":
                self.em.print_field("chr", width, left, False)
            else:
                self.em.print_field(how, width, left, zero)
        self.em.imm(ACC, 0)
        return I32


def compile_units(streams, oracle):
    """Several translation units, one program.

    There is no linker and no object format: the units are walked in turn by
    ONE walker, so a call in the first file reaches a definition in the last
    exactly the way it reaches one further down its own file -- `called` is
    already resolved at the end, not at the call.  What the units do NOT get
    is separate scope: a typedef or a struct tag from an earlier file is
    still visible in a later one.  That is wrong C, and it is the first thing
    to fix when a real program trips over it."""
    w = Walker(streams[0], oracle)
    multi = len(streams) > 1
    for k, toks in enumerate(streams):
        w.tk, w.i = toks, 0
        w.unit_tag = "_u%d" % k if multi else ""
        w.unit_start = len(w.em.t.code)
        w.renames = {}
        w.pending = {}
        try:
            w.tu()
        except CError as e:
            # Thirty-odd raise sites say "line N" from their own token; the
            # one place every error passes through attaches the token the
            # walker stood on, so the driver can name the USER's file,
            # line and column instead of a line in the spliced buffer.
            if not hasattr(e, "tok") and w.tk:
                e.tok = w.tk[min(w.i, len(w.tk) - 1)]
                e.unit = k
            raise
        if w.errors:
            for e in w.errors:
                e.unit = k
            raise CErrors(w.errors)
    tape = w.finish_program()
    if "main" not in tape.labels:
        raise CError("no main()")
    return tape
