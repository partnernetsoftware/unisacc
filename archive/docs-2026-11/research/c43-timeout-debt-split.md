# 0.0.43 超时债拆分提案（只读，不放宽 60 s）

标签：**[实际跑过]** = 本次在本机用 `tests/bound` 或仓外计时脚本实测；**[读源码]** = 只读代码与日志。
约束遵守：每次运行 ≤ 58 s；串行；未改产品、套件、gatedeps、队列；未碰 exec/lower、exec/c、tests/tapedomaincheck.cx、plans/；未触碰 /tmp/c42-7000（只读其日志）。计时脚本在 `/tmp/c43-tbprobe/`（仓外）。

## 0. 先更正一个前提 [读源码 + 实际跑过]
- 套件被**队列**限时，不是被 60 s 限时。`tests/gatequeue.py:773` 把每个作业的 limit 设为“窗口剩余时间 − 1”，7000 日志里的 limit 在 40–49 s（`START … limit=…`）。`com-tapebin-1..3` 在 7000 闸里 `rc=142 42.05s`（[读源码] `/tmp/c42-7000/cand.queue/release-queue.log`），是**窗口 42 s**截断，不是 58 s 截断。
- 因此“每片超 58 s”只说明**安静机器上的单跑**需要 ≥ 58 s；在窗口里真正的约束是 42–49 s。拆分的目标应按窗口算。

## 1. com-tapebin-1..3（tests/tapebinproduct.py）

### 1.1 实测构成 [实际跑过]
| 项 | 实测 | 说明 |
|---|---|---|
| `build_ref.sh`（每片各一次） | **11.3–11.5 s** | 每片都重建参考编译器（`tapebinproduct.py` main 的 `run(["./tests/build_ref.sh", …])`） |
| 片 2 单跑（`TAPEBIN_SHARD=2/3`，58 s 上限） | **rc142，58.1 s** | 完整片超时 |
| 片 2 前 20 个文件（墙钟 22.8 s，扣除 build_ref 后 11.3 s） | 子进程：产品 tapebin 3.9 s、产品文本 2.8 s、参考 0.6 s；其余为 Python（编解码等） | 每文件约 0.55 s（墙钟口径；Python `encode` 本身约 0.08 s/文件） |
| 片 3 前 20 个文件 | 同量级：墙钟 21.7 s（含 build_ref 11.3 s） | 与片 2 一致 |
| 片 1 固定部分（空切片 `0 0`） | **15.8 s**（build_ref 11.3 + 六目标镜像/运行/origin 守卫 ≈ 4.5 s） | 片 1 独有 |

**推算全片**（86 文件/片，0.55 s/文件）：片 2、片 3 ≈ 11.5 + 47 ≈ **59 s**；片 1 ≈ 16 + 47 ≈ **63 s**。与实测 58.1 s 超时的片 2 量级吻合（[推算，依据实测]）。

### 1.2 拆分方案
1. **参考编译器只建一次**：新增一个准备步骤，`build_ref.sh` 在队列外或作为独立作业写出 `ref`（按源码闭包哈希命名）。片作业通过环境变量读取，不再自建。省约 34 s/三片合计（[推算]）。
2. **六目标固定部分单列为作业 `com-tapebin-six`**（片 1 独有，≈ 4.5 s + 读参考）。
3. **文件片改为 6 片**（258 文件，每片 43 个）：每片 ≈ 43 × 0.55 ≈ **24 s**，加上固定部分后最大约 **29 s**（片 1）；低于窗口 42 s，留余量 ≈ 14 s。
   - 不推荐 3 片（每片 46 s 仍贴近窗口）；4 片（每片 ≈ 34 s）也可，但余量小。
4. 准备与执行分离：ref 的准备是独立作业，片作业只执行。准备失败时片作业应失败并指明“准备缺失”，而不是静默自建。

### 1.3 覆盖项数不减的对照法
- **切片不变式**：`files[k-1::n]` 对 k=1..n 的并集必须等于全集 258 个文件，且两两不交。在 `tapebinproduct.py` 外做一次性断言即可（[读源码]：现有切片本身满足）。
- **逐文件判定对照（拆前 = 拆后）**：拆前无法整片跑完，所以用**同一代码、按 ≤20 文件切片**逐段跑，记录每个文件的判定（matched / refused / known_diff）。拆后用新片跑同一批文件，逐文件比较判定集合；两边总数必须相等。
- **已知差异集合的全局核对**：现在 `known_tape_diffs &= names` 是**片内**交集，片间不做全局相等。拆片后若某个已知差异文件落在没有断言它的片里，会静默失去覆盖。缓解：拆后在汇总步断言“观测到的已知差异 = `exec/c/chain.knownfail` 全集”。（[读源码]，这是拆分引入的新风险，必须补。）
- 每个片仍打印 `N equal-source packages, M known upstream tape diffs`，汇总时对 N、M 求和并与 258 对照。

