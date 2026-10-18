# m4 / 流×网 — 最小合同草案（2026-10-18）

> **真源**：本文件位于 `research/m4-flow-net-contract-draft.md`。实现优先落同仓 **`m4/`**（暂定名，可改）。  
> **锁定**：见 [`paper-b-intent-decision-20261018.md`](paper-b-intent-decision-20261018.md) §4（B 为主 + A 种子 → **m4**）。  
> **分工**：ujs 开发维护本合同；论文撰写员据此翻转 `ujs-paper.md` 主张中心措辞，不得把 M3 出货口诀写成 Paper B 主贡献。

## 1. 目标（论文主图口径）

**gold → IntNet → flow → bytes**（闭合 JS 子集 → `\0asm`）。

- **gold**：有限表 / 构造器产出（可种子自 `ujs/` construct 臂，A 角色）。
- **IntNet**：Paper A 方法核 [cite A]；`intnet.build(..., stages=…)`；表阶段 **从 UJS 10 起**（`lex` … `reloc`，见 `ujs.STAGES`）。
- **flow**：字节流推导式管线；**流段数 N 可 ≠ 7**（不必硬贴 unisacc 七段字节流契约）。
- **bytes**：输出 **`\0asm`**（wasm 模块字节；Host/Pages 包装在合同外）。

**论文 ↔ ujs 开发对齐**：主图只用上述四格；**现状 M3 出货**（`compiler_core` 等）仅脚注「产品旁路」，不进主张分子。

## 2. 暂定 spine 口诀（方法证明弧）

**4 stream × 10 nets → `\0asm`**

| 层 | 暂定 | 说明 |
|---|---|---|
| **流** | **4** 段（命名待定） | 方法证明用压缩分子；**N 可辩证**，≠ unisacc 的 7 |
| **网** | **10** 表阶段 | 与 `ujs.STAGES` 同表 surface；≠ UNISA 18 fact nets |
| **出** | `\0asm` | 表驱动方法路径上的 wasm 字节 |

**现状出货（非主张中心）**：**1 M3 spine × 10 nets → `\0asm`** 仍描述 `ujs/iterate/` 产品面；与 m4 方法弧**并存**，M3 **不**冒充 IntNet 实例。

## 3. 最小验收（第一刀）

1. **acc exact**：十个表阶段（或合同签字阶段子集）各 **FULL-key `acc`** 绿（与现有 `ujs` 构造纪律同型）。
2. **一条端到端**：**一条**具名闭合 JS 源 → **仅经表驱动/流推导方法路径** → `\0asm`（具体样例名在 `m4/` 落地时登记；本拍不编造数字）。
3. **拒绝**：表外输入永久拒绝；不得用训练填表顶替 gold。

**非本刀**：全表 P-2 Lean、全 corpus、M3 twin 替代方法路径、改 unisacc 产品码。

## 4. 仓库与边界

| 项 | 约定 |
|---|---|
| **实现根** | 优先 **`m4/`**（本 monorepo）；可复用 `ujs/seed/construct` 种子材料，不把「只在 ujs 内改到全表驱动」当终态 |
| **M3 / `compiler_core`** | **产品旁路**；Pages/ship 可继续用；**不是** m4 方法合同的主体 |
| **Paper B** | 主张中心 = 构造 IntNet / 表阶段 /（目标）流管线；见 `ujs-paper.md` 摘要/§2 最小翻转 |
| **Paper A** | 方法核归属不变；m4 引用 [cite A]，不重证 |

## 5. 开放项（辩证清单）

- 四段流的切分命名与 construct/jtape 的映射表。
- `m4/` 与 `ujs/` 的门禁是否独立脚本或共享 `tests/ujs.sh` 子集。
- 暂定名 **m4** 是否在实现目录改名。

---

*ujs 开发起草 · 2026-10-18 · research-only*
