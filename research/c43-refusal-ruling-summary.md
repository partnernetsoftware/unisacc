# 0.0.43 拒绝域：待裁三项一页摘要（2026-10-14）

只陈述事实与选项，不代裁。产品、gatedeps、发布状态均未动。细节见
`c43-lower-refusal-domain-proposal.md`（cdx）、`c43-lower-refusal-evidence-gaps.md`、
`c43-middle-step0-receipt.md`；cdx2 的只读对照在仓外 `/tmp/unisacc-cdx2/observe-10m/bindings-refusal-evidence-gaps.md`。

## 已有事实

| 事实 | 来源 | 性质 |
|---|---|---|
| 输入 `?\n`：参考在 `unisa/tape.py:335` 拒绝（rc1），lower 模型接受（rc0，106 字节） | 第 0 步收据，NEG-001 | 实际跑过 |
| 出货产品把 `.tape` 文件当输入（`exec/c/compiler.c:44–46、:591`；`docs/tapebin-v1.md` 写了文本入口） | 读源码 | 只读 |
| 对内容为 `?\n` 的 `.tape`：`-S -o -` rc0 原样输出 2 字节；`-c` rc0 写出 404 字节对象；`-o` rc1 `reject: not covered: an op outside the first encoder slice` | 本次用 0.0.42 候选实跑 | 实际跑过；未与参考编译器对拍 |
| `unisa/lower.py:327` 把 `.hostcall`/`.hostaddr` 限在 osx、lnx；`exec/lower/libraryexitcheck.py:63–68` 要求模型在 `win/` 上接受这两条，且只对拍表执行器与模拟器 | 读源码 | 只读 |
| 非法 `.hostaddr` 索引没有任何负例；绑定拒绝分支多数只断言 rc1 | 两份缺口记录，cc 与 cdx、cdx2 三方独立核对一致 | 只读 |

## 待裁 1：拒绝域归谁

- **A**：新增前置 Tape 验域阶段，拥有语法、形状、记录规则；lower 只接已验流；目标相关拒绝留在 lower。代价：多一条显式边，所有直接调 lower 的路径要改。
- **B**：lower 公共入口自己验 raw tape。代价：lower 规则变大，可能重复 Tape 语法知识。
- 两案都改产品闭包，候选身份必变。

## 待裁 2：raw tape 入口是否产品承诺

上表第 2、3 行说明这不是假设：用户今天就能把 `.tape` 交给产品，且 `-S`、`-c` 对未知操作码返回成功。
- **是承诺**：`-c` 对 `?\n` 返回 rc0 是用户可见缺陷，NEG-001 应按产品缺陷处理，修复不可只靠上游过滤。
- **不是承诺**（仅内部/调试入口）：需在文档里明说，并决定是否对外关闭或标注“不验域”；阶段“独立执行”的账仍不能记绿。

## 待裁 3：Windows 上 `.hostcall`/`.hostaddr` 的接受

- **是有意的库模式合同**：要把这份合同写成显式输入（哪种模式、哪个目标允许），并让检查对拍一个承认该合同的参考；现在的检查没有参考一方。
- **是域外接受**：与 NEG-001 同类，登记为 NEG-002，`libraryexitcheck.py:63–68` 的正例要改。

## 与 cdx2 对照

一致：`libraryimportscheck.py` 的拒绝只验 rc1；截断前缀只在模拟器层验；NEG-001 不能因“上游不会给”而改绿。
cdx2 多出、本摘要采纳的两点：API 型负例（非零返回 + 具名 error + 无副作用）不应硬套“进程 rc1 + 空 stdout”；超时、信号、燃料耗尽、缺工具必须单列，不能算作拒绝。
本摘要多出：产品 CLI 对 `.tape` 的实跑结果（待裁 2 的依据）。

## 裁定后才能做的事

补非法 `.hostaddr` 负例；收紧 `libraryimportscheck.py` 的拒绝断言（闭包外，但可能把现有绿变红）；任何产品侧修复（需主人钉文件域和 0.0.43 窗口）。
