# 0.0.43 第 6 步第二刀：按阶段模型身份收窄 pp 族依赖（只读试点）

标签：**[实际跑过]** = strace `-f -e openat` 对套件的一次运行（≤60 秒，结果见下）与 `tests/affected.py` 回放；**[读源码]** = file:line。
不改队列与门禁；`tests/affected.py` 的假设模式只读。

## 1. pp 族真正读什么 [实际跑过]
六个 pp 套件（`exec-pp-directives`、`exec-pp-literals`、`exec-pp-pragmas`、`exec-pp-object`、`exec-pploc`、`pptruth`）中，前五个已 strace。**它们都不读候选的 P3 网络字节**（候选产品与 `model-audit` 都没有被打开）。它们自己从表构造 `pp.net`（`/tmp/pp-*/pp.net` 被反复打开，`pp.tbl` 同理），再用执行器运行。读集合（去掉 `__pycache__`）：

| 读集合 | 来源 |
|---|---|
| `exec/pp/*.tsv`（各 `*-byte.tsv`、`*-result.tsv`、`*-manifest.tsv`、模板） | 表与清单（pp 阶段的事实） |
| `exec/facts/pp-*.tsv`、`exec/facts/locations.tsv` | 事实表 |
| `exec/build/gen.py`（及 `graph.py`） | 构造器 |
| `exec/c/tbl.py`、`exec/c/net.py` | 编号化与网络写出 |
| `exec/c/core.c`、`core.h`、`codec.h`、`packagefooter.h`、`run.c` | 执行器与格式 |
| `unisa/front/pp.py` | 仅 literal、pragma 通过 import 读取 |
| 套件自身脚本与其 `refsource.py`（参考源） | 驱动 |

`pptruth` 尚未 strace（见 §5）。

## 2. 候选实证：pp 阶段网络哈希相同 [实际跑过]
用 `tests/modelstreamextract.cx` 从两个真实候选抽取 P3 网络（路由表 `routes.tsv` 与模型表 `models.tsv`，逐字节）：
- **92236ce3**（`/tmp/c43-tape-domain-range`）与 **0556ea9f**（`/tmp/c43-forward-paths/final`）。

| 阶段 | 两候选哈希 |
|---|---|
| e1、e2、e3、e4、units、tokenpp、tokenlex、lower、elf、prune、nativeabi、tapecheck | **相同**（逐个比较全部路由） |
| contract、symbols | 仅 0556ea9f 有（新增路由，前者无） |

即：除两个新增路由外，两候选的全部阶段网络哈希一致；pp 阶段（e2、e1、units）也一致。

## 3. 回放：三刀与 tapebin 拆分 [实际跑过]
`tests/affected.py --model-identity pp`（假设模式）：pp 套件只保留自身读集与本脚本（`PP_DEPS`）的命中，并在两候选 pp 哈希相等时去掉“候选字节”理由。

| 改动 | 闸作业受影响（of 748）：旧规则 | 假设模式 | pp 节省 |
|---|---:|---:|---:|
| 0571eb62 Tape 验域 | 737 | 731 | 6 |
| 03936e9f..ecdbd98d 晚绑定 | 735 | 729 | 6 |
| df1ce7ed 转发 include | 735 | 729 | 6 |
| 6420cfb3 tapebin（只 tests） | 173 | 173 | 0 |

诚实的读法：**pp 族只有 6 个作业，节省也只有 6 个**。这是因为 pp 套件的声明本就只占很小一块；真正的成本在其余 570 个声明与 159 个未声明作业。pp 族的“executable 声明”是**多余的**：它们不运行候选，却声明了候选字节（依赖错误但偏保守，不漏验）。

## 4. 风险与必须作废的条件
必须作废（即使阶段哈希相同）：
1. **执行器变**：`core.c`、`core.h`、`run.c`（包括 `UNISA_MAXSTEPS` 的编译期常量化这类改动）。
2. **构造器与表变**：`exec/build/*.py`、`exec/c/tbl.py`、`exec/c/net.py`、`exec/pp/*`、`exec/facts/pp-*`。
3. **参考变**：`refsource.py`、参考编译器的源（`export_ref` 输入）。
4. **检查代码变**：套件脚本本身，以及 `tests/` 中被其 import 的文件。
5. **封存数据变**：`tests/seals/*`（如 `exec-pp-literals` 的 44 次执行记录）。
6. **源标签规范变**：`\0cli/source`（见 `research/c43-source-label-step0.md`）；绝对路径输入会把物理目录带入输出。

**“模型字节没变，行为可能变”的反例**：
- **资源适配器**：`compiler.c` 与 `run.c` 把 `\0cli/source`、`cli/include-dir`、`cli/defines` 等作为资源交给 E2；网络字节不变，但适配器改变会改变 `__FILE__`、诊断和包含搜索。df1ce7ed（转发里的 caller-relative 强制包含）正属此类：阶段哈希相同，行为却按路径规则变化。pp 套件不走转发，所以它不受影响；这一点需要套件层面的证据，不能只靠哈希。
- **执行器的环境读取**：UNISA_MAXSTEPS 的读取位置在执行器（`run.c`），网络字节不变，超步行为会变（本次已由 `01262f28` 修改）。
- **宿主格**：`lnx`、`osx` 的输出与宿主库相关的运行结果，网络字节相同也可能不同。

## 5. 证据缺口与建议
1. `pptruth` 与 `pp-leaf-seal`（后者读 `tests/seals/exec-pp-literals` 并运行候选）未 strace；`pp-leaf-seal` 的读集合应以封存数据为准，不应归入“只读构造”。
2. strace 只跑了一次，且只覆盖一个输入集；读集合依赖输入的 case 也可能变化。
3. 更大的收益在 pp 之外：两候选所有已存在阶段的哈希相同，因此“按阶段哈希”的身份对其它依赖阶段网络的族也可能有效。但需要先为每个族做同样的读集合审计，否则不能扩大。
4. **声明修正（建议，不执行）**：pp 族的 `executable_inputs` 应改为假（它们不运行候选），这能去掉一个多余依赖。改动属于 `tests/gatedeps.json` 的声明，须按登记窗口处理。
