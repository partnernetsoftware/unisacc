# Paper A 形式化路线（Lean 4）

B/C 凡方法命题一律 **[cite A]**；本文件是 A 的可机读证明义务索引，不是产品规格（规格仍在根 `prd.md`）。

正文义务表见 [`unisacc-paper.md`](unisacc-paper.md) **§3.4「形式化义务」**（命题 ID → 枚举 / Lean 定理名 / 外部裁判 / 开放）。

## 该进 / 不进 Lean

| 命题 | 现状 | Lean 角色 |
|---|---|---|
| **P-8** 朴素存在性 | folklore（Tracr / Omlin–Giles） | L0 `naive_exact`；**不作新定理宣称** |
| **P-3** `net ≡ gold` | 枚举脚本 | L1 `decision_list_exact`：构造成功 ⇒ ∀k；枚举 = 复核 |
| **P-5** `C[N]≡C[G]` | 口头同余 | L2 `cong_of_pointwise` / `p5_of_p3_trace` |
| **P-1** 不透明 | 工程纪律 | 进 L2 模型假设，不证整仓 |
| **P-2** `key ∈ K_s` | 运行时断言；L3 样板已建 | L3 `asks_subset_K`（`reloc`）；**全 walker 仍开放** |
| **P-6** `gold ≡ C` | gold_audit / difftest | 外置裁判接口；不证 C99 |
| **P-8b** SGD 不可达 | 文献 + 实例 | 引用，不塞进里程碑 |

## 分层

| 层 | 目标 | 目录 |
|---|---|---|
| **L0** | 朴素每键一单元：`∀k, eval(naive G) k = G k` → `naive_exact` | `lean/UnisaFormal/L0/` |
| **L1** | cube / 决策列表声音性（发布构造）→ `decision_list_exact` | `lean/UnisaFormal/L1/` |
| **L2** | P-5 同余 → `cong_of_pointwise` / `p5_of_p3_trace` | `lean/UnisaFormal/L2/` |
| **L3** | 单阶段 P-2（`reloc`）→ `asks_subset_K` / `p5_reloc_of_domain_closure` | `lean/UnisaFormal/L3/` |

**刻意不做**：自举不动点、六目标 ABI、gold≡C99、`h_min` NP。

**Paper B / UJS**：方法命题一律 [cite A]。B **无** UJS 专用 Lean 里程碑；M3 `compiler_core` 自举是**出货脊**孪生检验，不进 L0–L3。表阶段（IC / wasm lower）仍欠全 walker P-2，继承上表「全 walker 仍开放」。

## 构建

```bash
# 需 elan；仓库不捆绑 toolchain 二进制
cd research/lean && lake build
```

与实现桥接约定（L1 起）：小表可从 `unisa/gold.py` 导出 Fin 枚举证书；`acc=1.000` 仍是部署门禁，Lean 证的是**算法声音性**，不是替代 CI。L3 样板：`UnisaFormal/L3/DomainClosure.lean`（`reloc` 全积域闭包）。
