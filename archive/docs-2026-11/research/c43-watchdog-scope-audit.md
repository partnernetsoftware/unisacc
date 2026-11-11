# c43 看门狗作用域令牌：边界、环境清空清单与 C 版与回归登记（只读，2026-10-15）

对应 `research/c43-watchdog-fix-step0.md` 的草稿（令牌写入 `UNISACC_BOUND_CHAIN`，超时按环境收后代，SIGSTOP 冻结再 KILL）。本笔不改 tests/、不改登记（登记窗在 cdx）。

## ① 边界：tests/term.sh 路径不在本修法内

- `tests/term.sh` 把命令交给 Terminal.app 执行：它生成 `run.sh`，并只把**固定白名单**里的变量写入 `env` 文件（`tests/term.sh:68–78`，白名单为 UA、MODEL_COM、SEED_DIR、GATE_STATE 等）。`UNISACC_BOUND_CHAIN` 不在白名单中，所以 Terminal 里的进程**收不到令牌**。
- 进程也换了：Terminal.app 的登录 shell 不是本地 `bound` 的子树。因此本修法（按环境标记收集）在这条路径上**收不到**，超时只能清理本地那一侧。这是**边界**，不在本修法内。
- 可选后续（不在本笔）：把 `UNISACC_BOUND_CHAIN` 加入白名单。Terminal 进程与本地进程同属一个用户，Linux 的 `/proc` 扫描能看见它们，因此可以收回。这是一行白名单的改动，属于另一笔，需要另外的回归（在 term.sh 下的孤儿用例）。

## ② 清空环境的子进程清单（扫描 tests/、release/、exec/、scripts/、apps/）

**真正清空环境、令牌会丢失的：1 处**

| 位置 | 情况 | 处置 |
|---|---|---|
| `exec/c/buildffiprovider.py:132` | `env = {k: os.environ[k] for k in ('PATH','HOME','TMPDIR') ...}` 再加构建变量，随后 `Popen(..., start_new_session=True)`（`:61–62`）。令牌丢失，且该子进程另起会话。 | **报告，不改**：属 exec/c（产品闭包，cdx 在途域）。修法须在该处白名单加入 `UNISACC_BOUND_CHAIN`，或把超时交给调用方。 |

**保留令牌（复制 os.environ 后增删，或继承）：**
- `env -u NAME`（只删指定变量）：约 30 处，全部保留令牌。
- `env -i`：**全仓无**（tests、release、exec、scripts、apps 均无）。
- `dict(os.environ, …)`、`dict(subprocess_environment(), …)`（`tests/gatequeue.py:32,52–55` 只去掉 TERM_SESSION_ID、OLDPWD）、`{k:v for k,v in os.environ.items() if …}`（`tests/modelbenchcheck.py:31` 只去掉三个键）、`dict(os.environ)` 后 `pop` 个别键（`exec/c/containercheck.py:45`、`exec/c/containerlinux.py:9`）、`dict(os.environ)` + `update`（`exec/lex/compare.py:33–34`）：全部保留。
- `env=None`（继承）：`exec/parse2/libraryexportscheck.py`、`tests/librarynativehostcheck.py`、`tests/seedconstructmatrix.py`、`exec/parse/compare.py` 等：保留。

**不是进程环境（Python 数据，不影响令牌）：** `tests/manifestentries.py:139,147`、`tests/seedconstructcheck.py:2022`（`runner.env = {}` 是对象属性）、`tests/seedfactscheck.py:55`、`exec/facts/export.py:589` 的 `env = {}` 均为字典数据。

**方法的局限**：以上为静态分类（按赋值语句看是否来自 os.environ）。不是对每个调用的运行时证明。正式证明应加一个探针：在 bound 下启动一个打印自身 `UNISACC_BOUND_CHAIN` 的子进程，逐个入口跑一遍。本笔未做。

## ③ tests/bound.c 同修：设计（另一笔，与 bound.py 同笔或紧随）

- 子进程（`fork` 后、`setsid` 前）`setenv("UNISACC_BOUND_CHAIN", 外层链 + 令牌, 1)`；令牌用 `getpid()` 与时间戳拼接，或读 `/dev/urandom` 八字节。
- 超时：`freeze_tree` 之外增加按环境收集：Linux 遍历 `/proc/<pid>/environ`（`opendir("/proc")`）；macOS 需要 `sysctl(KERN_PROCARGS2)` 读环境，**本机无法验证，须在 m4pro 上核对，未核对前不得记绿**。
- 收集后 SIGSTOP 直到无新增，再 SIGKILL 全部，然后原有的 `kill(-root)` 与父子链 ledger 保留。
- 嵌套 `bound` 追加自己的令牌，外层的收集能覆盖内层的后代（与 `bound.py` 的草稿一致）。

## ④ 两条回归与 ungated 登记（待窗）

- 回归脚本：`research/c43-watchdog-regression.sh BOUND`（R1 孤儿内层子进程在超时后不残留；R2 调用方经管道读取，3 秒内拿到 EOF）。草稿已在 `research/`。
- 登记建议（`tests/ungated-checks.tsv` 的格式：作业名、处置、理由）：
  - `watchdog-orphan`：`sh research/c43-watchdog-regression.sh tests/bound.py`，R1 部分。处置：gate（修后）。
  - `watchdog-eof`：同一脚本的 R2 部分。处置：gate（修后）。
  - 同时对 `tests/bound` 二进制运行一遍（C 版）。
- 修复落地前，两条在当前 HEAD 上**应当红**（已在本机复现：原 `bound.py` 两项都失败）。登记时应明确记为“现状红、修后绿”，不得在 macOS 未核前记绿。

## ⑤ 结论

- 清空环境的真实例外只有 `exec/c/buildffiprovider.py` 一处，产品闭包内，报告不改。
- `term.sh` 是修法边界，写明在此；白名单扩展另笔。
- C 版与登记按上述设计，待窗、待 m4pro 核 macOS 分支。