## 2. 审 cdx 的 `research/c43-driver-preparation-split-proposal.md`（557dff98）
它的方向对：准备分片、完整清单后才发布、执行时重算身份、缺失即失败、不以 `STAGE_MODELS_READY=1` 绕过。身份依据 `exec/pipeline/models.py:49–70` 与 `exec/c/compilerpack.py:93–121` 也确实存在（[读源码]）。缺口如下：

1. **没有对准窗口预算**。提案说“外层与内层 ≤ 60 s”，但队列给的是 40–49 s 的窗口剩余（见 §0）。准备分片必须以**窗口**为单位设计，否则 47 s 的单步仍会在窗口里被截断。
2. **准备没有自己的作业身份**。提案说“准备结果不是测试通过”，但没有说准备在队列里是哪个作业、以什么 gatedeps 条目出现。没有这一条，队列的指纹看不见准备的失败与重做。
3. **“缺失即失败”的 rc 未定**。若准备缺失记为 rc1，队列会把它当产品红，而它其实是“需要先准备”。建议单列一个返回值（例如 rc75 “需要准备”，与队列已有的 75 语义统一），由队列重排准备作业而不是报红。
4. **清单发布的原子性未写**。`compilerpack.py` 已有 `fcntl` 锁（[读源码]），但提案只写“成功后发布”。要写清楚：先写临时目录、校验摘要、最后原子重命名；并且多个准备分片共用同一把锁。
5. **27 个比较的执行成本未拆**。提案说“执行必须重算身份并核对每个产物摘要”，但没估这一步本身的耗时；在一个窗口里它只能是 ≈ 24 s（cc 报告的热缓存值，[读源码/未复现]），与准备加起来仍可能超窗口。应把 27 个比较也分片。
6. **“47 s 冷”未复现**。cc 报的 47 s 来自一次冷缓存；本次只在热缓存下测到 `exec-driver-core-modes` 20.7 s（7000 日志）。我没有做冷缓存实测（需要空的 `UNISACC_MODEL_CACHE`，约 47 s，同机又有 7000 在跑，按约束未做）。**这一点需要在空闲机上单跑确认。**
7. **与 `pack-prep` 现有步骤的关系未说明**。`tests/gate.sh` 已有 `seedgen-*consumer` 等准备类作业；提案应说明新准备步骤是替换还是并列，否则会有两套准备。

## 3. 同类排查：产品闭包一变→缓存全冷，必超窗口的套件 [读源码 + 7000 日志]
判据：命令会构造或编译模型网络/驱动（经 compilerpack 或 build_ref），且 7000 日志里热缓存时已接近窗口。冷缓存只会更慢（[推算]）。

| 套件组 | 命令 | 7000 热缓存时长（窗口 ≈ 42 s） | 备注 |
|---|---|---|---|
| exec-*self 系（12 个：macself、winx86self、winself、macxself、selfelf、armself，及其 -package 变体） | `exec/pipeline/selfcheck.sh` | 37–43 s，多个 rc0 贴边 | 冷缓存必超 |
| exec-driver-* 系（resources 39.7；core-contracts 23.6；core-modes 20.7；core-build 19.8；core-dependencies 16.3；language-1 15.2） | `exec/c/compilercheck.sh …` | 15–40 s | cc 报 core-modes 冷 47 s |
| target-package | `targetpackagecheck.py {MODEL_COM}` | 36.9 s | 需核其是否构建 |
| exec-e3self | `exec/parse2/selfcheck.sh` | 32.2 s | |
| tapebin 片 1–3 | `tapebinproduct.py`（每片 build_ref） | 42 s（窗口截断） | §1 |
| exec-memory-cc-1/2、exec-memwinarm/x86 | `memorycheck.sh`、`winmemorycheck.sh` | 14–24 s | 热缓存已接近；冷缓存需复核 |
| com-luatests-build | 未在 gatedeps 中给出 command（`None`） | 日志 rc142 @43 s | 提交 370e40ee 写明冷 lua 构建约 48 s [读源码] |
| com-fb12-multi | `fb12multi.sh` | rc142 @42 s | 需读脚本确认是否构建 |
| closure-c1–c4、stages-1..3、tools-2、shared-e2-plain、rowcov-*、exec-e5、exec-arm | 各自脚本 | 多数 rc142 @ 42–44 s | 热缓存已超窗口；多数是运行时间而非构建，不属于“缓存全冷”，但需单独排查 |

**判断**：第一类（selfcheck 12 个）与第二类（compilercheck 系）是“缓存全冷必超”的主体；tapebin 与 com-luatests-build 是构建内嵌在套件里的同类问题。其余 rc142 多数是本身运行时间长，需要另起议题，不应与缓存冷混为一谈。

## 4. 下一步（建议，不执行）
1. 在空闲机上对 `exec-driver-core-modes` 做一次冷缓存单跑（确认 cc 的 47 s）。
2. 按 §1.2 先拆 tapebin（改动在 `tests/tapebinproduct.py`，属套件，需主人批准）。
3. 对 §3 第一、二类，先建“准备作业”的设计再动套件；不要放宽 60 s 或窗口。
