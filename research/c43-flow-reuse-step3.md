# 0.0.43 第 6 步第四刀：现有队列复用是否因声明缺口而不安全（只读审计）

标签：**[读源码]** = file:line；**[计算]** = 对现有 `git ls-files` 名单与 gatedeps 的只读计算（未改文件，未跑队列）；**[实际跑过]** = strace `-f -e openat` 单作业（≤55 秒）。

## 1. 队列的真实指纹：不是“声明的文件”，而是全局名单 [读源码 + 计算]
- `tests/k2b-whitelist.tsv` 的数据行为 **0**，所以没有任何套件走“窄指纹”（`tests/gatequeue.py:253–262` 的 `k2b` 只对白名单生效，其它一律“whole tree”）。
- 对所有套件（已声明或未声明，`:404–407`），指纹都包含 `global_inputs = {n: digest(n) for n in names}`；`names` 是 `git ls-files --cached --others` 在 `src exec tests include kernel weights unisa examples unisacc.c README.md ARCHITECTURE.md AGENTS.md prd.tree.md prd.map.md research/referee.tsv iterate release scripts Makefile` 下的 **3280** 个文件（`:186–189`，`[计算]` 重算核对）。
- 因此：**仓库名单内的任何文件改动，都会让所有套件（含未声明的）重跑**。声明缺口在队列里不会造成复用错误，只要读集合落在名单内。这修正了我上一刀对“声明缺口”的表述：affected.py 的数字只是挑选估计，队列本身更保守。

## 2. 真正的漏洞：读集合落在名单之外 [实际跑过 + 计算]
名单外且会被套件读取的文件（strace 实证）：

| 套件 | 名单外读取 | 名单中是否存在（计算） |
|---|---|---|
| exec-warnprep-0..3（4 个作业）、exec-driver-language-2 | `seed/ape.c`、`seed/blob.c` 等 12 个 `seed/` 文件（作为构造工具的源码，由 cc 现场编译） | `seed/ape.c` **不在** 名单（NO）；两者声明里也没有 seed（`files`/`trees` 为空） |
| exec-multi-ua-n-reverse | 同上 | 名单外，但它声明了 `seed` trees，所以声明的输入里有它（已覆盖） |
| ledgercheck | `plans/roadmap-0.0.x.md`、`plans/v0.0.41.md`、`plans/v0.0.42.md` | `plans/*` **不在** 名单（NO） |
| freezecheck | `.git/HEAD` | 不是文件内容，是分支元数据 |
| 其它 | `/tmp`、`~/.local` 的 Python 包、系统 cc | 不在名单（外部，靠 env-pin 与 provenance 处理） |

**影响面**：`seed/` 中被构造工具编译的源码改动后，上述 **5 个作业**（warnprep ×4、driver-language-2）与 `ledgercheck` 会**复用旧结果**：改 `seed/ape.c` 不改候选字节（它只改构造工具），也不进任何名单。这是发布闸的正确性问题，因为发布闸的前提是“测试结果不跨源码变化复用”（CLAUDE.md）。在 10 个已抽样的作业中，3 个读 `seed/`（30%），所以未抽样的作业里可能还有更多，需要全量 strace 才能定。

## 3. 最小修法（只写设计，不改 gatequeue）
1. **结构修法**：名单不要手写，而是 `models.closure()` 的文件集 ∪ 所有声明输入 ∪ 已审计读集合（`exec/pipeline/models.py` 已有闭包遍历）。这样名单与产品闭包一致，`seed/` 的工具源码自动进入。代价：一次性全量重跑。
2. **局部修法**（不动队列）：把 `seed` 与 `plans` 作为 `trees` 声明给这 5+1 个作业。已声明的 `trees` 会进入指纹（`inputs`，`:407`），因此可以生效；但需要确认非家族套件的 `trees` 确实被摘要（`inputs` 的计算方式未在本刀逐行核对）。
3. 无论哪种，`plans/` 与 `prd.md`（当前只作 guard）也应进入名单，否则 ledgercheck 的读集会一直落在外面。

## 4. 第三步：未声明组（other，78 个）的读集草案与比对 [实际跑过]
other 组实际读集（strace，三个代表）与现声明（均无声明）的比对：

| 作业 | 实际读取（仓库内，去 pycache） | 草案 files | 草案 trees | 名单内？ |
|---|---|---|---|---|
| gate-layers | `tests/gatelayers.py`、`tests/bound.{py,c}`、`tests/lib.sh`、`tests/seedconstructcheck.py`、`exec/facts/parse-{constants,tokens,words}.tsv`、`parse2-extra.tsv`、`valueranks.tsv`，`exec/build`、`exec/facts` 目录 | 上述 `tests/*` 与 5 个 facts | `exec/build`、`exec/facts` | 是 |
| ledgercheck | `tests/ledgercheck.py`、`src/version.h`、`plans/roadmap-0.0.x.md`、`plans/v0.0.41.md`、`plans/v0.0.42.md` | `tests/ledgercheck.py`、`src/version.h` | `plans` | **plans 否** |
| freezecheck | `tests/freezecheck.py`，`.git/HEAD` | `tests/freezecheck.py` | （无） | `.git/HEAD` 不可声明为文件，应以 `git rev-parse HEAD` 作为戳 |

草案只放在 `research/`（见 `research/c43-undeclared-draft.tsv` 与本节），**没有写进 gatedeps**。它与现声明的比对结论是：other 组三个代表全部未声明；草案的 trees 与名单的关系说明，依赖漏洞集中在 `plans/`、`seed/`，而不在 `tests/`、`exec/`。

## 5. 对前几刀结论的修正
- 第三刀的 `--declared-reads` 假设模式只描述“若队列按声明收窄”会怎样；**队列当前并不按声明收窄**（白名单为空），所以那些数字是假设，不是现行行为。它仍可用于日常挑选，但不能当作队列的节省。
- 现行队列的真实风险不是“声明不全导致漏验”，而是“名单之外的读取导致复用”。两者要分开报告。
