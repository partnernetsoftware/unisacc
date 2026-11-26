# unisacc 0.1.3 已知缺口、未跑范围与事故披露(known-gaps)

版本: `0.1.3`(`REV=f485ec9a2bf02a7204b0d40b4b048ebd8a652a7a`)
候选 APE SHA-256: `eb6741afe2d54c4a7e867b229aea903f1650a797508623869f858c0b197d3575`(`9232894` 字节)
`SOURCES_SID`: `15305fd30620e1953b7479dfa3cc8e88f48278e6b0d9a2c1ff85a687627fc6e5`(链 helper 从 frozen-root 实算)

> 数字取自机器原行:整链账本、`merged/strict-final.tsv`、`strict-W*.tsv`、`priority.tsv`、tape 计划原行;sha 见 `INDEX.tsv` 与 `evidence-rows.tsv`。

## 1. A3-4 外部权重旁置的真实状态(C0f 前后)

- C0f 前(基线 a0c4fc828 及更早):只有文件名为 `unisacc.com` 的 APE 进入旁置分支,且真实 APE 走不到公开面(名为 `unisacc.com` 时旁置取解包切片的私有缓存目录 `~/.cache/unisacc`);其它文件名强制禁用;显式 `--nn-dir` 不受文件名限制。依据:`cc/gk-c0f.md` B1 段与探针原行。
- C0f 后(`37f197cbf`):内嵌启动头同时导出 `UNISA_CONTAINER` 与 `UNISACC_APE_SELF`,配对相等才启用默认旁置;默认旁置先开后核(父目录与 `nn` 各 `openat`/`fstat`,属主为 root 或 euid 且无组/他人写位,`nn` 为符号链接 ELOOP 拒绝);显式 `--nn-dir` 与 `UNISACC_NN_DIR` 走旧路径;未覆盖的启动头(`tools/release-ape.cx`、`tools/dev-product.cx`、`ujs/product/ape-pack.cx`)仍强制禁用,三种形态有意拿不到默认旁置;裸入口回退分支仍用旧规则(记 N2)。
- 未跑范围:
  - E2E 全矩阵(`check-nn-external.cx --e2e`,约 30 条 APE 腿)本版无正式结果:`E2E_LAYER status=UNMEASURED`。依据:`cc/gk-c0f-r2.md` K1。
  - Darwin/Windows 与其它交叉目标的外部权重旁置全部未跑,记 `UNCOVERED`;不得以 Linux 结果顶替。
  - 属主判断、`nn` 为符号链接/非目录、父目录可写且 `nn` 缺席等腿仍无用例;记 `UNMEASURED`。
- 已知风险:N1 本地拒绝服务(父目录不合格且 `nn` 存在时整次编译 rc 1;绕行用显式 `--nn-dir` 或清除 `nn`);S4 候选批次污染(链外候选切片若继承相等变量会取发起者目录的 `nn`;候选批次须显式 `UNISACC_NN_MODE=builtin`,列 v0.1.4 S4)。

## 2. 整链与预跑的返工(机器抽取)

来源:`~/.local/share/unisacc/research/cc/chain-ledger-0.1.3.txt`(15 行)与 `agyneon/v014/retro-v013.md`。账本计数:链启动 5 次、预跑启动 4 次、resume 3 次;返工序列为链 #1-#4 与预跑 #1-#3,链 #5 与预跑 #4 是 C0f 冻结后的正式轮。

| 轮次 | 起止(账本原行) | 返工原因 | 量化 |
|---|---|---|---|
| 链 #1 | `05:20:46` 起,`REV=fd57ec1f` | `whole-front.cx.in`/`whole-win-span.cx.in` 未在 `cxn_boot+cxn_seal` 前封存,`osx front0` 才暴露 | 至 `05:46:01` 重起,约 25.3 分钟 |
| 链 #2 | `05:46:01` 起,`REV=bfdd1448ba` | 修复 worker 模板封存后重起 | 并入上项 |
| 预跑 #1 | `06:16:56` 起,`PDIR=v0.1.3-bfdd1448ba`,`timeout=7200s` | 三连红:`06:42:56` `equiv3`、`07:23:35` `equiv1`(5 DIFF)、`07:32:04` `equiv1` 二次;CXIF mtime 续龄类 | `C0c=d07d9640c` 于 `07:38:24` 落地,约 81.5 分钟 |
| 链 #3 | `07:46:15` 起,`REV=d07d9640ca`(C0c) | C0c 后重起 | 至 `09:24:13` C0d/C0e 落地,约 98.0 分钟(含预跑 #2) |
| 预跑 #2 | `08:21:43` 起,`PDIR=v0.1.3-d07d9640ca` | C0d/C0e 落地,`ident-helper` 目录消失后重建 | 并入上项 |
| 链 #4 | `09:31:19` 起,`REV=a0c4fc8285`(C0e) | C0d/C0e 后重起 | 并入上项 |
| 预跑 #3 | `10:00:18` 起,`PDIR=v0.1.3-a0c4fc8285` | 承接链 #4 | 旧目录后标 bak-pre-C0f |
| 链 #5 | `12:09:31` 起,`REV=f485ec9a2b`(C0f-freeze) | C0f 落库后正式轮,非返工 | 账本原行:`done rc=0 (APE eb6741af, SID 15305fd3)` |
| 预跑 #4 | `12:46:20` 起,`PDIR=v0.1.3-f485ec9a2b` | 正式轮 | 见第 3 节 |

