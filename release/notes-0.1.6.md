# unisacc 0.1.6 发布说明（冻结前草稿）

状态:已公开(2026-11-27,tag v0.1.6 指向 c9275e7756,APE ed50199f,九资产)。CI 门六目标全部成功(CI 门判定 QUALIFIED);整体裁决仍为 `NOT_QUALIFIED`,因为全套 s9 本版未跑(见 release/decision.txt)。严格证据 593/593 PASS,其中 5 批为补跑。主题为 tape 载体公开分发;项 1 已由独立复核关闭。

## 真实身份

| 对象 | 身份 |
| --- | --- |
| 冻结提交 | 待填 |
| 发布 APE (`unisacc.com`) | 候选 APE sha256 待填 |
| CI run | 待填 |
| 链阶段数 / 各目标裁决 | 待填 |
| 链目录 | 待填 |
| 预跑目录 | 待填 |
| 上一公开版 | 0.1.5（tag `v0.1.5`, APE `824e3f05...`, Y2 `ee829606c`） |

## 产品改动

(1) **主题：tape 载体公开分发（选定九资产）**：在 0.1.5 六资产（五件被校验文件加 `SHA256SUMS`）之上，公开侧只面向 `lnx/x86_64` 增加 GitHub 顶层文件 `tapemode.so`、`tape-manifest.tsv`、`m53.tapeslice`，合计九件；安装目录内对应 `tape/tapemode.so`、`tape/manifest.tsv`、`tape/m53.tapeslice`。正式上传清单与回读未完成前，公开分发仍记 `UNMEASURED`。
(2) **GV.compat 产品补丁**：v0.1.6 分支 `a1f3b8d00`（合入 `be9902be0`）把 `GV.compat.base` 收窄到函数指针区间 `[FPS_FIRST, SBB)`，新增 7 个多单元回归。公开 0.1.5 根 `--controller-canary` 返回 `root-controller-canary-failed`；该文案也覆盖临时目录等其他原因，不能单独作为 GV.compat 定因。正式候选是否过门 `UNMEASURED`。
(3) **M1X-A slot-12**：`d68f73d0f`（源 `9861a04ce`/`fe0858c1d`）；raw syscall 首操作数、精确元数与坏操作数拒绝，负控覆盖。正式候选运行项 `UNMEASURED`。
(4) **解释器 A1**：`d07a46b7b`；私有 Linux x86_64 热态 ns/step 微基准，`85d271c2d` 对 `d07a46b7b`，9 次，3.553x arith / 3.996x cmp-branch / 1.539x mem-call，load1 前后 2.66，IQR 最大 3.10%。正式候选的 G1/G2/G3 均 `UNMEASURED`。
(5) **门 19 安全子集**：`78db58587`；生产入口移除测试替身、单一门名花名册与漂移检查、6-affected 失败闭合与 `-z` 解析。剩余项列 v0.1.7 `UNCOVERED`。
(6) **ident-helper 固定**：`2d5849564`；`chain-ready --prepare-helper` 写 `.no-purge` 且写失败即拒，`release-keepalive` 覆盖 ident-helper。
(7) **起链前卫生门**：作者 `a01396be7`，main 合入 `85c524785`；新增 `25-tools-compile-shards`，并加固已有的 `10-envhygiene` 与 `20-ident-helper`。正式门输出与阈值以候选回执为准。
(8) **事故驱动的运行器合同**：runner 必须使用干净环境并显式提供所需材料变量；`TMPDIR` 必须在任何 git 仓之外；git 探测失败必须失败闭合；预跑分片必须与登记表齐平；超时重试要保留原始失败行。以上是操作合同，不是已完成的形式验收。

### 回退块：六资产安全子集（仅当私下稿读回连续失败两次时启用）

- 资产沿用 0.1.5 布局：五件被校验文件加 `SHA256SUMS` 共六件；不随本方案分发 `tapemode.so`、`tape-manifest.tsv`、`m53.tapeslice`，公开实际清单待回执。
- 选择本回退块时必须同时撤回九资产相关发布工具默认与安装同步；未选择前主文仍为九资产。

## 开发树证据

