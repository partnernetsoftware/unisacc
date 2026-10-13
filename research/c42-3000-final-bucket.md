# c42 定点闸 /tmp/c42-3000 FINAL 分桶（2026-10-13 SGT，只读分桶）

性质：分桶记账，不是验收通过。acceptance FAILED（final rc=1），release NOT ready。不改验收措辞。

## 1. 运行身份

| 项 | 值 |
|---|---|
| GATE | `/tmp/c42-3000/cand.queue`（fresh start；未 resume；env-pin `3dffcca1`） |
| 候选 | `85cc9b33c2154df9323b4deba4659a2aa7e55c69f0eaf5a70ad2479ed47ee3f8` |
| 候选源提交 | `cf7f594a`（build.json `commit`；**不是** 当前 HEAD，见第 8 节） |
| H1 | INHERIT（`archive/plans/v0.0.38.md`） |
| comboot | stage1、stage2、stage3、fixedpoint 全部 rc=0；fixed point 成立（sha 同为 `85cc9b33…`） |
| 结证 | 721/721 completed；`95 failed, 6 unverified, 0 pending`；final rc=1 |
| 参照 | `/tmp/c42-2000`（`93ec722e…`，721/721，final rc=1）；`/tmp/c42-1640`（`e221516a…`，史证） |

## 2. exittable 总表

命令：`python3 -I release/tools/exittable.py /tmp/c42-3000/cand.queue --h1 archive/plans/v0.0.38.md`（只读，rc=0）

| 桶 | 数量 | 记账 |
|---|---:|---|
| PASS | 620 | rc=0 |
| BLOCKED | 27 | 前驱失败，未执行；见第 6 节 |
| HOST_TIMEOUT | 14 | rc=142，超时账 |
| NEEDS_RULING | 14 | 逐项见第 4 节 |
| RESOURCE_UNKNOWN | 7 | rc=2，缺内存证据 |
| RULED_BASELINE | 33 | 宿主基线记录，不计 PASS |
| UNVERIFIED_HOST | 6 | rc=77，宿主不适用，覆盖义务保留 |
| 合计 | 721 | 620+27+14+14+7+33+6 = 721 ✓ |

## 3. 真红栏

**无**（按 cc 指示的记账口径）。

真红三项全部 rc=0（来自 `release-queue.log` 的 DONE 行）：
- `lib-lifecycle` rc=0，8.29s（`concurrent_contexts 4`，`guest_pool_released true`）。
- `com-multi` rc=0，`multi-unit ok`。
- `com-forward-multi` rc=0，`forwardmulti ok 3 wrong 0`。

注：“真红栏无”**不等于**无需裁定。第 4 节 14 项 NEEDS_RULING 里有 `seedmemory`、`exec-native-stages`、`freezecheck`、`ledgercheck` 等 rc=1，未被归入真红，但也未被裁定，验收前必须分诊。

## 4. NEEDS_RULING 逐项（14）

| # | 套件 | rc | exittable 摘要 | 初步判断（只读） |
|---:|---|---:|---|---|
| 1 | com-elfobj | 1 | objects 14, relocations 1603, linked 14, skipped 3 | 待裁；疑似宿主 ld 能力面 |
| 2 | elfobj | 1 | 同上 | 待裁；与 #1 同源 |
| 3 | exec-native-stages | 1 | neg ok 4, fail 1 | 待裁；单个负例失败 |
| 4 | freezecheck | 1 | closure commits without fix: 4 | 待裁；2000 时为 3，本拍为 4，需要看是哪一条 |
| 5 | ledgercheck | 1 | carry into v0.0.42.md mismatches 1 | 待裁；与 `plans/v0.0.42.md` 账目有关 |
| 6 | observation | 1 | queue.sh on window rc 65: exit 2, launches | 元套件，观察面；非产品 |
| 7 | queuestart-launch | 1 | another queue.sh running (pid 2411881) | 元套件，撞车；非产品 |
| 8 | queuetimeout | 1 | `$2` unbound variable（`tests/queuetimeoutcheck.sh:35`） | 测试脚本缺陷；非产品 |
| 9 | queuetimer | 1 | queue rc=2, want 142 from inner watchdog（撞车同源） | 元套件，撞车；非产品 |
| 10 | seed-construct-parse2-20 | 1 | subprocess.TimeoutExpired | 超时类，待裁是否计超时账 |
| 11 | seed-construct-parse2-21 | 1 | subprocess.TimeoutExpired | 同上 |
| 12 | seed-construct-parse2-22 | 1 | subprocess.TimeoutExpired | 同上 |
| 13 | seedgen-ppconsumer-1 | 142 | NEG-badsg prepare stops rc 2 before lower；无 Python fallback | 负例夹具停止记录；rc142 但不计超时 |
| 14 | seedmemory | 1 | FAILED (failures=2) | 待裁；真需要分诊的一项 |

