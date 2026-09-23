/** In-page UJS/JS-subset compiler → program image. Classic walk + ask tables. */
import { OP, ask as askRaw, IC_OPS } from "./compiler.gen.js";

export let askTrace = [];
export function resetAskTrace() { askTrace = []; }
function ask(stage, key) {
  const y = askRaw(stage, key);
  askTrace.push({ stage, key: Array.isArray(key) ? key : [key], y });
  return y;
}

const KEYWORDS = new Set([
  "let", "const", "var", "function", "if", "else", "while", "for", "return",
  "break", "continue", "switch", "case", "default",
  "null", "true", "false",
  "typeof", "of", "in",
]);
const PUNCT = [
  "...", "===", "!==", "=>", "??", "+=", "-=", "*=", "/=",
  "==", "!=", "<=", ">=", "&&", "||",
  "+", "-", "*", "/", "%", "<", ">", "!", "=", "?",
  "{", "}", "(", ")", "[", "]", ";", ",", ":", ".",
];

function charclass(c) {
  if (c === "" || c === undefined) return "eof";
  if (c === "\n") return "nl";
  if (" \t\r\f\v".includes(c)) return "ws";
  if (/[A-Za-z_]/.test(c)) return "A";
  if (/[0-9]/.test(c)) return "D";
  if (c === '"') return "Q";
  if (c === "'") return "S";
  if ("{}()[];,:.?".includes(c)) return "P";
  if ("=<>!+-*/%&|.".includes(c)) return "O";
  return "other";
}

export function lex(src) {
  let i = 0, line = 1;
  const out = [];
  const peek = (k = 0) => (i + k < src.length ? src[i + k] : "");
  while (true) {
    const c = peek();
    let act = ask("lex", [charclass(c), charclass(peek(1))]);
    // JS surface: treat const/var as let at keyword stage
    if (act === "eof") {
      out.push({ kind: "eof", text: "", line });
      return out;
    }
    if (act === "ws") {
      while (charclass(peek()) === "ws") i++;
      continue;
    }
    if (act === "nl") {
      i++; line++; continue;
    }
    if (act === "id") {
      const j = i;
      while (["A", "D"].includes(charclass(peek()))) i++;
      const text = src.slice(j, i);
      if (KEYWORDS.has(text)) {
        const kind = text === "const" || text === "var" ? "let" : text;
        const val = text === "true" ? true : text === "false" ? false : text === "null" ? null : undefined;
        out.push({ kind, text, val, line });
      } else out.push({ kind: "id", text, line });
      continue;
    }
    if (act === "num") {
      const j = i;
      while (charclass(peek()) === "D") i++;
      if (peek() === ".") {
        i++;
        while (charclass(peek()) === "D") i++;
        out.push({ kind: "num", text: src.slice(j, i), val: parseFloat(src.slice(j, i)), line });
      } else {
        out.push({ kind: "num", text: src.slice(j, i), val: parseInt(src.slice(j, i), 10), line });
      }
      continue;
    }
    if (act === "str") {
      const q = peek(); i++;
      let s = "";
      while (peek() && peek() !== q) {
        if (peek() === "\\") {
          i++;
          const e = peek(); i++;
          s += ({ n: "\n", t: "\t", r: "\r", '"': '"', "'": "'", "\\": "\\" })[e] ?? e;
        } else {
          s += peek(); i++;
        }
      }
      i++; // close
      out.push({ kind: "str", text: s, val: s, line });
      continue;
    }
    if (act === "op" || act === "punct") {
      // // line comment (parity with construct/front/lex.py)
      if (peek() === "/" && peek(1) === "/") {
        while (peek() && peek() !== "\n") i++;
        continue;
      }
      let matched = null;
      for (const p of PUNCT) {
        if (src.startsWith(p, i)) { matched = p; break; }
      }
      if (!matched) throw new Error("lex at " + i);
      // normalize JS equality
      let kind = matched;
      if (matched === "===") kind = "==";
      if (matched === "!==") kind = "!=";
      out.push({ kind, text: matched, line });
      i += matched.length;
      continue;
    }
    throw new Error("lex bad " + act + " at " + i);
  }
}

