# 首红后后续任务零启动：补证材料（cc；0857 ONE=(A)；草案，未实跑）

**状态：r3 已实跑，命令层限定闭合，其余限定仍保留。** r3 证据见 `first-red-run-0902/` 与 §11；身份见 `first-red-identity-0910.md`。§2–§5 是实跑前的设计与预期，§11 才是观测。预期负例 ≠ 验收通过；不以首红 NEG 当 PASS；不改 bump / version / Draft / freeze / 验收措辞。

## 0. 固定身份

- tip：`7b07b72343907d06f797340ae26585f14174a377`（origin/main）。
- `exec/c/chain.sh` 的 git blob：`82956486c71734e400ed700d65aab8c89c225d5b`，最后一次改动为 `848bc986`（K5-1i），此后未变。
- 工作文件 sha256：`5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0`（与 gatedeps 五键新值一致）。
- 在 tip 之外的任何实跑，须在日志里另记 tip 与工作树 `git status` 结果。

## 1. 要证明的命题

在 `exec/c/chain.sh` 的 E3（parse2 δ）步首次非零后，**其后的任何后续任务都不启动**：不跑 tbl、net、`run --check-net`、探针 `run`、UA 参考编译，链以非零退出。

## 2. 夹具 / driver（具名）

| 名称 | 状态 | 覆盖 | 不覆盖 |
|---|---|---|---|
| `tests/seedppchaincheck.sh`（K5-1d） | 已有 | E2（pp）首红：NEG 断言“无 lex、无 Python 回退” | E3（parse2）首红后的 tbl/net/run/UA |
| `tests/seedparse2consumercheck.sh`（K5-1h） | 已有 | `prepare.sh` 的 parse2 首红：prepare 在 parse2 停，无 opt/prune/pp | chain.sh 本身；chain 的 tbl 循环 |
| `tests/chainfirstredcheck.sh`（**拟新增，未创建**） | 草案 | chain.sh 在 E3 步的首红，及其后零启动 | 进程组清空、TERM、UA 过 shim、真实 exec-chain 预算 |

拟新增夹具的形态（沿用 K5-1h/1d 的 shim 设计，不另起一套）：
- 场景树：`git archive` 自 tip 的 scratch 副本，不碰工作树。
- PATH 前置 shim：`sh`、`python3`、`cc`，每次调用记一行（类别、完整 argv、单调纳秒）到 `SHIMLOG`。
- 输入：单个小探针 `tests/c/a_char.c`，`CHAINSHARD=1/1 NETWORK=1`（0902 更正：NETWORK=0 会让 net/check-net 不可达，零计数不成立），`CHAINKEEP` 指向只含该探针的一行列表（0902 更正：空列表会被 chain.sh:66 拒绝）。
- 不跑 `exec-chain` 门，不设 `REFERENCE_KEYS`，不写 gatedeps。

## 3. 注入点

- 位置：`exec/c/chain.sh:55`，即 `b 60 env SEED_GEN_DIR=… sh "$R/exec/parse2gen/gen-delta.sh" "$T/e3.json" … || { echo "chain: E3 gen failed"; exit 1; }`。
- 方式（二选一，择一写明）：
  - (a) 已选用（0902）：shim 拦截 `sh` 调用，argv[2] 为 `exec/parse2gen/gen-delta.sh` 的调用时不运行 helper，写入诊断到 stderr，返回 rc=3，且不写 `e3.json`。只绑定 E3 的 argv，不会先在 E2 红。
  - (b) 让 helper 真实运行，但 `SEED_GEN_BIN` 指向一个坏二进制（沿用 K5-1h 的 `NEG_PARSE2_BIN`）。这样 helper 自身的失败路径也被覆盖。
- 注入只改 scratch 副本的运行环境，不改 `chain.sh` 源文件。

## 4. 首个非零子 rc（预期）

- 首个非零子进程：E3 步的 helper（或 shim 返回的 3）。
- chain 归一化为 `exit 1`，`chain: E3 gen failed` 写到 **stdout**（`chain.sh:55` 的 echo；0902 更正，原稿写成 stderr 错误）。
- 预期 `e3-gen.err` 非空（helper 诊断保留于 `$T`，随 EXIT trap 删除前须被夹具快照）。

## 5. 调用序（按源码推导；实跑前为预期，不是观测）

| 序 | 行 | 调用 | 预期 |
|---|---|---|---|
| 1 | 48 | `cc … exec/c/run.c` | 启动 |
| 2 | 51 | `sh exec/pp/gen-delta.sh`（E2） | 启动，rc 0 |
| 3 | 52 | `python3 exec/build/gen.py lex`（E1） | 启动，rc 0 |
| 4 | 55 | `sh exec/parse2gen/gen-delta.sh`（E3） | **启动，首个非零** |
| — | 58–68 | `python3 exec/c/tbl.py` ×3、`net.py`、`run --check-net` | **预期 0 次** |
| — | 约 70–90 | UA 参考编译、探针 `run`（e2/e1/e3） | **预期 0 次** |

