# c43 合同层第二次全扫（HEAD 1d1bf3a1 干净导出，2026-10-14）

性质：实测。逐个运行合同层套件（`tests/gatelayers.py --list` 中 layer=contract），每个 `tests/bound.py 58 sh -c <命令>`，串行。命令取自 `tests/gatedeps.json`，无则取 `tests/gate.sh` 的 job 行。干净导出 = `git archive 1d1bf3a1`，UA = `tests/build_ref.sh` 构建的参考二进制。

**数量**：列表实际为 **90** 个合同层套件（不是 89）。全部 90 个都有命令、都运行了。

## 结果

- rc=0：**71 个**。
- 非 0：**19 个**，分为环境 15、真红 4。

### 一、环境缺件（15 个，不是真红）

| 类别 | 套件 | 缺件 | 证据 |
|---|---|---|---|
| 根产品 `./unisacc.com` 不在导出里（未跟踪的本地构建物） | staticimage、staticimage-selftest、abi-manifest、abi-manifest-selftest、early-stage-seal-selftest、pp-leaf-seal、pp-leaf-seal-negative、pp-leaf-capture-selftest、middle-flow-e3、middle-flow-lower-accept | `sh: cannot open ./unisacc.com` | 在导出里放入主检出的根产品副本（10-13 构建，非 HEAD）后，**10 项全部 rc=0** |
| `${MODEL_COM:-./unisacc.com}` 默认根产品 | qprefix、ape-version | 同上 | 直接传 `./unisacc.com` 后 **rc=0** |
| 队列自测需要根产品副本（`cp $R/unisacc.com`） | queuestart-launch、queuetimeout、queuetimer | 同上 | 放入根产品副本后 **rc=0** |

注：以上“放入根产品副本后通过”只用于判定环境，所用的二进制不是 HEAD 构建，不作为 HEAD 通过的证据。HEAD 的根产品需要用 `make com` 构建后才可判定，本次未做。

### 二、真红（4 个）

| 套件 | 现象 | 来源提交 | 说明 |
|---|---|---|---|
| reviewed-trees | `stale snapshot(s): compilercheck/kernel, compilercheck/weights` | **1d1bf3a1** | 该提交改了 kernel/ 与 weights/，但没有在同一笔里刷新 reviewed 快照（派生文件须与其树同笔刷新） |
| facts-export | `facts: 30 tables, 1 differ`（exec/facts/opt-gen.tsv） | **1d1bf3a1** | opt-gen.tsv 由 export.py 从 `weights/gold/opinfo.tsv` 等派生；1d1bf3a1 改了 opinfo.tsv（72→73 行），却没有重写 opt-gen.tsv |
| gate-infra | `AssertionError: README-real-reviewed-snapshot` | **1d1bf3a1**（同上，reviewed 快照陈旧的下游） | 与 reviewed-trees 同源 |
| observation | `queue.sh on window rc 65: exit 2`（`cannot open launches-65`） | **43db8f27** | 该提交给 queue.sh 加了无条件的 static-image 预检（release/tools/queue.sh:37），用候选做 `-run`；observationcheck.sh 的候选夹具写的是纯文本 `c`，预检即拒绝。手工复现：`c: not found` → `the candidate fails the static-image contract`。另两项队列自测（queuestart-launch 与 queuetimeout）已为同样的预检换成了真产品字节，observationcheck 未改。 |

#### 验证（仅在导出副本里做，未提交，未改主检出）

- reviewed-trees + facts-export + gate-infra：在导出里执行 `python3 ./exec/facts/export.py`（写 opt-gen.tsv）→ 提交到导出的 scratch 仓 → `tests/refresh_gatedeps.py` → 提交 → 三项 **全部 rc=0**。这证实了派生链的缺口是 1d1bf3a1 漏了 facts 与 reviewed 的同笔重生成。
- observation：未验证修法。修法候选是把夹具候选换成真产品字节（同 queuestartlaunchcheck 的做法）；按今天的套路，**不直接修**，等 cc 定。

## 结论

- 合同层在 1d1bf3a1 **不干净**：4 个真红，全部来自今天的派生链：
  - 1d1bf3a1 漏了两件同笔派生：`exec/facts/opt-gen.tsv`（facts-export）与 reviewed 快照（reviewed-trees、gate-infra）。
  - observation 早于今天，来自 43db8f27（队列预检与夹具不同步）。
- 其余 71 个 rc=0；15 个非 0 为导出环境缺件（根产品），与 main 的代码无关。

## 待 cc 定

1. 1d1bf3a1 的派生补交：重生成 `exec/facts/opt-gen.tsv` 并在同一笔刷新 reviewed 快照（上面的验证即这套做法）。
2. observation 夹具：是否把候选换成真产品字节（需要根产品，且 HEAD 根产品需先 `make com` 构建）。

## 方法注记

- 命令提取：`gatedeps` 的 `command` 优先；`gate.sh` 的 `job` 行其次。`${MODEL_COM:-…}` 类写法在提取时丢了 `$`，因此 qprefix、ape-version 另行用直接路径复跑。
- 干净导出加了 scratch `.git`（部分套件要求 git 仓），仅为运行，不入库。
