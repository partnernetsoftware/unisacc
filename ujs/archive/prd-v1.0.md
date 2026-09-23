# UJS —— 规格 v1.0（归档）

> **已归档。** 活规格见 [`../prd.md`](../prd.md) v1.1。  
> 本文保留完整 [L/V/A/G/X/IC/…] 条款表，供对照；勿当当前交付面真源。

# UJS —— 规格 v1.0

> **产品**：`wasm_run(code | fn, globals, locals)`  
> **命题**：与 UNISA SH 同构 —— Shell = 推理器 + 执行器 + 模型数据。  
> **语言**：UJS-1（闭合；不是 ECMAScript。表内全功能，表外永久拒绝。）  
> **配套**：共享 `unisa` 的构造代数与整数 kernel；本包自有 gold / 走查 / jtape / lower / IC。

## 0. 导读

### 0.1 交付

**交付**：
1. **JS 产品 / 库** —— `ujs/web` + `package.json`：`bootRuntime` / `wasm_run`（浏览器 · Node · Bun）
2. **站点 / 示例** —— `web/index.html` playground（`npm run demo`）；**UXE** `web/engine/`（`demo/` 源码测 · `ship/` 发布面：html + `{game}.wasm` + `engine.wasm`）；可选 `web/game/`（Three 对照，非 API 契约）
3. **`js2wasm`** —— 全量 UJS-1 → `.wasm`（construct CLI；C VM via zig）
4. **构造侧** —— `ujs/construct/`（Python gold / front / build；非应用依赖）

浏览器与 Node 跑的是 WASM + ESM；**不在页面训练**。构造：`python3 -m ujs web-build`。  
二进制倾向 **GitHub Release**；仓内留 `BUILD.json` 指纹（见 `TOOLS.md`）。

**不交付**：完整 ES、开放原型链、`eval`、异步、正则引擎。

### 0.2 术语

| 术语 | 定义 |
|---|---|
| **UJS-1** | 本规格闭合的语言；条款 [L-*] |
| **jtape** | 目标无关指令流（对标 unisa tape） |
| **Fn** | `compile` 产物：jtape + 元数据；可缓存 |
| **IC** | `(shape × op × guard) → stub_kind`；有限 stub 模板库 |
| **fold** | 同一 Fn：jtape VM ≡ WasmProgram 解释 ≡（可选）宿主 wasm 实例 |

### 0.3 条款前缀

| 前缀 | 域 | 前缀 | 域 |
|---|---|---|---|
| `T` | 命题 | `L` | 语言 UJS-1 |
| `D` | 确定性 | `V` | 值与环境 |
| `A` | API | `W` | 走查 |
| `G` | Gold | `TP` | jtape |
| `IC` | 内联缓存 | `LW` | wasm lower |
| `P` | 证明 | `K` | kernel |
| `F` | 超级拟合 | `X` | 验收 |
| `U` | CLI | | |

---

# 第一部分 · 契约

## 1. 命题 [T]

- **[T-1]** 走查器 / 环境绑定 / 栈 / stub 装配 / wasm 段布局 = **经典代码**。
- **[T-2]** 网络只做最后一公里选表。阶段清单见 §3。
- **[T-3]** Gold 同时是标注、验证器；出货路径无「低置信回退」。
- **[T-4]** 唯一 kernel：`embed → gemv → ReLU → gemv → argmax`（与 unisa 相同；整数构造）。
- **[T-5]** 对外语义锚点是 `wasm_run`；编译可缓存，执行绑定显式 `G`/`L`。

## 2. 确定性 [D]

| ID | 条款 |
|---|---|
| **D-1** | 推理期无 RNG |
| **D-2** | argmax 平局取最小类下标 |
| **D-3** | 同源码 + 同权重 → Fn / wasm blob **逐字节相同** |
| **D-4** | 网络与 gold 对同一 key 决定必须相同 |

## 3. 超级拟合与证明 [F] [P]

| ID | 条款 |
|---|---|
| **F-3** | `SHIP_ACC = 1.000`；低于此值拒绝 ship |
| **F-5** | 到不了 1.000 = key 编码错，去修编码 |
| **P-1** | Oracle 只回传类名；禁读 logits；禁回退 gold |
| **P-2** | `ask` 的 key ∈ K_s，无条件断言 |
| **P-3** | 逐阶段穷举：`∀k: argmax(N)=G` |
| **P-5** | net 驱动 ≡ gold 驱动（前提 P-1） |

---

# 第二部分 · 语言与 API

## 4. 语言 UJS-1 [L]

### 4.1 有（全功能闭包 —— 全部实现，无「以后再说」）

| 类 | 内容 |
|---|---|
| 字面量 | `null` `true`/`false` 整数 浮点 字符串 |
| 复合 | `list` `[...]` · `dict` `{k: v}`（键为 str）· tuple 多返回 |
| 绑定 | `let` / `const`（语义同 let）赋值；块作用域；复合赋值 `+=`… |
| 控制 | `if`/`else` `while` `for(;;)` `for…of` `break` `continue` `return` **`switch`/`case`/`default`** |
| 函数 | `function name(a,b,...rest)`；**箭头**；调用；rest；**call-site spread** `f(...xs)` / `f(a,...xs)`（`call` argc=`128+nfixed`） |
| 运算 | 算术 比较（含 `===`/`!==`）逻辑 `??` 三元 `?:` 索引 `a[b]` 成员 `a.b` `in` `typeof` `len` `keys` |
| 闭包 | 仅可读 `G ∪ 词法外层已绑定名`；捕获集在 compile 期封闭 |

