# UJS — 推理编译实验（平行于 UNISA C 线）

> **产品**：网页里 `wasm_run(code | fn, globals, locals)`  
> **命题**：与 UNISA 同构 — Shell = 推理器 + 执行器 + 模型数据  
> **语言**：UJS-1（闭合 JS 子集，表内全功能；表面持续向 JS 扩）  
> **规格**：[`prd.md`](prd.md)

Python 是**构造器与验收台**；浏览器跑的是 **WASM + demo JS**（不在页面训练）。

## 怎么跑

```bash
# 验收（acc / fold / icfold / difftest / ship / web / in-page wasm_run）
./tests/ujs.sh

# 构造网页产物
python3 -m ujs web-build
# 然后静态打开 ujs/web/index.html，或：
# python3 -m http.server -d ujs/web 8765

python3 -m ujs acc
python3 -m ujs run --code 'return 1+2;'
python3 -m ujs js2wasm prog.ujs -o prog.wasm
python3 -m ujs wasm-run prog.ujs
```

## 目录图

```
ujs/
├── prd.md              # 规格（交付 / L / IC / X-*）
├── README.md           # 本文件
├── __main__.py         # CLI：acc / run / fold / icfold / difftest / ship / web-build / js2wasm
├── api.py              # compile / run / wasm_run
├── gold.py             # 各 stage 全表 gold（构造真源）
├── catalog.py          # 词表：shape / IC stub / wasm form
├── oracle.py           # ask：gold 或 built IntNet（P-1/P-2）
├── value.py / jtape.py / vm.py
├── bc.py / bc_encode.py / bc_vm.py   # 全量字节码 ISA + 编码 + Python 参考 VM
├── lower_wasm.py / wat_vm.py         # WasmProgram 路径 / 程序映像 pack
├── front/
│   ├── lex.py          # 词法（分类经 lex 表）
│   └── compile.py      # 走查 → jtape（决策经 oracle.ask）
├── emit_web.py         # → ujs_rt.wat/wasm（ask + i64 demo VM）
├── emit_ic_c.py        # → native/ujs_ic_net.c（ic 表 ≡ 构造网）
├── emit_browser.py     # → web/compiler.gen.js（页内 ask 表）
├── js2wasm.py          # UJS-1 → 自包含 .wasm（zig + C VM + embed）
├── native/
│   ├── ujs_vm.c        # freestanding 全量字节码 VM + host_* API
│   └── ujs_ic_net.c    # 生成的 ic ask 表（勿手改）
└── web/                # 产品页（web-build 产出）
    ├── index.html / style.css / demo.js
    ├── wasm_run.js     # 浏览器 API
    ├── compiler.js     # 页内走查编译器
    ├── compiler.gen.js # 生成的 ask 表
    ├── jspi.js         # Chrome/Firefox JSPI 异步探针
    ├── ujs_rt.wasm     # ask runtime
    ├── ujs_full.wasm   # 共享全量 VM（宿主灌 image）
    └── progs/*.wasm    # js2wasm 预编译 demos
```

## 数据流（一眼）

```
src ──front──► jtape (Fn)
                │
                ├─ vm.run          （语义真源）
                ├─ bc_encode       ──► bytecode image
                │                       ├─ bc_vm（Python）
                │                       └─ native/ujs_vm.c → .wasm
                └─ lower_wasm      ──► WasmProgram（fold 另一侧）

页内：compiler.js + ask表 ──► image ──host_*──► ujs_full.wasm
```

## 与仓内其它部分的关系

| 路径 | 角色 |
|---|---|
| `unisa/` | C99 线：同一构造代数 / IntNet；**不交付 Web** |
| `ujs/` | 平行实验：JS 子集 → 网页 WASM |
| `tests/ujs.sh` | UJS 验收入口 |
| `weights/` | UNISA C 线权重；UJS 权重经 `ujs build-weights` 可写 `weights/ujs/` |

生成物（`web-build` / `emit_*`）可提交以便裸开 `index.html`；改 gold/VM 后请重跑 `python3 -m ujs web-build`。
