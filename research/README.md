# research/

五篇论文组成的研究路线。这里是论文与研究草稿，**不是**产品规格：unisacc 的规格是仓库根的
[`prd.md`](../prd.md)，UJS 的规格是 [`ujs/prd.md`](../ujs/prd.md)。

## 五篇论文的分工

| 主线 | 题目 | 回答的问题 | 正文 | 状态 | 编辑 |
|---|---|---|---|---|---|
| **Paper A** | neural-network-based compiler：给定表到网络的精确构造 | 有限控制表如何构造为网络；编译器实例与保证层次（§2.3） | [`unisacc-paper.md`](unisacc-paper.md) | 修订中 | cdx-unisacc 编辑，cc-unisacc 提供实现证据 |
| **Paper B** | UJS：封闭 JavaScript 子集的构造式表网络 | 同一方法在第二门语言上是否成立 | [`ujs-paper.md`](ujs-paper.md) | 草稿 | csr |
| **Paper C** | 把“确定性模型推理替代编译”推广为管道方法 | 为什么能推广、需要什么条件、能否机械化 | [`paper-c-intent.md`](paper-c-intent.md) | 意向书 | 待定 |
| **Paper D** | 流水线上的可选内存安全节点：模型推断标注、确定性检查器验证、分级编译 | 不改一行 C99 能否拿到接近 Rust 的保证；保证从哪来、如何量化 | [`paper-d-intent.md`](paper-d-intent.md) | 意向书 | 待定 |
| **Paper E** | 编译器有限控制表的系统构造方法 | 如何从文法、属性规则与状态不变量获得可靠的控制表 | [`paper-e-intent.md`](paper-e-intent.md) | 研究候选 | 待定 |

A 与 B 分别提供 C99 子集与 UJS 的实现及验证证据；C 不重复这些工作，只把 A、B 当作证据引用。
B 的方法命题一律引用 A；不编造延迟或准确率数字；不把 M3 写成 IntNet。

A 研究给定有限控制表到网络的精确构造，不证明控制表自动覆盖完整 C99；E 研究可靠控制表的系统构造方法。A 仍须交代表的来源、可信基、覆盖边界和已知缺陷。C 研究阶段组合条件，D 研究安全节点，均不把网络等于表当作语言或安全语义正确性的证明。

## 每条主线的文件

**Paper A**

| 文件 | 角色 |
|---|---|
| [`unisacc-paper.md`](unisacc-paper.md) | 正文 |
| [`paper-a-notes.md`](paper-a-notes.md) | 实现侧的修改建议与投稿待办（含原始日志归档、历史性能版本核对）；已并入正文的条目删去，余下移入投稿待办后再删文件 |
| [`delta-framework.md`](delta-framework.md) | 理论草稿：编译 = 通用执行器 ∘ δ*（未证；只此一份，A §2.2 与 C 交叉引用其定义） |
| [`formalization-roadmap.md`](formalization-roadmap.md) | 形式化义务：Lean 4 L0–L3 |
| [`lean/`](lean/) | Lean 4 契约内核（`lake build`） |
| [`figures/`](figures/) | 插图（图 1 网络结构） |

实现证据在仓库里：`prd.md` 的 S-17（模型化迁移 E0–E7）与 `exec/`（E0 执行器）。

**Paper B**

| 文件 | 角色 |
|---|---|
| [`ujs-paper.md`](ujs-paper.md) | 正文草稿：构造脊 + M3 出货脊 |
| [`ujs-paper-outline.md`](ujs-paper-outline.md) | 一页提纲 |

验收分层见 `ujs/prd.md` #4：`tests/ujs.sh` · `tests/ujs2wasm_compiler.sh` · `tests/uxe_ship_js.sh` · UXE 另门。

**Paper C**

| 文件 | 角色 |
|---|---|
| [`paper-c-intent.md`](paper-c-intent.md) | 意向书（方法、需要的理论、至少三个新场景、风险） |

**Paper D**

| 文件 | 角色 |
|---|---|
| [`paper-d-intent.md`](paper-d-intent.md) | 意向书（分工、三步、需要的理论、实验、风险）；工程条目在 plans/v0.1.x.md v0.1.3 |

**Paper E**

| 文件 | 角色 |
|---|---|
| [`paper-e-intent.md`](paper-e-intent.md) | 系统构表方法的研究候选；随 unisacc 实践积累证据，不作为 A 投稿前必须完成的能力 |

**共享资料**

| 文件 | 角色 |
|---|---|
| [`prior-art.md`](prior-art.md) | 对抗性相关工作 |
| [`papers/`](papers/) | 相关工作的 PDF |

## 归档

已发布版本的逐项证据（r9–r12 与 2026-09-28 的基准数据）在 [archive/research/](../archive/research/)，按版本分目录；仍被现行文档或门禁引用的文件留在本目录。