### 4.2 无（永久）

`undefined` 与 `null` 双无值 · `var`/hoisting · `this`/`new`/`class`/prototype · `eval`/`with`/Proxy · `async`/`await`/generator · RegExp 字面量 · 原型链查找 · 隐式全局。

### 4.3 名字解析 [L-10]

查找顺序：**locals → 词法外层 → globals**。未命中 = 硬错。无原型、无 `globalThis` 魔法。

## 5. 值与环境 [V]

| ID | 条款 |
|---|---|
| **V-1** | `Value` = `null \| bool \| i64 \| f64 \| str \| list \| dict \| fn \| tup` |
| **V-2** | `dict` 无原型、无 accessor；形状由有限 `shape_id` 刻画 |
| **V-3** | `G` 默认只读；写全局须 `run(..., mutate_globals=True)` |
| **V-4** | `L` 可写；`run` 可返回更新后的 `L'` |
| **V-5** | 宿主 intrinsic 仅经 `G` 注入（如 `print`） |

## 6. API [A]

```text
compile(src: str) -> Fn
run(fn|src, globals, locals, *, mutate_globals=False) -> Result
wasm_run(fn|src, globals, locals, *, mutate_globals=False) -> Result
  Result = { ok: Value } | { err: { kind, message } }
```

| ID | 条款 |
|---|---|
| **A-1** | `wasm_run` ≡ 选定执行后端的 `run`（默认：已 lower 的 wasm 路径；无 wasm 实例时用 WasmProgram 解释，语义同） |
| **A-2** | `src` 形态每次可重 compile；`Fn` 形态跳过前端 |
| **A-3** | `Fn` 可序列化；反序列化后 `run` 结果与原 Fn 一致 |
| **A-4** | Python / C / WASM 导出三宿主同探针同答 |

---

# 第三部分 · 模型层

## 7. 阶段 [G]

全部阶段出货时必须 1.000。无「先手写长期不换网」。

| stage | 字段 | 类（示意） |
|---|---|---|
| `lex` | charclass × peek | lexact |
| `parse` | NT × TOK | prod |
| `scope` | ctx × kind | act |
| `type` | t1 × op × t2 | tyout |
| `shape` | ty × keysig | shape_id |
| `irsel` | family × flavor | recipe |
| `ic` | shape × op × guard | stub_kind |
| `isel` | op | wasm_form |
| `enc` | form × immclass | enc_template |
| `reloc` | kind | reloc_kind |

**[K-1]** 与 unisa 同一构造路径与 IntNet。

---

# 第四部分 · 经典层

## 8. 走查 [W]

`src → lex → parse → scope/type/shape → irsel → jtape (+ 可选 IC 注解)`

每表决策经 `oracle.ask`。[P-1][P-2] 适用。

## 9. jtape [TP]

| ID | 条款 |
|---|---|
| **TP-1** | jtape 是语义真源；fold 的一侧 |
| **TP-2** | 操作数栈 + 局部槽 + 全局槽；无隐式对象 |
| **TP-3** | `ujs vm Fn [G] [L]` 直接解释 |

## 10. IC [IC]

| ID | 条款 |
|---|---|
| **IC-1** | 热点计数超过阈值 → `ask(ic, (shape, op, guard))` → stub_kind |
| **IC-2** | stub 模板库有限，列于 catalog；新增 stub = 扩表 + 重构 |
| **IC-3** | 开/关 IC，对同一 `(fn,G,L)` 结果必须一致（`icfold`） |
| **IC-4** | deopt / 回解释器是经典控制流，不是网络回退 |

## 11. wasm lower [LW]

| ID | 条款 |
|---|---|
| **LW-1** | jtape → WasmProgram →（可选）`.wasm` 字节 |
| **LW-2** | WasmProgram 解释器是 fold 的另一侧（对标 unisa TargetProgram 解释） |
| **LW-3** | 浏览器/wasmtime 跑 `.wasm` 是第三侧；与 LW-2 不一致 = bug |
| **LW-4** | `isel`/`enc`/`reloc` 只经 Oracle |

---

# 第五部分 · 验收与 CLI

## 12. 验收 [X]

| ID | 套件 | 要求 |
|---|---|---|
| **X-1** | `acc` | 每 stage 1.000，FULL gold |
| **X-2** | `difftest` | 与无网参考解释器同答 |
| **X-3** | `icfold` | IC on/off 同答 |
| **X-4** | `fold` | jtape VM ≡ WasmProgram |
| **X-5** | `determinism` | 两遍 compile 字节相同 |
| **X-6** | `api` | `wasm_run` 探针（含 switch/rest/spread/str） |
| **X-7** | `ship` | blob + MANIFEST + 自检 |

## 13. CLI [U]

```text
python3 -m ujs build-weights
python3 -m ujs acc
python3 -m ujs compile file.ujs -o out.fn
python3 -m ujs run file.ujs [--globals JSON] [--locals JSON]
python3 -m ujs wasm-run file.ujs ...
python3 -m ujs js2wasm file.ujs -o out.wasm   # 全量 UJS-1 → .wasm（main_export）
python3 -m ujs web-build                       # ask runtime + full demos
python3 -m ujs fold file.ujs
python3 -m ujs ship --out kit.zip
```

---

## 14. 与 UNISA C 线的关系

- 共享：构造代数、IntNet、确定性与 P-1/P-2/P-3 纪律。  
- 分离：gold、走查、jtape、IC、wasm lower、语料、kit。  
- C 线 prd「不交付 Web」不变；本产品是平行交付物。