NEEDS_RULING 14 项要**一次性**送一份裁定请求（exittable 的要求）。本文件不做裁定。

## 5. RESOURCE_UNKNOWN（7，rc=2）与 UNVERIFIED（6，rc=77）

- RESOURCE_UNKNOWN（rc=2）：`com-seedgen`、`seedgen`、`seedgen-2`、`seedgen-3`、`seedgen-4`、`seedparse2-1`、`seedparse2-2`。缺内存证据（required=memory, observed=unknown）。需单 owner 单项测量。
- UNVERIFIED_HOST（rc=77）：`exec-bootstrap-osxarm`、`exec-bootstrap-osxx86`、`lib-stack-arm`、`lib-stack-x86`、`lib-stack-x86-hostabi`、`lib-windows-gp-native`。宿主 Linux/x86_64 不具备 darwin/win 执行宿主；覆盖义务保留，**不是 PASS**。

## 6. BLOCKED（27）与前驱根

| 前驱根 | 被阻塞数 | 成员 |
|---|---:|---|
| `rowcov-parse2-build`（rc=142） | 25 | `rowcov-parse2-1` … `rowcov-parse2-24`（24）+ `rowcov-parse2-union`（1） |
| `rowcov-enc-1`（rc=142） | 1 | `rowcov-enc-union` |
| `com-luatests-build`（rc=142） | 1 | `com-luatests` |

来源：`release-queue.log` 中 `predecessor … failed` 的 DONE 行，0.00s，未执行。计数与 exittable 的 `[+25 not run downstream]` 一致。

## 7. 超时账（HOST_TIMEOUT 14 + RULED_BASELINE 中的 rc142）

- HOST_TIMEOUT（rc=142，14）：`closure-c1`、`closure-c2`、`closure-c3`、`closure-c4`、`exec-e5`、`exec-f1-attributes`、`exec-macself`、`exec-tableself-2`、`lib-callable-variadic-model`、`shared-e2-plain`、`stages-1`、`stages-2`、`stages-3`、`tools-2`。
- RULED_BASELINE 中 rc=142 的有：`com-fb12-multi`、`com-luatests-build`、`com-tapebin-1..3`、`exec-arm`、`exec-armself-package`、`exec-e4self`、`exec-macxself`、`exec-selfelf`、`exec-winself`、`rowcov-enc-1..8`、`rowcov-parse2-build`、`rowcov-pp`、`rowcov-pp-locations`、`rowcov-pp-shared`、`shared-e2-located`。
- 超时记账，不当产品红；limit 未放宽。

## 8. RULED_BASELINE（33，rc=1 或 142，宿主基线留档）

`ccinterop`、`com-forward`、`forward`（skip 即失败记账）、`exec-bridge-linux`、`exec-driver-language-2`、`hosthdr`、`realprog`、`syscall6`、`windows-resolver-host`（缺 Windows SDK 交叉编译器）、以及上面第 7 节列出的 rc142 项。全部“never counted PASS”。

## 9. 边界与注记（重要）

1. **候选树与当前 HEAD 不一致。** 候选源提交是 `cf7f594a`，当前本地 `main` 是 `8f0466a8`。二者的产品路径差异：`exec/c/run.c` 有 7 行（`cf7f594a` 缺 `3206af4c` 的 readstream read-dir 修复）。因此 **721 闸没有覆盖 `3206af4c`**。
2. `cf7f594a` 不在任何本地或远端分支上（`git branch -a --contains` 为空），而是与 main 上的 A+C 修复（`ef90a8a3`）同题但不同 SHA 的副本（`4ada2b91`）。两条线的共同祖先是 `018b4509`。这说明候选是在另一条基线上构建的。
3. 本文件只在本地新建，**未提交、未推送**。
4. 禁止项遵守：未 resume 旧壳，未 seal，未碰 Latest，未签名，未公开，未 Draft。Draft 仍缓（acceptance 未过）。
5. lib-lifecycle 的绿来自候选树。若要把绿带到 HEAD，需要用 HEAD 重新做候选与闸，或者证明 `3206af4c` 对 lib-lifecycle 与 read-dir 相关套件无影响。本文件不做这个判断。