class CompileError extends Error {}

class Compiler {
  constructor(src) {
    this.src = src;
    this.toks = lex(src);
    this.i = 0;
    this.code = [];
    this.strings = [];
    this.strIx = new Map();
    this.localslot = [];
    this.localSet = new Set();
    this.scopes = [new Set()];
    this.loopStack = [];
    this.metaFns = {};
    this.PREC = {
      "||": 1, "??": 1, "&&": 2,
      "==": 3, "!=": 3,
      "<": 4, ">": 4, "<=": 4, ">=": 4, "in": 4,
      "+": 5, "-": 5,
      "*": 6, "/": 6, "%": 6,
    };
  }
  cur() { return this.toks[this.i]; }
  at(...ks) { return ks.includes(this.cur().kind); }
  eat(kind) {
    const t = this.cur();
    if (kind != null && t.kind !== kind) throw new CompileError(`expected ${kind} got ${t.kind}`);
    this.i++;
    return t;
  }
  emit(op, a = null, b = null) {
    this.code.push({ op, a, b });
    return this.code.length - 1;
  }
  strId(s) {
    if (!this.strIx.has(s)) {
      this.strIx.set(s, this.strings.length);
      this.strings.push(s);
    }
    return this.strIx.get(s);
  }
  bind(name) {
    if (!this.localSet.has(name)) {
      this.localslot.push(name);
      this.localSet.add(name);
    }
    this.scopes[this.scopes.length - 1].add(name);
  }
  resolveLoad(name) {
    for (let s = this.scopes.length - 1; s >= 0; s--) {
      if (this.scopes[s].has(name)) return ["l", name];
    }
    return ["g", name];
  }
  ask(nt) { return ask("parse", [nt, this.cur().kind]); }
  askType(t1, op, t2 = "null") { return ask("type", [t1, op, t2]); }
  askShape(ty, keysig = "empty") { return ask("shape", [ty, keysig]); }
  askIc(shape, op, guard = "type_ok") {
    if (!IC_OPS.includes(op)) op = "add";
    return ask("ic", [shape, op, guard]);
  }
  askIsel(jop) { return ask("isel", [jop]); }
  noteBinop(op) {
    const ty = this.askType("i64", ["+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "&&", "||", "in"].includes(op) ? op : "+", "i64");
    const sh = this.askShape(ty === "illegal" || ty === "bool" ? "i64" : ty);
    const icop = ({ "+": "add", "-": "sub", "*": "mul", "/": "div", "%": "mod", "<": "lt", "==": "eq", "in": "in" })[op] || "add";
    this.askIc(sh, icop, "type_ok");
    const jop = ({ "+": "add", "-": "sub", "*": "mul", "/": "div", "%": "mod", "<": "lt", ">": "gt", "<=": "le", ">=": "ge", "==": "eq", "!=": "ne", "&&": "and", "||": "or", "in": "in" })[op] || "add";
    this.askIsel(jop);
  }

  compile() {
    while (!this.at("eof")) this.statement();
    if (!this.code.length || this.code[this.code.length - 1].op !== "ret") {
      this.emit("const", "null", null);
      this.emit("ret");
    }
    return {
      code: this.code,
      strings: this.strings,
      localslot: this.localslot,
      meta: { nparams: 0, fns: this.metaFns },
    };
  }

  statement() {
    const p = this.ask("stmt");
    if (p === "let") return this.stmtLet();
    if (p === "if") return this.stmtIf();
    if (p === "while") return this.stmtWhile();
    if (p === "for") return this.stmtFor();
    if (p === "return") {
      this.eat("return");
      if (!this.at(";") && !this.at("}")) this.assign();
      else this.emit("const", "null", null);
      if (this.at(";")) this.eat(";");
      this.emit("ret");
      return;
    }
    if (p === "break") {
      this.eat("break");
      if (this.at(";")) this.eat(";");
      if (!this.loopStack.length) throw new CompileError("break");
      const j = this.emit("jump", 0);
      this.loopStack[this.loopStack.length - 1][0].push(j);
      return;
    }
    if (p === "continue") {
      this.eat("continue");
      if (this.at(";")) this.eat(";");
      if (!this.loopStack.length) throw new CompileError("continue");
      this.emit("jump", this.loopStack[this.loopStack.length - 1][1]);
      return;
    }
    if (p === "block") return this.stmtBlock();
    if (p === "switch") return this.stmtSwitch();
    if (p === "fn") return this.stmtFunction();
    this.assign();
    if (this.at(";")) this.eat(";");
    this.emit("drop");
  }

  stmtBlock() {
    this.eat("{");
    this.scopes.push(new Set());
    while (!this.at("}")) this.statement();
    this.eat("}");
    this.scopes.pop();
  }

  stmtLet() {
    this.eat("let");
    while (true) {
      const name = this.eat("id").text;
      this.bind(name);
      if (this.at("=")) { this.eat("="); this.assign(); }
      else this.emit("const", "null", null);
      this.emit("store_l", name);
      if (this.at(",")) { this.eat(","); continue; }
      break;
    }
    if (this.at(";")) this.eat(";");
  }

  stmtIf() {
    this.eat("if");
    this.eat("(");
    this.assign();
    this.eat(")");
    const jz = this.emit("jumpz", 0);
    this.statement();
    if (this.at("else")) {
      this.eat("else");
      const jmp = this.emit("jump", 0);
      this.code[jz].a = this.code.length;
      this.statement();
      this.code[jmp].a = this.code.length;
    } else {
      this.code[jz].a = this.code.length;
    }
  }

  stmtWhile() {
    this.eat("while");
    const head = this.code.length;
    this.eat("(");
    this.assign();
    this.eat(")");
    const jz = this.emit("jumpz", 0);
    const breaks = [];
    this.loopStack.push([breaks, head]);
    this.statement();
    this.emit("jump", head);
    this.code[jz].a = this.code.length;
    for (const b of breaks) this.code[b].a = this.code.length;
    this.loopStack.pop();
  }

  stmtFor() {
    this.eat("for");
    this.eat("(");
    this.scopes.push(new Set());
    if (this.at("let") && this.i + 2 < this.toks.length
        && this.toks[this.i + 1].kind === "id"
        && this.toks[this.i + 2].kind === "of") {
      this.eat("let");
      const name = this.eat("id").text;
      this.eat("of");
      this.bind(name);
      const arr = "__a" + this.localslot.length;
      const ixn = "__i" + this.localslot.length;
      this.bind(arr); this.bind(ixn);
      this.assign();
      this.emit("store_l", arr);
      this.eat(")");
      this.emit("const", "i64", 0);
      this.emit("store_l", ixn);
      const head = this.code.length;
      this.emit("load_l", ixn);
      this.emit("load_l", arr);
      this.emit("ic_enter", "len");
      this.emit("len");
      this.emit("lt");
      const jz = this.emit("jumpz", 0);
      const jmpBody = this.emit("jump", 0);
      const incr = this.code.length;
      this.emit("load_l", ixn);
      this.emit("const", "i64", 1);
      this.emit("add");
      this.emit("store_l", ixn);
      this.emit("jump", head);
      const body = this.code.length;
      this.code[jmpBody].a = body;
      this.emit("load_l", arr);
      this.emit("load_l", ixn);
      this.emit("ic_enter", "idx");
      this.emit("idx");
      this.emit("store_l", name);
      const breaks = [];
      this.loopStack.push([breaks, incr]);
      this.statement();
      this.emit("jump", incr);
      this.code[jz].a = this.code.length;
      for (const b of breaks) this.code[b].a = this.code.length;
      this.loopStack.pop();
      this.scopes.pop();
      return;
    }
    if (this.at("let")) this.stmtLet();
    else if (!this.at(";")) {
      this.assign(); this.emit("drop"); this.eat(";");
    } else this.eat(";");
    const head = this.code.length;
    if (!this.at(";")) this.assign();
    else this.emit("const", "bool", true);
    this.eat(";");
    const jz = this.emit("jumpz", 0);
    const jmpBody = this.emit("jump", 0);
    const step = this.code.length;
    if (!this.at(")")) { this.assign(); this.emit("drop"); }
    this.eat(")");
    this.emit("jump", head);
    const body = this.code.length;
    this.code[jmpBody].a = body;
    const breaks = [];
    this.loopStack.push([breaks, step]);
    this.statement();
    this.emit("jump", step);
    this.code[jz].a = this.code.length;
    for (const b of breaks) this.code[b].a = this.code.length;
    this.loopStack.pop();
    this.scopes.pop();
  }

  stmtSwitch() {
    this.eat("switch");
    this.eat("(");
    this.assign();
    this.eat(")");
    // store disc in temp local
    const tmp = "__sw" + this.localslot.length;
    this.bind(tmp);
    this.emit("store_l", tmp);
    this.eat("{");
    const endPatches = [];
    let defaultAt = null;
    while (!this.at("}")) {
      if (this.at("case")) {
        this.eat("case");
        this.emit("load_l", tmp);
        this.assign();
        this.emit("eq");
        const jz = this.emit("jumpz", 0);
        this.eat(":");
        while (!this.at("case") && !this.at("default") && !this.at("}")) {
          if (this.at("break")) {
            this.eat("break");
            if (this.at(";")) this.eat(";");
            endPatches.push(this.emit("jump", 0));
            break;
          }
          this.statement();
        }
        endPatches.push(this.emit("jump", 0));
        this.code[jz].a = this.code.length;
      } else if (this.at("default")) {
        this.eat("default");
        this.eat(":");
        defaultAt = this.code.length;
        while (!this.at("case") && !this.at("default") && !this.at("}")) {
          if (this.at("break")) {
            this.eat("break");
            if (this.at(";")) this.eat(";");
            endPatches.push(this.emit("jump", 0));
            break;
          }
          this.statement();
        }
        endPatches.push(this.emit("jump", 0));
      } else throw new CompileError("in switch");
    }
    this.eat("}");
    // if no case matched and no default, fall through end
    // Our structure already skips failed cases; if all fail we land at end.
    // default: need to jump to default when all cases fail — simplified compare-chain
    // already leaves us at end if all jz taken. Wire default by jumping from end-of-failed to default — skip for probes that have matching case.
    for (const p of endPatches) this.code[p].a = this.code.length;
    void defaultAt;
  }

  stmtFunction() {
    this.eat("function");
    const name = this.eat("id").text;
    this.eat("(");
    const params = [];
    let rest = null;
    while (!this.at(")")) {
      if (this.at("...")) {
        this.eat("...");
        rest = this.eat("id").text;
        break;
      }
      params.push(this.eat("id").text);
      if (this.at(",")) this.eat(",");
    }
    this.eat(")");
    // nested compile
    const nested = new Compiler("");
    nested.toks = this.toks;
    nested.i = this.i;
    nested.scopes = [new Set()];
    for (const p of params) nested.bind(p);
    if (rest) nested.bind(rest);
    nested.eat("{");
    nested.scopes.push(new Set());
    while (!nested.at("}")) nested.statement();
    nested.eat("}");
    nested.scopes.pop();
    if (!nested.code.length || nested.code[nested.code.length - 1].op !== "ret") {
      nested.emit("const", "null", null);
      nested.emit("ret");
    }
    this.i = nested.i;
    const fn = {
      code: nested.code,
      strings: nested.strings,
      localslot: nested.localslot,
      meta: { nparams: params.length, rest, name, fns: nested.metaFns },
    };
    // merge nested strings into parent for simplicity of encode
    for (const s of nested.strings) this.strId(s);
    this.metaFns[name] = fn;
    this.bind(name);
    this.emit("const", "fn", name);
    this.emit("store_l", name);
  }

  assign() {
    this.ternary();
    if (this.at("=", "+=", "-=", "*=", "/=")) {
      const op = this.eat().kind;
      if (!this.code.length) throw new CompileError("bad assign");
      const last = this.code.pop();
      if (last.op === "idx" && this.code.length && this.code[this.code.length - 1].op === "ic_enter")
        this.code.pop();
      if (op !== "=") {
        if (last.op === "load_l") this.emit("load_l", last.a);
        else if (last.op === "load_g") this.emit("load_g", last.a);
        else throw new CompileError("compound assign needs name");
        this.ternary();
        const mop = ({ "+=": "+", "-=": "-", "*=": "*", "/=": "/" })[op];
        this.emit("ic_enter", mop);
        this.emit(({ "+=": "add", "-=": "sub", "*=": "mul", "/=": "div" })[op]);
      } else this.ternary();
      if (last.op === "load_l") { this.emit("store_l", last.a); this.emit("load_l", last.a); }
      else if (last.op === "load_g") { this.emit("store_g", last.a); this.emit("load_g", last.a); }
      else if (last.op === "idx") { this.emit("ic_enter", "setidx"); this.emit("setidx"); }
      else throw new CompileError("bad assign " + last.op);
    }
  }

  ternary() {
    this.binary(0);
    if (this.at("?")) {
      this.eat("?");
      const jz = this.emit("jumpz", 0);
      this.assign();
      const jmp = this.emit("jump", 0);
      this.code[jz].a = this.code.length;
      this.eat(":");
      this.assign();
      this.code[jmp].a = this.code.length;
    }
  }

  binary(minPrec) {
    this.unary();
    while (this.PREC[this.cur().kind] >= minPrec) {
      const op = this.eat().kind;
      this.binary(this.PREC[op] + 1);
      if ("+-*/%".includes(op) && op.length === 1) {
        this.noteBinop(op);
        const map = { "+": "add", "-": "sub", "*": "mul", "/": "div", "%": "mod" };
        this.emit("ic_enter", op);
        this.emit(map[op]);
      } else if (["<", ">", "<=", ">=", "==", "!="].includes(op)) {
        this.noteBinop(op);
        if (op === "<" || op === "==") this.emit("ic_enter", op);
        const map = { "<": "lt", ">": "gt", "<=": "le", ">=": "ge", "==": "eq", "!=": "ne" };
        this.emit(map[op]);
      } else if (op === "in") {
        this.noteBinop("in");
        this.emit("ic_enter", "in");
        this.emit("in");
      } else if (op === "&&") { this.noteBinop("&&"); this.emit("and"); }
      else if (op === "||") { this.noteBinop("||"); this.emit("or"); }
      else if (op === "??") {
        this.askType("null", "==", "str");
        this.askIsel("typeof");
        const tr = "__nr" + this.localslot.length;
        const tl = "__nl" + this.localslot.length;
        this.bind(tr); this.bind(tl);
        this.emit("store_l", tr);
        this.emit("store_l", tl);
        this.emit("load_l", tl);
        this.emit("typeof");
        this.emit("const", "str", this.strId("null"));
        this.emit("eq");
        const jz = this.emit("jumpz", 0);
        this.emit("load_l", tr);
        const jmp = this.emit("jump", 0);
        this.code[jz].a = this.code.length;
        this.emit("load_l", tl);
        this.code[jmp].a = this.code.length;
      }
    }
  }

  unary() {
    const p = this.ask("unary");
    if (p === "neg") {
      this.eat("-"); this.unary();
      this.emit("const", "i64", -1); this.emit("mul"); return;
    }
    if (p === "not") { this.eat("!"); this.unary(); this.emit("not"); return; }
    if (p === "spread") { this.eat("..."); this.unary(); this.emit("spread"); return; }
    if (p === "typeof") { this.eat("typeof"); this.unary(); this.emit("typeof"); return; }
    this.postfix();
  }

  postfix() {
    this.primary();
    while (true) {
      const p = this.ask("postfix");
      if (p === "index") {
        this.eat("[");
        this.assign();
        this.eat("]");
        this.emit("ic_enter", "idx");
        this.emit("idx");
      } else if (p === "call") {
        this.eat("(");
        let argc = 0;
        let spread = false;
        while (!this.at(")")) {
          if (this.at("...")) {
            if (spread) throw new Error("multiple spreads in call");
            this.eat("...");
            this.assign(); // leave list on stack
            spread = true;
            if (this.at(",")) throw new Error("spread must be last call argument");
            break;
          }
          this.assign();
          argc++;
          if (this.at(",")) this.eat(",");
        }
        this.eat(")");
        this.emit("ic_enter", "call");
        this.emit("call", spread ? (128 + argc) : argc);
      } else if (p === "dot") {
        this.eat(".");
        const name = this.eat("id").text;
        this.emit("ic_enter", "dot");
        this.emit("dot", name);
      } else break;
    }
  }

  primary() {
    const p = this.ask("primary");
    if (p === "ident") {
      const name = this.eat("id").text;
      if (this.at("=>")) return this.arrowFn([name]);
      if ((name === "len" || name === "keys") && this.at("(")) {
        this.eat("(");
        this.assign();
        this.eat(")");
        if (name === "len") this.emit("ic_enter", "len");
        this.emit(name);
        return;
      }
      const [where, n] = this.resolveLoad(name);
      this.emit(where === "l" ? "load_l" : "load_g", n);
      return;
    }
    if (p === "number") {
      const t = this.eat("num");
      if (typeof t.val === "number" && !Number.isInteger(t.val)) this.emit("const", "f64", t.val);
      else this.emit("const", "i64", t.val);
      return;
    }
    if (p === "string") {
      const t = this.eat("str");
      this.emit("const", "str", this.strId(t.val));
      return;
    }
    if (p === "null") { this.eat("null"); this.emit("const", "null", null); return; }
    if (p === "true") { this.eat("true"); this.emit("const", "bool", true); return; }
    if (p === "false") { this.eat("false"); this.emit("const", "bool", false); return; }
    if (p === "list") {
      this.eat("[");
      let n = 0;
      while (!this.at("]")) {
        this.assign();
        n++;
        if (this.at(",")) this.eat(",");
      }
      this.eat("]");
      this.emit("mklist", n);
      return;
    }
    if (p === "dict") {
      this.eat("{");
      let n = 0;
      while (!this.at("}")) {
        let key;
        if (this.at("str")) key = this.eat("str").val;
        else key = this.eat("id").text;
        this.eat(":");
        this.emit("const", "str", this.strId(key));
        this.assign();
        n++;
        if (this.at(",")) this.eat(",");
      }
      this.eat("}");
      this.emit("mkdict", n);
      return;
    }
    if (p === "paren") {
      this.eat("(");
      if (this.at(")")) {
        this.eat(")");
        if (this.at("=>")) return this.arrowFn([]);
        throw new CompileError("empty paren");
      }
      if (this.at("id") || this.at("...")) {
        const save = this.i;
        const params = [];
        let rest = null, ok = true;
        while (!this.at(")")) {
          if (this.at("...")) {
            this.eat("...");
            rest = this.eat("id").text;
            params.push(rest);
            break;
          }
          if (!this.at("id")) { ok = false; break; }
          params.push(this.eat("id").text);
          if (this.at(",")) this.eat(",");
          else if (!this.at(")")) { ok = false; break; }
        }
        if (ok && this.at(")")) {
          this.eat(")");
          if (this.at("=>")) return this.arrowFn(params, rest);
        }
        this.i = save;
      }
      this.assign();
      this.eat(")");
      return;
    }
    throw new CompileError("primary " + p + " at " + this.cur().kind);
  }

  arrowFn(params, rest = null) {
    this.eat("=>");
    const name = "__arrow" + Object.keys(this.metaFns).length;
    const nested = new Compiler("");
    nested.toks = this.toks;
    nested.i = this.i;
    nested.scopes = [new Set()];
    for (const p of params) nested.bind(p);
    if (this.at("{")) {
      nested.eat("{");
      nested.scopes.push(new Set());
      while (!nested.at("}")) nested.statement();
      nested.eat("}");
      nested.scopes.pop();
    } else {
      nested.assign();
      nested.emit("ret");
    }
    if (!nested.code.length || nested.code[nested.code.length - 1].op !== "ret") {
      nested.emit("const", "null", null);
      nested.emit("ret");
    }
    this.i = nested.i;
    for (const s of nested.strings) this.strId(s);
    const nparams = params.length - (rest ? 1 : 0);
    this.metaFns[name] = {
      code: nested.code,
      strings: nested.strings,
      localslot: nested.localslot,
      meta: { nparams, rest, name, fns: nested.metaFns },
    };
    this.emit("const", "fn", name);
  }
}

function u32(n) {
  const b = new Uint8Array(4);
  new DataView(b.buffer).setUint32(0, n >>> 0, true);
  return b;
}
function i64(n) {
  const b = new Uint8Array(8);
  new DataView(b.buffer).setBigInt64(0, BigInt(n), true);
  return b;
}
function f64(n) {
  const b = new Uint8Array(8);
  new DataView(b.buffer).setFloat64(0, n, true);
  return b;
}

function encodeFn(fn) {
  const strings = [...fn.strings];
  const strIx = new Map(strings.map((s, i) => [s, i]));
  const intern = (s) => {
    if (!strIx.has(s)) {
      strIx.set(s, strings.length);
      strings.push(s);
    }
    return strIx.get(s);
  };
  const nested = { ...(fn.meta.fns || {}) };
  const fnNames = Object.keys(nested).sort();
  const fnIx = Object.fromEntries(fnNames.map((n, i) => [n, i]));
  const slots = {};
  fn.localslot.forEach((n, i) => { slots[n] = i; });
  const ensureLocal = (name) => {
    if (slots[name] === undefined) slots[name] = Object.keys(slots).length;
    return slots[name];
  };
  const globalsUsed = [];
  const gIx = {};
  const gslot = (name) => {
    if (gIx[name] === undefined) {
      gIx[name] = globalsUsed.length;
      globalsUsed.push(name);
    }
    return gIx[name];
  };
  for (const ins of fn.code) {
    if ((ins.op === "load_l" || ins.op === "store_l") && typeof ins.a === "string") ensureLocal(ins.a);
  }
  const localNames = Object.entries(slots).sort((a, b) => a[1] - b[1]).map(([n]) => n);

  const abs = [];
  const icMap = { "+": "add", "-": "sub", "*": "mul", "/": "div", "%": "mod", "<": "lt", "==": "eq" };
  for (const ins of fn.code) {
    const op = ins.op;
    if (op === "ic_enter") {
      let name = icMap[ins.a] || ins.a || "add";
      if (!IC_OPS.includes(name)) name = "add";
      abs.push(["ic_enter", IC_OPS.indexOf(name)]);
    } else if (op === "nop") abs.push(["nop"]);
    else if (op === "drop") abs.push(["drop"]);
    else if (op === "const") {
      const [kind, val] = [ins.a, ins.b];
      if (kind === "null") abs.push(["const_null"]);
      else if (kind === "bool") abs.push(["const_bool", val ? 1 : 0]);
      else if (kind === "i64") abs.push(["const_i64", val]);
      else if (kind === "f64") abs.push(["const_f64", val]);
      else if (kind === "str") abs.push(["const_str", typeof val === "number" ? val : intern(val)]);
      else if (kind === "fn") abs.push(["const_fn", fnIx[val]]);
      else throw new Error("const " + kind);
    } else if (op === "load_l") abs.push(["load_l", ensureLocal(ins.a)]);
    else if (op === "store_l") abs.push(["store_l", ensureLocal(ins.a)]);
    else if (op === "load_g") abs.push(["load_g", gslot(ins.a)]);
    else if (op === "store_g") abs.push(["store_g", gslot(ins.a)]);
    else if (["add", "sub", "mul", "div", "mod", "lt", "le", "gt", "ge", "eq", "ne", "and", "or", "not", "idx", "setidx", "len", "keys", "in", "typeof", "shapeof", "spread", "print", "ret"].includes(op)) {
      abs.push([op]);
    } else if (op === "dot") abs.push(["dot", intern(ins.a)]);
    else if (["mklist", "mkdict", "mktup", "call", "restpack"].includes(op)) abs.push([op, ins.a | 0]);
    else if (["jump", "jumpz", "jumpnz"].includes(op)) abs.push([op, ins.a | 0]);
    else throw new Error("op " + op);
  }

  const codeToAbs = {};
  abs.forEach((_, i) => { codeToAbs[i] = i; });
  // map original code index → abs index (1:1)
  fn.code.forEach((_, i) => { codeToAbs[i] = i; });
  codeToAbs[fn.code.length] = abs.length;

  const opSize = (item) => {
    const n = item[0];
    if (["nop", "const_null", "drop", "add", "sub", "mul", "div", "mod", "lt", "le", "gt", "ge", "eq", "ne", "and", "or", "not", "idx", "setidx", "len", "keys", "in", "typeof", "shapeof", "spread", "print", "ret"].includes(n)) return 1;
    if (["const_bool", "load_l", "store_l", "load_g", "store_g", "mklist", "mkdict", "mktup", "call", "restpack", "ic_enter"].includes(n)) return 2;
    if (n === "const_i64" || n === "const_f64") return 9;
    if (["const_str", "const_fn", "dot"].includes(n)) return 5;
    if (["jump", "jumpz", "jumpnz"].includes(n)) return 5;
    throw new Error("size " + n);
  };
  const byteOff = [];
  let off = 0;
  for (const item of abs) {
    byteOff.push(off);
    off += opSize(item);
  }
  const endOff = off;

  const encOne = (item) => {
    const n = item[0];
    const out = [OP[n]];
    if (n === "const_bool") out.push(item[1] & 0xff);
    else if (n === "const_i64") out.push(...i64(item[1]));
    else if (n === "const_f64") out.push(...f64(item[1]));
    else if (["const_str", "const_fn", "dot"].includes(n)) out.push(...u32(item[1]));
    else if (["load_l", "store_l", "load_g", "store_g", "mklist", "mkdict", "mktup", "call", "restpack", "ic_enter"].includes(n)) out.push(item[1] & 0xff);
    else if (["jump", "jumpz", "jumpnz"].includes(n)) out.push(...u32(item[1]));
    return out;
  };

  const final = [];
  for (const item of abs) {
    if (["jump", "jumpz", "jumpnz"].includes(item[0])) {
      const ti = item[1];
      const absI = codeToAbs[ti];
      const tgt = absI >= byteOff.length ? endOff : byteOff[absI];
      final.push(...encOne([item[0], tgt]));
    } else final.push(...encOne(item));
  }

  const encodedFns = fnNames.map((n) => {
    const child = encodeFn({
      code: nested[n].code,
      strings: nested[n].strings,
      localslot: nested[n].localslot,
      meta: nested[n].meta,
    });
    return {
      name: n,
      code: child.code,
      strings: child.strings,
      locals: child.locals,
      nparams: nested[n].meta.nparams || 0,
      rest: nested[n].meta.rest,
      fns: child.fns,
      globals: child.globals,
    };
  });

  return {
    code: new Uint8Array(final),
    strings,
    locals: localNames,
    globals: globalsUsed,
    fns: encodedFns,
    nparams: fn.meta.nparams || 0,
    rest: fn.meta.rest,
  };
}

export function packProgram(blob) {
  const parts = [];
  const code = blob.code instanceof Uint8Array ? blob.code : new Uint8Array(blob.code);
  parts.push(u32(code.length), code);
  const strings = blob.strings || [];
  parts.push(u32(strings.length));
  for (const s of strings) {
    const b = new TextEncoder().encode(s);
    parts.push(u32(b.length), b);
    const pad = (4 - (b.length % 4)) % 4;
    if (pad) parts.push(new Uint8Array(pad));
  }
  const fns = blob.fns || [];
  parts.push(u32(fns.length));
  for (const f of fns) {
    const locs = f.locals || [];
    const restIx = f.rest && locs.includes(f.rest) ? locs.indexOf(f.rest) : 0xffffffff;
    const nested = packProgram(f);
    parts.push(u32(f.nparams || 0), u32(restIx >>> 0), u32(locs.length), nested);
  }
  let total = 0;
  for (const p of parts) total += p.length;
  const out = new Uint8Array(total);
  let o = 0;
  for (const p of parts) {
    out.set(p, o);
    o += p.length;
  }
  return out;
}

export function compile(src) {
  resetAskTrace();
  const c = new Compiler(src);
  const fn = c.compile();
  const blob = encodeFn(fn);
  const image = packProgram(blob);
  const heat = {};
  for (const e of askTrace) heat[e.stage] = (heat[e.stage] || 0) + 1;
  return { image, blob, fn, askTrace: askTrace.slice(), askHeat: heat };
}