- `retro-v013.md` 合计:返工 1-3 可量化墙钟 `25.3 + 81.5 + 98.0 = 204.8` 分钟(约 3.4 小时)。
- 归档覆盖:现存链目录为 `fd57ec1f`、`a0c4fc8285`(`bg-b1.bak-pre-C0f` 内)、`f485ec9a2b`;`bfdd1448ba` 与 `d07d9640ca` 目录与 `bg-b1.bak-pre-C0c` 均不存在,只留 `chains/snap/*.partial.lock` 残锁。缺链以 `chains/ABSENT.md` 如实记录,不以账本行冒充原件。

## 3. 预跑补跑窗口(冷态贴限与真实经过)

机器取证:预跑 #4 目录 `~/.cache/unisacc/prerun/v0.1.3-f485ec9a2b` 共 298 个 `strict-W*.tsv`(292 个原窗 + 6 个补跑窗),已全部归档;`merged/strict-final.tsv` 折叠前有 554 条批行、549 个唯一批;按批折叠后 `549/549 PASS`,与合并原行一致(`PRERUN_MERGE windows=298 batches=549 PASS=549 FAIL=0 UNKNOWN=0 SUPERSEDED=1`)。`strict.tsv` 按原样保留 4 条非 PASS 原始行(`slow-split` FAIL 55616 / `tsv-lint-root` UNKNOWN 55596 / `slow-split` UNKNOWN 55512 / `slow-split` FAIL 55635),折叠后由补跑窗覆盖,`SUPERSEDED=1` 即 `slow-split` 的 FAIL->PASS 一条。

| 批 | 窗口文件 | 机器原值 | 处置与经过 |
|---|---|---|---|
| `slow-split` | 原窗 `strict-W0089.tsv` | `FAIL`, `ms=55616`, `load1=6.53`, `transport=-1`, `assertions=UNMEASURED` | 严格 lane 新布局、run-cache 全冷;检查内四组串行且组限与批窗同为 55000ms,只能被外窗杀。按"同布局预热一次丢弃、不 rm 不覆盖"补跑 |
| `slow-split` | `strict-W9904-slow-split.tsv` | 首行 `UNKNOWN`, `ms=55512`, `load1=3.93`, `transport=-1`;第二行 `FAIL`, `ms=55635`, `load1=4.14`, `retries=1` | 冷态仍贴限;保留两行原样,不删不覆盖;继续补跑 |
| `slow-split` | `strict-W9905-slow-split.tsv` | `PASS`, `ms=21690`, `load1=2.53`, `assertions=5` | 预热后通过;作为覆盖窗 |
| `slow-split` | `strict-W9906-slow-split.tsv` | `PASS`, `ms=20897`, `load1=5.76`, `assertions=5` | 第二次确认;原窗与 W9904 两行仍保留 |
| `tsv-lint-root` | 原窗 `strict-W0277.z05.tsv` | `UNKNOWN`, `ms=55596`, `load1=3.57`, `transport=-1` | 冷布局首跑未完成;补跑 `W9901` 得 `PASS`, `ms=11187`, `load1=4.42`, `assertions=2` |
| `bpidecl` | 携带行 `priority.tsv` 第 4-5 行 | `PASS`, `ms=38498`, `load1=2.35`, `candidate=e31c413a`(0.1.2 候选) | 非本候选证据;补跑 `W9902` 得本候选 `PASS`, `ms=2469`, `load1=4.18`, `assertions=14` |
| `lane-key-probes` | 携带行 `priority.tsv` 第 6-7 行 | `PASS`, `ms=50217`, `load1=1.96`, `candidate=e31c413a`(0.1.2 候选,距 55 秒约 4.8 秒) | 贴限且非本候选;补跑 `W9903` 得 `PASS`, `ms=20755`, `load1=3.92`, `assertions=5` |

- 补跑纪律:窗号唯一并校验不存在;先同布局预热一次丢弃再取 RAW;命令不 `rm`/`mv`/覆盖 strict 文件;补跑窗按批 ID 折叠、原行不删。
- 结论:四批均有本候选 `PASS` 窗;`slow-split` 两次冷态贴限(55512/55635)与 `tsv-lint-root`(55596)的当前判断是冷缓存为主因(原窗原行证明 `transport=-1`,同布局热态补跑 `PASS`),但原行本身不直接证明"冷缓存"这一原因,不写成已证明结论;未放宽任何时限。`slow-split` 的结构性修法(按组分片、共享总预算)列 v0.1.4。

## 4. tape M1 的真实边界

机器原行:`ompgpt/v014/tape-ab-implementation-plan.md` 第 101 行。

