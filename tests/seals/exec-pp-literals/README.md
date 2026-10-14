# exec-pp-literals measured leaf seal

实际测量，非 fixture。重新封存于提交 826233c4；其 exec/pp 表、exec/c/run.c 与 src 与 e7c05741 相同（之后无改动这些路径的提交）。

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
