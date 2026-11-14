# cx-lab

实验性示例,不属于产品。均为 `.cx`,在仓库根运行:`sh ./unisacc.com examples/cx-lab/NAME.cx`。
所有产物写到 `$CXLAB_OUT`(未设置时为 `${TMPDIR:-/tmp}/cx-lab-out`),不写仓库内任何文件。

| 文件 | 作用 | 预期输出 | 写出位置 |
|---|---|---|---|
| cxlab.cx | 共享模块(`#pragma unisa use cxlab`):TSV 加载、命令中位耗时、`cxlab_outdir` | 不单独运行 | 无 |
| lex_table.cx | 用 charclass.tsv 与 transition.tsv 两张表驱动一个通用引擎,对 sample.c 做词法切分 | `lex_table: ok tokens=88 classes=9 rules=29`,rc 0 | `tokens.tsv` |
| time_run.cx | 对若干 `unisacc.com` 调用计 3 次中位耗时 | 逐行 `名称 rc=N 秒数`,末行 `time_run: wrote <目录>/times.tsv`,rc 0(其中 run sample 的 rc=2 是 sample.c 自身退出码,历史如此) | `times.tsv`、`sample.o` |
| runtimeiocheck.cx | 原 runtimeiocheck.py 的移植:二进制 IO 字节保持检查 | 当前为具名拒绝(rc 2):依赖的归档 C 源码无法再用 plain cc 链接;不是 PASS 也不是 FAIL | `bytes.bin`、`runtimeio.tsv` |

数据文件:charclass.tsv(字节到类)、transition.tsv(状态与类到动作与下一状态)、sample.c(被切分的输入)。

runtimeiocheck 的依赖:`archive/testing-c43-legacy/tests/runtimeiobytes.c` 与 `archive/reference-c/exec/c/run.c`,
后者需要 `cxcodec_*` 符号(codec.h 声明,现由 .cx 实现),所以拒绝是确定性的。

根下 9 个 C99 示例(do fact fib hello host indirect6 ptr struct switch)由批次 examples-smoke 覆盖:
`sh ./unisacc.com tests/check-examples-smoke.cx`。