- 16 档:`94.037` 倍;64 档:`114.316` 倍;均为旧逐步入口证据,不是本候选 M1 结论;历史 r1 是重建存档,不是历史 g2 字节。
- 102 档:未完成,无完成倍率,只留真实终止原因与最后阶段步数下界。
- 15 倍门:未过;25/40/55 分级门与 40 倍运行中止损未在本版成立,tape 线按 `roadmap-tape-nn.md` 整体顺延。
- M1X 扩槽:未启动;M1 冻结八槽 `{0,1,2,3,4,5,6,8}`(掩码 383)未改,扩槽不得称为"满足八槽"。
- 状态:洁净且绑定身份的明确超时/步数耗尽为 `G1 FAIL`;已见 40 倍止损为 `STOP`;缺有效样本/观测/未启动为 `UNMEASURED`;本版按原行记 `UNMEASURED`,不写 PASS。

## 5. 延后到 v0.1.4 的缺口

- C3: S3 59 格(59A/59B、M1-M6)未完成;S1 `syp_reap` 的 `deadline_at_ms` 字段复用留 v0.1.4。
- `prerun-drive --rebase-rev --plan`(P01-P33)未落。
- devsvc 提交互斥 v5.3 余项(30 秒单片归一、b 片与真实 commit 证据)仍被复核阻断。
- CXIF 读探测续龄:本版以测试补偿绕过,根治留 v0.1.4。
- 对应 v0.1.4 计划编号见 `plans-v0.1.4-draft-v4.md` 的 1、2、3、10、11 项。

## 6. 证据缺口披露(本项目新增)

- 三份红停原件(`evidence-red-064216`/`072250`/`073114`)在归档前已随 `v0.1.3-bfdd1448ba.superseded-073824` 目录清理,`prerun/` 与 `prerun-backups/v013-before-W99-20261126T141445` 下均无匹配;原件缺失以 `incident-red/ABSENT.md` 记录,不以重建文件冒充。
- `bfdd1448ba` 与 `d07d9640ca` 两条整链目录不存在,仅有账本行与残锁;以 `chains/ABSENT.md` 记录。
- `frozen-root` 为约 218,969,889 字节的树,本次只归档 16059 个文件的路径与字节清单(`prerun-inputs/frozen-root.manifest.tsv`),不复制整树;需要逐字节核身份时以清单 + 链收据复核。

## 7. 证据丢失事件(2026-11-26 13:00:11 前后)

- 事实:三个目录在同一时刻消失,`chains` 与 `prerun` 两个父目录 mtime 均为 `2026-11-26 13:00:11` 本地时间:
  - `~/.cache/unisacc/chains/bg-b1.bak-pre-C0c`(含 `bfdd1448b`、`d07d9640c` 两条旧链);
  - `~/.cache/unisacc/prerun/v0.1.3-bfdd1448ba.superseded-073824`(含 `evidence-red-064216`、`072250`、`073114` 三份红停原件);
  - `~/.cache/unisacc/prerun/v0.1.3-d07d9640ca.superseded-092413`。
- 归因(与现有证据吻合):用户 journal 显示 `system-cleanup.service` 于 `13:00:03` 到 `13:00:13` 运行;`13:00:03` `before: 250G 190G 61G 76%`,`13:00:11` `/tmp and 2h caches cleaned` 并报 `unisacc stale artifacts cleaned`,`13:00:13` `after: 186G`;unit 的 `ExecStart` 指向 moltbaby 脚本,脚本第 60-67、79-80 行对 `~/.cache/unisacc` 的 `prerun` 与 `chains` 有同阶段递归清理规则(子目录自身 `atime` 与 `mtime` 均超 120 分钟且内部无 120 分钟内访问或改动条目即 `rm -rf`,排除 `.*`/`*.lock`/`*.sock`)。`journalctl --user -u system-cleanup.service` 无逐目录原行:逐目录原行不可得,据目录 mtime 与源码规则推定。
- 责任面:六窗询问中五窗已声明不是自己;ccogweco 亦明确不是自己,且在该时刻未对 `~/.cache/unisacc` 做任何删除或改名操作。归因对象是用户定时器服务,不是协作窗口。
- 事后处置:已加有界 keepalive `~/.local/share/unisacc/keepalive/keepalive-0.1.3.sh`(每 25 分钟 touch,最多 12 轮),台账 `15:15:34` 已记。
- 影响:三份红停原件与两条旧链记录不再可直接核验;原件的 sha 从未记录,不能声称其内容已核。
- 剩余证据:台账当前保护库 sha `5c3482f8…`(2263 字节,含 keepalive 一条)与早先摘录 sha `5b05efde…`(2066 字节,差异即 keepalive 一行)、最终正式轮监督输出 `prerun4.out`(sha `f6eaf3eb…`,`PRERUN_STOP` 行数为 0,不能重建三次红停的 `raw=` 行)、保护库 `~/.cache/unisacc/evidence-vault-0.1.3/`;归档侧以 `incident-red/ABSENT.md` 与三份 `reconstruct-*` 副本明确标注非原件。
- 处置:原件不重建、不冒充;后续证据保留按 `evidence-vault-0.1.3` 只读保管,删除须书面授权。
