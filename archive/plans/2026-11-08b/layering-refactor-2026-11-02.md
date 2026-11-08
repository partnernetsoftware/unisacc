# unisacc 分层收敛固化方案（cc，2026-11-02，主人要求“马上全面反思并重构”）

## 1. 反思（事实，不辩解）

1. **“上层不得直调 libc，平台调用只能进 L0”被用成了“凡是需要系统能力的东西都进 cx/sys”。** 钢印本意是隔离平台差异；执行结果是 L0 成了所有人的堆放处。工具要回收子进程 → 进 cx/sys；应用要 TCP → 进 cx/sys；有人要 TLS → 进 cx/crypto。
2. **“产品”与“产品周边”没有目录边界，也没有依赖方向的强制。** 实测（今天）：产品链路（src、cx/runtime、cx/compiler、cx/driver、cx/core）只 `use` 约 14 个 L0 模块（mem.heap/bytes、sys.fs/env/io/time/proc/posix/mem/kernel/dl/sha、text.text、core.memory）。其余——整个 cx/crypto（15 模块 3.4k 行）、sys.reaper/childdepth/process/net/tty/poll/guard/fd/ffi、mem.arena/slice/grow、text.num/fmt/build/deflate——**产品一行都不用**，只有 tools/tests/apps 在用。
3. **裁定写在文档里，没有机器强制。** 2026-10-31 主人已裁定“生产传输继续用 libcurl 经 sys.ffi，不自实现 TLS”，同一天仓里完成了约 3400 行原生 TLS。
4. **模型容量与结构耦合。** UNITOK2=64 单元写进 K2 表；拆分 dv_main 越线，整程序编译拒绝。
5. **验证体系是产品的 5–8 倍，且在被验证的语言里自造，自身故障面很大。**
6. **进度度量偏基础设施**（本周提交 1215:313）。

## 2. 目标分层（固化 = 每层有契约文件 + 冻结的接口清单 + 棘轮 + 回归）

| 层 | 内容 | 对上依赖 | 规则 |
|---|---|---|---|
| P0 平台核心 | cx/sys（仅产品链路用的 12 模块）、cx/mem（heap/bytes）、cx/text（text） | 无 | 导出符号清单冻结；新增必须指向“编译器/装载器/某架构”的具体需求 |
| P1 执行器与运行库 | cx/core、cx/runtime | P0 | 无 libc；字节账固定 |
| P2 编译器胶水与事实 | cx/compiler（生成）、exec/、weights/、kernel/ | P0,P1 | 生成物只来自事实表→生成器；不手改 |
| P3 驱动 | src/、cx/driver | P0–P2 | srcshape 棘轮（只降） |
| X 扩展 | ext/crypto、ext/sysx（reaper、childdepth、process、net、tty、poll、guard、fd、ffi）、ext/text（num、build、fmt、deflate）、ext/mem（arena、slice、grow） | P0 | **产品层禁止 use ext.\***；由新棘轮强制 |
| D 开发工具与测试 | tools/、tests/ | P0–P3、X | 行数棘轮（只降）；新工具须指向产品阻塞 |
| 非产品 | apps/、ujs/、research/、archive/、release/… | — | 迁出（最后一波，需文档治理窗口） |

## 3. 波次（owner 在括号里；每波一个小提交序列，棘轮 7/7 才算完）

| 波 | 内容 | 风险 | owner |
|---|---|---|---|
| W0 今天 | **冻结令**：cx/sys、cx/mem、cx/text、cx/crypto、cx/core 禁新增模块；工具/应用需要的 OS 能力一律放 tools/cxlib 或 ext/（不进产品链路）。扩展 tools/check-cx-layers.cx 加“产品层禁 use ext”断言 | 低 | cc 发令；ds 实现检查 |
| W1 | `cx/crypto` → `ext/crypto`（命名空间 crypto 不变，retarget tools/crypto 与 tests/ 符号链接）；更新基线表中的路径；批次不变 | 低 | omp-ds |
| W2 | 工具/应用专用 L0 模块迁 `ext/sysx`：sys.reaper/childdepth/process/process_group*/net/tty/poll/guard/fd/ffi；模块改名 sys.X→sysx.X，用 tools/refactor.cx rename 分批扫；每批全量编译 + affected | 中（机械、点多） | omp-ds（含 sys.sig 是否迁另议） |
| W3 | `mem.arena/slice/grow`、`text.num/build/fmt/deflate` 同样判定；产品用的留，不用的迁 | 低-中 | omp-ds |
| W4 | 生成物去重：compiler/runtime 双份合一，523 个迷你名字表构造期生成不入库；cx-flat.h 只作构造产物 | 中 | omp-go（加速线之后） |
| W5 | **容量与结构解耦**：UNITOK2 容量改为构造参数/大幅上调；慢车道纪元（omp-bdy 试点已打通到生成阶段） | 中-高 | omp-bdy |
| W6 | 工具瘦身：tools 行数棘轮；归档 84 个无引用文件；合并重叠；统一入口 | 低 | omp-ds / omp-go2 |
| W7 | 顶层分区：apps/ujs/research/archive/release 迁出 | 低（纯移动）但需治理窗口 | cc 择日 |

## 4. 验收（每波都要）

- 产品字节等价：`-o`/`-S` 对固定输入集 sha256 不变（冷/热）。
- `tools/ratchets.cx` 7/7，加新棘轮 `layer-boundary`（产品层 use ext.* → 红）。
- `tools/check-cx-layers.cx` 对新层表通过；p1-baseline 12 项绿。
- affected 对被迁移文件的调用方全跑；迁移不得删断言、不得放宽基线。

## 5. 不做的事

- 不重写模型层；不在这一轮动权重。
- 不一次性大搬家：每波独立可回滚，主线（加速/修bug）不停。