- GV.compat phase 4a：`global-redecl 25/25`；0.1.3 根对照 23/2（`w-agyneon2/.work/report-v016-post-thaw-verification-master-order.md:9`）。这是开发编译器检查，不等于正式候选。
- slot-12：公开 0.1.5 根上 check-tapepack 负控 21/0，开发补丁夹具结果，不等于发布候选。
- 门 19 与 ident-helper：公开 0.1.5 根上 `check-chain-ready core 129/0`，属开发检查。
- 卫生门：作者工作树 `25-tools-compile-shards PASS shards=202 sources=1610 batches=202/202`（`w-agy-flow/.work/report-v016-item6-hygiene-gates.md:45`）；正式门输出用占位。
- 上述计数只作开发树证据，不进入正式资格。

## 严格证据

预跑与合并实测(候选 c9275e7756,APE ed50199f;首次正式预跑在 acad02543 上于 W0080 红停,原因与修复见下节):

```
PRERUN_RUN	status=DONE	ran=294	skipped=0	window_wall_ms=7135164	sum_batch_ms=9678769
PRERUN_MERGE	windows=309	batches=593	PASS=593	FAIL=0	UNKNOWN=0	SUPERSEDED=4	out=strict-final.tsv
SUMMARY	batches=593	PASS=593	FAIL=0	UNKNOWN=0	INVALID=0
RATCHETS	ok=20	fail=0
DERIVE_SUMMARY	UNMEASURED
```

首轮非 PASS 共 5 批,全部单独补跑为 PASS,原证据保留,补跑写入新文件 strict-X9001..X9006 并按文件名后者胜出合并:chain-ready-launch、chain-ready-launch-b、freeze-cut(首轮 FAIL:预跑环境继承 UNISACC_BOOTER_CHAIN_OUT,未在 build/chain-env.tsv 登记);slow-split、tsv-lint-root(首轮 UNKNOWN 与补跑前两次:55 秒窗口超时,运行缓存键含根身份 rcident.cx:608,共享根换为候选后工具全部冷编译;预热后 35.3 秒与 11.4 秒 PASS)。正式 strict 路径 `release/evidence/v0.1.6-strict-20261127.tsv`;严格证据以外批次 `UNMEASURED`;全套 s9 本版未跑。

## 已知缺口与未验证范围

(1) 发布执行 7 步全部完成:CI run 45938522235 六目标成功,私有草稿读回 8 件与 SHA256SUMS 一致,公开读回通过,共享根已升级为 0.1.6 并通过 root-current 与 --controller-canary;全套 s9 未跑,整体裁决 `NOT_QUALIFIED`。
(1a) 冻结两次:首次冻结 acad02543 的预跑在 W0080 红停(check-gen-all-check:GV.compat 补丁 a1f3b8d00 手改生成表 exec/facts/k2-gen2.tsv,facts-gen --check 报 DIFF);修复 84737d935 改生成器并重生 37 张表(仅表头输入行),重冻结 C0a c35123e67、C0b c9275e775,整链重跑,APE 字节与首链相同。
(1b) 打包首跑被 release-run 拒绝(tape-sidecar-receipt-or-manifest-mismatch):打包检查与 release-ape 收据行格式不一致,九资产路径此前未端到端实跑;已在 tools/release-run.cx 修检查一侧,回归用例待登记。
(2) 九资产 sidecar 仅计划 `lnx/x86_64`；八条清单与顶层文件名须以正式资产单回读为准。六资产只是失败两次后的回退块。
(3) GV.compat 修在分支，公开 0.1.5 根 canary 为 RED；正式候选过门与 phase 4a 之外用例 `UNMEASURED`。
(4) A1 数字只在私有微基准成立；slot-12、门 19 剩余项、ident-helper、卫生门的正式运行项 `UNMEASURED`。
(5) 配对改名中断恢复、product-map 读集、G2/G3、全套 s9 仍为缺口。

## 回退办法

- 发布回退：转草稿并恢复上一版 `v0.1.5` 为 Latest；保留已有标签与发布记录，不改写历史。
- 安装回退：先以备份 manifest 校验 `v0.1.5` APE 与 build record 摘要；不匹配时下载并校验 `v0.1.5` APE 后安装，实查 `unisacc 0.1.5` 与两项摘要。九资产方案还须清理本机 sidecar 并按 `release/rollback.txt` 验证；未执行前记 `UNMEASURED`。
- `UNISACC_M53_S1` 默认关，清除显式开关即回原生路径；源码撤回只列方案，未经验证。
