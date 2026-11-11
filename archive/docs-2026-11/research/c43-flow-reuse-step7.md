# 0.0.43 第 6 步第七刀：名单外读取审计的剩余部分（rowcov 族）与第三批测量（只读，不改登记文件）

标签：**[读源码]** = file:line；**[实际跑过]** = strace `-f -e openat`（≤55 秒，超时即记为部分读集）。
登记文件未改动：声明改动仍攒在仓外，等 cdx 提交后由 refresh_gatedeps.py 一次落地。

## 1. rowcov 族：源码层面没有名单外读取 [读源码]
- `tests/rowcov.py` 中提到 `research/` 的唯一位置是文件头的设计说明（`tests/rowcov.py:4`，docstring 里的文件名）。整份源码没有打开 `research/`、`plans/`、`prd.md`、`spec.md`、`archive/`。
- 剩余 44 个作业全部是 `tests/rowcov.py` 的不同参数（`python3 ./tests/rowcov.py <族> <分片>`，已逐个核对命令，`[计算]`）。
- 因此静态清单中 rowcov-* 的命中全部是**误报**。

## 2. 实际测量：rowcov 单片超过 55 秒 [实际跑过]
- 第一批的 `rowcov-pp-locations`、`rowcov-pp-shared`、`rowcov-parse2-build`，以及第三批的 `rowcov-parse2-1..10`（参数 `parse2 gate1/24` 等）**全部在 55 秒处超时**（rc124）。
- 超时前打开的仓库文件都在名单之内（`outside_reads.py`：名单外 0）。这是部分读集，不是完整读集。
- 尝试用分片参数单测一片：`rowcov-parse2-*` 本身就是单片（`gate1/24`，即 24 片中的第 1 片），仍超过 55 秒。无法在 55 秒内取得完整读集。

## 3. 结论与后续
- 44 个 rowcov 作业中，源码证明没有名单外读取（§1）。这是比 strace 更强的证据，足以把它们从“待声明”中移出，不需要声明。
- 已测的 13 个 rowcov 作业（第一批 3 个超时、第三批 10 个超时）在超时前也没有名单外读取。
- 剩余待测的 rowcov 作业（共 34 个）同样只需源码审计，不再逐个 strace。
- 第三批其余 34 个作业中，rowcov 族已由源码排除；`tests/` 之外的 `exec/`、`unisa/` 作业仍需 strace，且第三批已经覆盖的 10 个全部是 rowcov，因此下一批应从非 rowcov 作业中取。

## 4. 本刀不改动的文件
- `tests/refresh_gatedeps.py`、`tests/gatedeps.json`：未改动（登记窗由 cdx 持有）。
- 声明补丁：本刀**没有需要新增的声明**（rowcov 族无名单外读取；其它已测作业见 `research/c43-flow-reuse-step6.md`）。
