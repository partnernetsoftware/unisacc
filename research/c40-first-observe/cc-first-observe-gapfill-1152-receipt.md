# cc 首观察补证短记（1152；只补现有采样可落盘项；不重跑、不续构造、不宣称产品 PASS）

范围 PASS（rc97 垫片截停）保持；本记不改变其结论。原回执：`cc-first-observe-1147-receipt.md`；状态目录 `/tmp/cc40-first-observe-1147`。

## 1. 逐步 rc 与哈希（已有原件）
| 项 | 原件 | 值 |
|---|---|---|
| UA 同源 reference（build_ref） | `/tmp/cc40-first-observe-1147/ua`；`ua-build.log`（空，rc 0 由 1147 回执记） | sha256 `6a05c3c628961d025a884efd4170639bc98980d51209a3176c19b7cc4d1887ac`（本次 sha 复算，与 1140 的 6a5c3c8e 树 UA 同值） |
| h1source（前） | `h1-before.out` | rc 0；INHERIT archive/plans/v0.0.38.md |
| h1source（后） | `h1-after.out` | rc 0；同上 |
| archive/plans/v0.0.38.md sha256 | 1147 回执（前后同命令） | `d1f05f7d…`（前后相同） |
| build_candidate 的 h1source 步 | `build-candidate.log` | rc 0 |
| shared 步 `make model-com MODEL_STEP=shared` | `cand/step-shared.log`（垫片输出一行） | **rc 97**（垫片 exit 97，预期截停） |
| build_candidate 退出 | `build-candidate.log` | bc_rc 97；墙钟 11:49:00 |
| 唯一 make 调用记录 | `make-calls.jsonl`（1 行，argv `model-com MODEL_DIR=/tmp/cc40-first-observe-1147/cand MODEL_STEP=shared`） | 1 行 |

## 2. 时点语义（纠正 1147 回执的"未另记"）
- `timing.txt`：t0 = monotonic_ns **11359944995521**（build_candidate 启动前采）。
- `make-calls.jsonl`：t_make = monotonic_ns **11360011661411**（垫片内采，首个 make 进入时）。
- 可复算：t_make − t0 = **66 665 890 ns（约 66.67 ms）**，即 build_candidate 启动到首 make 的墙钟间隔（含 h1source 与 bound 启动）。
- t_end：**UNKNOWN**。1147 未在退出后采 monotonic；build-candidate.log 只有墙钟 11:49:00（非 monotonic），不能换算。
- 墙钟与 monotonic 不混比（doc 规则）。

## 3. 残留/清理（不升格为 PGID/TERM 证明）
- 1147 当时的 sid 筛查（sid 194489）结果：无残留；**该 ps 原表未落盘**（只在会话内出现），故按规则记为：枚举成功、结果为空，原表 **UNKNOWN（未保存）**。
- 本补证时点的全进程快照：`/tmp/cc40-first-observe-1147/gapfill-ps-now.txt`（1152 采，**事后时点**，非首 make 当时）；按 sid 194489 与 build_candidate/bound/make 字样筛：**0 行**。
- 限定：PGID/SID 清空、TERM 行为、bound 子进程自 setsid 后的组内残留 **未证明**。

## 4. 身份采样（补 1147 漏的 ls-remote 时点）
- 1147 前后身份文件 `identity-before.txt`（含 ls-remote）、`identity-after.txt`（**无 ls-remote 行**，1147 回执已注明）。
- 本补证时点：HEAD `10c2e9b768f422f8646e930aace687321f528d4f`；ls-remote `10c2e9b768f422f8646e930aace687321f528d4f`；二者一致；**事后时点**，非首 make 当时。

## 5. UNKNOWN 清单（具名）
1. t_end（退出后 monotonic）— 无原采样。
2. 1147 的残留 ps 原表 — 未保存。
3. identity-after 的 ls-remote — 1147 未采（本补证仅记事后时点）。
4. 失效键全集、seed 内存键、exittable --h1 — 1147 未采，本补证不补。
5. 真 make 行为 — 本机无系统 make，垫片替代，未验证。

## 6. 未做 / 不授
- 未重跑首观察、未续 B.2、未 Draft、未公开、未 seal、未 bump、未改 version/gatedeps/.cx、未 sudo 装 make、未全量 queue。
- 本记不把 rc97 或补证写成产品 PASS。
