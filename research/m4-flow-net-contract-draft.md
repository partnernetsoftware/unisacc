# m4 流×网最小合同（草案 · 2026-10-18 AWST）

> 轨名：**m4**（暂定，日后可改）。路径：**B 为主 + A 作种子**。  
> 主张：表构造 IntNet 做编译（非训练）；与 Paper A 同方法核。  
> **M3 / `compiler_core` = 产品旁路或历史脚手架**，不是本轨成功判据，也不是 Paper B 主张中心。  
> 约束：不改 unisacc 产品码；方法证明 ≠ Pages 必经；不投稿除非政委另点头。  
> 本文件真源：`research/m4-flow-net-contract-draft.md`（本仓 research/）。

---

## 1. 总览对照

| 层 | unisacc | **m4** | 现仓 ujs（种子/旁路） |
|---|---|---|---|
| 流水 | **7** 字节流合同 | **N=4**（wasm 向；角色对齐七段，**数量≠7**） | M3：1 条出货脊（旁路） |
| 网表 | **18** 事实网 | **起步 10**（复用 ujs STAGES；可裁可增另拍） | 同 10；construct 方法工厂 |
| 出证 | 六平台原生 | 表驱动源→`\0asm`（方法证明） | Pages/`sim.wasm`（产品，非 B 中心） |

口诀：**unisacc = 7 流 × 18 网 → 原生；m4 = 4 流 × 10 网 → `\0asm`（方法）；M3 出货不进主张分子。**

---

## 2. 流段合同（角色对齐 unisacc 七段）

| # | m4 流段 | 对齐 unisacc | 输入 → 输出 | 默认绑网 |
|---|---|---|---|---|
| 1 | **lex** | e1 lex | 源文本 → tokens | `lex` |
| 2 | **front** | e3 parse（兼 type/scope 决策） | tokens → jtape/IR | `parse`, `type`, `scope`, `shape`；（`ic` 可选） |
| 3 | **select** | lowering 前半 | IR/op → 后端形态 | `isel`；**`irsel` 见 §4 待拍** |
| 4 | **encode** | enc + elf | 形态 → `\0asm` 字节 | `enc`, `reloc` |
| — | 本轨不做 | e2 pp / e4 opt / prune | C 预处理与 tape 优剪 | — |

**诚实声明（须进 Paper B）：** m4 的 4 不是「缩小版 7」；是 wasm 向重新切的字节合同，仅**角色**与七段对照。

种子来源：ujs `seed/construct` 的 front→jtape→emit/ujs2wasm 工序，升格为显式流段，而不是继续以 M3 自举当主叙事。

---

## 3. 网表层（复用 / 裁剪 ujs 10）

起步名单（与 `ujs.gold.ALL` 一致）：

`lex, parse, type, scope, shape, irsel, ic, isel, enc, reloc`

| 网 | m4 默认 | 说明 |
|---|---|---|
| lex…shape, isel, enc, reloc | **启用**（ask 或等价查表） | 流段绑定见上 |
| **ic** | 可选 | 热路径桩；方法证明可不依赖 |
| **irsel** | **表核保留；运行接线 = 待拍** | 见 §4 |

构造：`unisa.intnet.build(..., stages=m4.STAGES)`，**不登记**进 unisa 默认 18。验收：acc 类「构造 + 全键枚举 exact」。

---

## 4. 待政委拍（仅列点，不阻塞草案）

1. **irsel**：预留（仅 gold/acc）vs 接入 select 流段 ask。推荐草案默认：**预留**（与现行 ujs 一致，少双源）。  
2. 10 网是否首刀裁掉 `ic`（证明确认不需要热路径）。  
3. 同仓目录名（`m4/` vs 其它）与是否先只放 research 文档。

---

## 5. 成功判据（最小，绿了才算本轨立住）

1. **acc 类**：本轨 STAGES IntNet 构造 + 枚举 exact。  
2. **一条表驱动路径**：源 → 流段 1…4（网驱动决策）→ `\0asm`，可复现；允许方法工厂/Python 或后续非 M3 实现。  
3. **口径**：对外主张图 = gold → IntNet → 流 → 字节；**M3 仅脚注/旁路**。  
4. **非目标**：替换 Pages 出货、tinyvm 执行层、改 unisacc、训练填表、投稿。

---

## 6. 与决策卡对齐

- 决策卡 `research/paper-b-intent-decision-20261018.md`（`a723d52d`）：初心 = 表构造 IntNet；M3 非主张中心。  
- 本草案落实工程「流×网」最小合同，供 Paper B 主张措辞与附录引用。
