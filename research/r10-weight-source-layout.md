# v0.0.10：权重与源码布局审计

状态：**只读源码审计与设计，未改生成器、未构建、未跑套件**。2026-09-28。

## 1. 结论

用户提出的“源码只装配、权重由表构造并单独引入”合理。这里有两套不同的权重，不能把经典参考的 `MODEL` 当作出货 P3 中的全流水线模型：

| 对象 | 现状与来源 | 合适的存储形态 |
|---|---|---|
| 经典参考的 18 个决策阶段 | `unisa/ckernel.py:emit_core` 把 `weights/built.json` 的网络打包成 MODEL；DENSE 是逐键运行该网络得到的答案；二者在 `kernel/unisa_model.inc`，再被 `tests/build_ref.sh` 拼入 `unisacc.c` | 生成的 `.inc`，由经典装配源 include |
| 默认产品的 33 个共享网络 | `exec/c/buildcompiler.sh` 调各阶段生成器写 JSON → `tbl.py` → `net.py`；`compilerpack.py` 配路由，`pack.py` 对相同网络字节去重，写二进制+DEFLATE P3；`packageformat.py` 能离线解包 | 继续独立 `.net` 与去重 P3，不额外转成 C 字面量副本 |

33/1082 是 `research/r9-pipeline-structure-prune.json` 中 v0.0.9 载荷的网络数/引用行数，不是本轮新测量。把 `unisacc.c` 改成 include 装配源能消除仓库中大段数据与程序的混排，但**不会自动缩小产品二进制，也不删除网络和语法规则**。P3 产品权重已经不是 `unisacc.c` 内的 C 数据。

## 2. 真值来源不能扩大结论

- 经典常规 `build-weights` 由 `unisa/gold.py` 规则定义构造，导出 `weights/gold/*.tsv`；现有 `--from-tsv` 路径可以从 TSV 的答案构造同一网络，但阶段集合、顺序、容器 schema 仍借用 `gold.ALL`/`gold.Stage`（`unisa/__main__.py:_build_from_tsv`）。
- `emit_core` 读 built.json；维度与词表仍读 `gold.py`，TYPEKW 仍读 `front/lex.py`，ENC 仍读 `catalog.ENCSPEC`。J10 `iterate/` 的 TSV 声明/C 生成器是已保留的开发路线，不是当前 `emit-kernel` 的唯一权威。
- P3 各阶段 δ 由 `exec/*/gen.py` 和 `parse2/gen2.py` 的显式结构/规则构造；它们读取部分 gold TSV、catalog 等静态声明。**当前不能写成“33 个网络完全仅由现有 gold TSV 构造”**。源码布局切片不顺便重写这些规则来源。

## 3. 最小实施：先分离装配源与独立导出

### 3.1 第一片（建议立即做）

1. 根 `unisacc.c` 改为短小的经典参考装配入口，按既有依赖顺序 include version、生成模型、生成内嵌头、推理核及 `src/*.c`；不含权重字面量。
2. 当前 `kernel/unisa_model.inc` 已满足“数据单独生成”这个核心要求，第一片无需改 MODEL/DENSE 的 ABI。文件名可选 `kernel/weight.classic.inc`，但若只是改名，它仍是 18 阶段整包，不宣称每网已独立。
3. 增加单一、明确的 flat 导出入口，例如 `tests/export_ref.sh OUTPUT`，使用 shell 基础文本工具按唯一 manifest 装配。输出 `out/unisacc-flat.c` 或套件私有目录，绝不写回根 `unisacc.c`。离线导出与编译经典自举不要求 Python。
4. 让 `tests/build_ref.sh` 调此入口，产出私有 `UA.c` 后编译；它不再每次重写受跟踪的根文件。flat 仅为可重建交付物，不提交另一份含完整权重的根源码副本。
5. 装配 include 注意重复：`unisa_core.c` 本身 include 模型，`back_encode.c` 本身 include host_dl.h。现有 flatten 会剔除这两行；新入口必须沿用同一逻辑，或给生成模型加生成的 include guard。不要同时显式与间接重复定义。

推荐第一片保持现有模型命名，先完成真实源/导出分离；`weight.<name>.inc` 细粒度命名列入下一片，而不是以重命名冒充功能成果。

### 3.2 每个经典网络 `.inc`（第二片，仍不改推理内核）

MODEL 和 DENSE 当前均为连续串，`STAGE_OFF`/`STAGE_DOFF` 假设全局连续位置。直接改成 18 组数组会改内核及消费者，超出源码组织任务。

