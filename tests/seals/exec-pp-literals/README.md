# exec-pp-literals measured leaf seal

实际测量，非 fixture。重新封存于提交 c3d13214（含 926a8925 对 exec/pp 表的改动）；测量在该提交的干净导出上进行，两次独立 strace 的 identity 一致。

**测量命令**：`python3 exec/pp/literalcheck.py`，在仓外以 `strace -ff -yy -s10485760 -e trace=%file,read,write,getdents64` 运行，共 74 个进程、74 个 trace 文件。采集用 `tests/ppleafcapture.cx TRACE ROOT OUT`，校验用 `tests/ppleafseal.cx`。

**源名为相对名**：`literalcheck.py` 进入临时目录后以相对名 `literal.c`、`ifmacro.c`、`b_ppif.c` 调用，封存数据不含宿主路径的源名。采集器按 PID 数字序扫描 trace，使 `streams.tsv` 行序确定（readdir 序不稳定，而行序进入 output-proof）。同一树两次独立 strace 得到相同的 core 与 proof。

**内容**（由 `ppleafseal.cx` 核验）：
- `streams.tsv`：44 次模型执行（6 接受、38 拒绝；每模式 3 个 aggregate 源流与 18 literal + 1 macro arity 拒绝），两个模型。每行给出 input、stdout、stderr、model、显式源名的文件/长度/SHA256。
- `resources.tsv`：14 次模型侧资源响应（两模式各 7 个头）。
- `dependencies.tsv`：91 项，由实际 read/exec 收集；`closure.expected.tsv` 为同一清单，漏一个实际 pp 规则必须拒绝。
- `identity.tsv`：固定 core 与 output-proof；核验重算后须逐字节相等。
- `measurement.tsv`：固定元数据。

**已知残留**：`resource-b0b3…bytes` 含 `/tmp` 字样，它是 `include/stdio.h` 注释里的 `"/tmp/untm"` 原文（仓库头文件内容，不是宿主路径）。除此之外封存目录中没有 /tmp 或 /home。

**不入仓**：原始 trace、tools.tsv（绝对工具路径）、queries.tsv、directories.tsv、observation.tsv。它们是观测记录，不参与核心检查，只保存在仓外。`--check-observation` 只报告 NOT_RULED。

**负例**（`--negative`）：在私有镜像上漏实际规则依赖、给 predefines.tsv 增加一行、给 b_ppif.c 间接使用的 stdarg.h 增加一行，三项均必须判红；共享源码不改。

**两份 net 模型**保留实际执行字节，尚未证明构建器可逐字节确定性重建，因此不能只留哈希。

## 2c3 split-at owner-prefix 后同源重采

在 HEAD4e2c266a+本笔的干净导出（index-tree 见仓外 source.json）上，以同源私有P3产品 `665334b0edf32df52e845539e3a53db0f19430f1b1120c6b2dce33a9365454e8` 执行仓内 `tests/reseal-pp-literals.sh`（含 `ppleafseal.cx --write-seal`）。12产品构建步全rc0、最大20.669s；四重采步全rc0、最大33.65s。

逐项变化只有 `dependencies.tsv` 中 builder `exec/finite_rules.py`：长度38371→38562、SHA4a8151fa…→2a858163…，原因是通用 split-at 可选owner-prefix。全部case输入/输出/错误/源名、两个model产物、resources/streams/closure.expected/identity/measurement均逐字节不变。原件实际93依赖（上方91是历史说明），6收38拒/两模型/core/output-proof保持原值。没有手填新identity或放宽seal，也不授复用。

仓外保留 source.json、12步build-steps.json、raw trace、reseal.tsv、file-comparison.json；保存于仓外本轮 `2c3/joint/pp-reseal/`，不把宿主绝对路径放入封存输入。

## 0.0.43 F1 同源重采

本轮以七补丁与 cx_read 容量边界补修后的根产品 `4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda` 正式重采，四步均 rc0，最大单步31.17秒；内容变异负例均拒绝。93依赖、6接受、38拒绝、两模型。采集器现接受strace对齐空白，见 research/c43-ppcapture-padding.md；实际凭据见 research/c43-f1-evidence/pp-reseal.tsv。原始trace留scratch仓外目录，封件不含观测文件；本README为手写历史说明，不在capture目录摘要之内。不授复用、不声明发布。