## 6. 首红后启动计数

- 计数定义：SHIMLOG 中、位于 `helper-parse2-rc` 非零行之后的行数。六类为 tbl / net / check-net / probe-run / ua-ref / gen（python 的 gen.py），另计所有 shim 事件总数。
- 通过条件（仅指这一切口的预期负例成立）：六类后续计数全部为 0、所有事件总数为 0，且 chain rc 为 1，**stdout** 含 `chain: E3 gen failed`。
- r3 观测（§11）：六类与事件总数全为 0；chain rc 1；stdout 为 `chain: E3 gen failed`；首个非零子 rc 为 E3 的 3。与本节一致。
- 通过条件**不**包括：PGID/SID 清空、TERM 行为、UA 过 shim、exec-chain 预算、净提速。

## 7. 日志原件

- 缺口（已确认）：K5-1i run1 的 `receipt-run1.md` / `push.log` 原在 `/tmp/cc40-prep/k5-1i/`，该目录**现已不存在**（`ls` 确认）。0804 真审引用的“run1 受控证 pp0→parse2 红 3/2、chain1 与 shim 停点”因此无原件可引；不得以旧路径或转述代替。
- 拟办：夹具运行时用 `EVIDENCE=<新建空目录>`（模仿 `seedpprun.sh` 的 SEEDPPRUN_EVIDENCE：写 SHIMLOG、rc、stderr、最终 chain 输出的 sha256），在 scratch 删除前落盘；落盘目录须在仓外 `/tmp/cc40-prep/first-red/`，入仓时再以副本形式进 `research/c40-stamp-B-guard-k5-1i/`，并记副本与原件的 sha256。

## 8. 与 0804 真审限定的关系

- 继承限定（不因本材料消失）：真实 exec-chain 门预算未测；净提速未证；UA 未过 shim；ps-g 不是严格 PGID/树清空；TERM 未测；post HEAD/status 为事后采样（补证需前后身份）；枚举失败记 UNKNOWN，不作“组空”。
- `bound.c` 对子进程自身 setsid，shim 只看见 exec 进来的命令，不看见 bound 之下的进程组；本切口只声明“命令层面未启动”，不声明“进程层面无残留”。

## 9. 本窗未做

- 未创建 `tests/chainfirstredcheck.sh`；未执行任何 shim；未改 chain.sh、gatedeps、prd、version；未 bump、未 Draft、未 freeze、未跑门。
- 未入仓（本页仅落盘于工作树，未 commit / push）。

## 10. 下一步（需另授）

1. 授权实写 `tests/chainfirstredcheck.sh` 草案并在 scratch 实跑一次（正例 + E3 红 NEG 两个 case）。
2. 实跑结果与日志原件按 §7 落到仓外，再经 cdx 只读核验缺口是否闭合。
3. 若一轮未闭合，按 0857 顺延，续同一切口；不自动开 (B)。

## 谁裁

- 实跑、入仓、是否把“首红后零启动”记为已闭合：机房主任另裁。
- 本页不替代验收，也不构成 Draft 或 freeze 的前置完成声明。

## 11. 0902 实跑结果（夹具 `tests/chainfirstredcheck.sh`，tip 7b07b723）

- 夹具：`tests/chainfirstredcheck.sh [pos|neg]`；shim 在 PATH 前置（sh/python3/cc），UA 与探针 `run` 为日志包装；scratch = `git archive` tip，工作树需与 tip 在归档路径上一致。
- 正式记录：`first-red-run-0902/`（r3，全部断言通过，exit 0；SHA256SUMS 与副本一致）。
- 实跑历史：r1（旧夹具 NETWORK=0，POS 通过）；r2（NEG 全过，POS 的 "chain tbl" 正则写错，实际输出为 "chain net"，判 FAIL，属夹具误判，scratch 保留）；r3 修正正则后全绿。
- POS：chain rc 0，`chain net files 1 equal 1`；E3 helper rc 0；e3.json 产出；tbl=3、net=3、check-net=3、probe-run=3、ua-ref=1，证明日志能看见这些后续步骤。
- NEG：chain rc 1，stdout `chain: E3 gen failed`；首个非零子 rc = `helper-parse2-rc=3`（E3）；e3.json 不存在；首红（shimlog 第 17 行，即最后一行）之后 tbl/net/check-net/probe-run/ua-ref/gen 六类计数均为 0，所有 shim 事件计数也为 0。
- 预期负例 ≠ PASS：NEG 的零启动只证明命令层面没有后续启动。未证明：进程组/会话清空、TERM、UA 自身行为（仅包装计数）、exec-chain 门预算、净提速、真实 exec-chain 门绿。
- 原件位置：`/tmp/cc40-prep/first-red/r3/`（仓外）；本目录为副本。
- 回执：`first-red-receipt-cc-0902.md`。