可行的最小实现：生成 `kernel/weight.<stage>.inc` 与 `kernel/dense.<stage>.inc`，内容是同一连续串的逐阶段相邻 C 字面量片段；`unisa_model.inc` 在 `char *MODEL =`/`char *DENSE =` 后按顺序 include 各片段，最后一个分号由装配文件写出。所有 offsets、维度和运行 ABI 保持。C 端已有相邻字面量生成形式（`_cstr`），但新 include 边界和单字节为空时需实际验证。

只保留片段为数据源、聚合文件为引用/维度/词表；不要把完整 MODEL/DENSE 还写在聚合文件里。flat 导出必须识别这些数据 include，展开同一片段。片段序列化如改变字面量换行，不要求 flat 源文本和历史文本相同；要求拼接后的权重字节、tape、镜像及行为一致，分别记录证据。

## 4. 精确影响清单

| 文件/领域 | 最小需要改或核对的内容 |
|---|---|
| `unisacc.c` | canonical include 装配源，注明经典参考/回退 |
| 新 `tests/export_ref.sh` 或同等唯一导出工具 | 按声明顺序导出 flat；所有生成 include 展开；输出私有；不调用 Python |
| `tests/build_ref.sh` | 不再改根源码；flat 产出后加 refshim/reffoot；保留返回码与私有目标路径 |
| `unisa/ckernel.py` | 第一片可仅为模型 include guard；第二片负责生成所有片段和 provenance，不能手改 kernel |
| `kernel/unisa_model.inc`、`kernel/unisa_core.c` | 按生成规则重建；第二片新增 weight/dense 片段；整包只存一次 |
| `tests/kernel.sh` | 现有遍历输出能检查新增片段，但需检查完整预期清单、陈旧孤儿片段、空清单；不能遗留已删除网络片段 |
| `tests/lib.sh` | ua_ready 输入签名覆盖装配 manifest、导出工具及片段；现在只 hash kernel/src/refshim/reffoot，漏导出脚本与根入口 |
| `Makefile`、`unisa/__main__.py` | classic-com/ape 可编 canonical include 源；显式单文件导出命令与文件名统一，不改变默认模型产物 |
| `tests/nativeboot.sh`、`bootstrap.sh`、`bigclosure.sh`、`selfhost.sh`、`opt.sh`、`optpy.sh`、`scale.sh`、`bench*.sh`、`run.sh` | 依赖独立文件的自举/VM 输入改用唯一 flat 导出；VM `file push` 目前只发 unisacc.c，不能发送只含 include 的入口而遗漏依赖 |
| `exec/c/memorycheck.sh`、`tests/strconvertroute.py`、`tests/bench_vs.sh` | 当前直接 cat refshim+unisacc.c+尾部：私有 /tmp 源上的相对 include 会找错位置，须先 flat 导出 |
| `tests/gate.sh`、`exec/opt/check.sh` 自身语料 | 决定把 canonical 与 flat 均列测试还是用明确 flat 输入；保持固定语料意义，不能以入口变小规避自身源码回归 |
| `exec/pipeline/models.py`、`exec/c/provenance.py`、`tests/gatequeue.py` | `.inc` 已在源码闭包内；新增装配 manifest/导出工具必须明确纳入相应 reference/product/suite identity。flat 放 `out/` 不作为模型生成输入，避免构建循环失效 |
| `ARCHITECTURE.md`、`AGENTS.md`、README、prd、论文 | 改“根源码就是权重+程序拼接”叙述为“装配源+显式独立导出”，继续分别说明18阶段参考与33网络产品 |

审计见到 89 条 `unisacc.c` 引用（grep 匹配行计数，不是89个必须改的文件），不可只改 build_ref 一个位置后宣称完成。

## 5. 有限验收与停止点

第一片：导出不改根文件；cc 与 unisacc 均编装配源与 flat；对同一目标比较 tape/镜像，并执行回归；nativeboot 继续 N1=N2=N3；kernel 生成检查；MODEL/DENSE 字节核对；产品包路由及现有 --check-net 全域结果不变。冻结后再跑对应门禁。

第二片：再加逐片长度/顺序/内容哈希清单，缺片、乱序和陈旧片明确失败，确认总权重没有额外一份；自举/六目标引用图与测试继续通过。无需新训练、第三个编译器或新的权重算法。

本报告是设计/引用审计，以上验收均未在本轮执行。父会话应先把决定写入 prd，再统一实施与重建。
