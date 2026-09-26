# research/

三条论文主线。这里是论文与研究草稿，**不是**产品规格：unisacc 的规格是仓库根的
[`prd.md`](../prd.md)，UJS 的规格是 [`ujs/prd.md`](../ujs/prd.md)。

## 三条主线

| 主线 | 题目 | 回答的问题 | 正文 | 状态 | 编辑 |
|---|---|---|---|---|---|
| **Paper A** | 表即网络：用确定性模型推理替代 C99 编译 | 方法能否做成一个真实的编译器；理论依据（§1.1 已证 / 证据 / 未证） | [`unisacc-paper.md`](unisacc-paper.md) | 修订中 | cdx-unisacc 编辑，cc-unisacc 提供实现证据 |
| **Paper B** | UJS：封闭 JavaScript 子集的构造式表网络 | 同一方法在第二门语言上是否成立 | [`ujs-paper.md`](ujs-paper.md) | 草稿 | csr |
| **Paper C** | 把“确定性模型推理替代编译”推广为管道方法 | 为什么能推广、需要什么条件、能否机械化 | [`paper-c-intent.md`](paper-c-intent.md) | 意向书 | 待定 |

A 和 B 各自证明方法能做成一个编译器；C 不再重复这一点，只把 A、B 当作证据引用。
B 的方法命题一律引用 A；不编造延迟或准确率数字；不把 M3 写成 IntNet。

## 每条主线的文件

**Paper A**

| 文件 | 角色 |
|---|---|
| [`unisacc-paper.md`](unisacc-paper.md) | 正文 |
| [`paper-a-notes.md`](paper-a-notes.md) | 实现侧给正文的修改建议（待并入正文后删除） |
| [`delta-framework.md`](delta-framework.md) | 理论草稿：编译 = 通用执行器 ∘ δ*（未证；正文 §1.1 只引用其定义） |
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

**三文共享**

| 文件 | 角色 |
|---|---|
| [`prior-art.md`](prior-art.md) | 对抗性相关工作 |
| [`papers/`](papers/) | 相关工作的 PDF |
