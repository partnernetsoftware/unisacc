# 0.0.38 冻结身份回执（模板，逐项实测后填写；未填 = 未执行，不预写 PASS）

授权：董秘代裁 0.0.38 版本冻结可开（2026-10-10，授权到达时刻 09:57 +0800）。顺序：§24 生成器全矩阵预验 → 版本号提交 → seed 内存证据一次采 → build_ref → build_candidate → comboot 定点 → queue 出口。同机重活串行，单令牌。

| 字段 | 值 | 来源 / 时刻 |
|---|---|---|
| 授权前 HEAD | b8ed07cf | git |
| seed/gen.c sha256 前缀 | b802e4f2d2d7f10a | 72120ea8 起 |
| source_digest（版本提交前） | c7e75006 | exec/c/provenance.py |
| §24 矩阵预验 owner（认领≠begin）/ 阶段 id / begin / end / 证据摘要 / 结论 | cdx / — / — / — / — / **阻塞**：无有效内存隔离（见下） | |
| 版本提交 / source_digest | — / — | |
| 内存证据键（C_SHA、REFERENCE_KEYS、cc 实体/版本/flags/并发、启动器 path/sha/target）/ 采证时刻 | — | tests/seedmemory.py；矩阵 RSS 只有在配置/路线与准入证据一致时才复用，不自动抵账 |
| 同源参考 UA 路径 / sha | — | build_ref.sh |
| 候选路径 / sha / sources_sha256 | — | build_candidate.sh |
| seed 对 / comboot stage2=stage3 定点 | — | comboot.py |
| queue state / 证据目录 / 最终 rc | — | ~/.unisacc/evidence/<run>-final（含 SHA256SUMS） |
| 出口表（exittable）PASS / RULED / H1 / H2 / UNKNOWN / 77 / NEEDS_RULING | — | release/tools/exittable.py |

出口表目标：除已裁 RULED/H1/H2/77 外无 NEEDS_RULING；RESOURCE_UNKNOWN 是未执行义务，不因“无待裁”自动放行，按既定口径处理；旧 SC1 三项不替代全矩阵；旧 740007ef 与 ver038 证据不签新闭包。

## 阻塞（2026-10-10 10:0x，cc 实测）

§24 只准在有效内存隔离内测峰，禁止 prlimit 兜底。本机：
- `systemd-run --user --scope -p MemoryMax=…` rc1：无 systemd user manager（/run/user/1000 不存在，user bus 连接失败）。
- 进程位于 cgroup `0::/agent`，无委派（mkdir 被拒）；有免密 sudo，但 `/sys/fs/cgroup/background` 的 `cgroup.subtree_control` 为空，子 cgroup 无 memory.max，root 也写不进。
- 可行路线均需改宿主 cgroup 配置（例如在某父级启用 memory 控制器后建专用子组），属宿主变更，待裁；未做任何改动，测试子组已删除。
- MemAvailable 约 4.3 GiB（cdx 观测）。
- 隔离 WAIT 时刻：发现 unknown（cdx 回执与 cc 实测无原始时间戳）；报裁 unknown（无原始事件时间戳，不编分钟）（给董秘三选一：改宿主 cgroup / 换有隔离宿主 / 改测峰口径）；恢复 —；采证 —（记录于 09:59 +0800，仍 WAIT）。
- 采证前宿主就绪出口（cdx2）：可验证的 memory 硬上限（memory.max + memory.peak）。MemAvailable、单令牌、ulimit、超时都不等于总 RSS 硬隔离；UNKNOWN 拒启。旁路已备：30 个调用位置及身份（cdx 仓外 /tmp/cdx38-freeze-matrix/），全部 NOT_RUN。
- 隔离 WAIT 结束：董秘代裁选 1（改宿主 cgroup；不改测峰口径、不换机、禁 prlimit 兜底），background 已启用 memory 控制器。cc 10:04 实测包装 /tmp/cc38-memscope.sh（sha256 66c04dac1caf77f9…，仓外）：子组 memory.max + swap.max=0，以 uid 1000 运行；256M 限下 100 MiB 分配 rc0 peak 108158976；64M 限下 200 MiB 分配被 OOM 杀 rc137 oom_kill=1；子组用后删除。§24 采证 begin 另记（cdx）。
- 测峰口径（cdx2）：每次尝试独立子组，先入组再 exec，不迁无关进程；memory.max 读回打印；memory.peak 是该子组 cgroup 记账峰值，与进程 RSS 分列；OOM（oom_kill 增量 + rc）照实保留，下界不当 PASS。
- §24 矩阵采证 begin：2026-10-10T10:08:00+08:00（cdx 回执；WAIT 结束 10:04 分列）。memscope sha 66c04dac 已由 cdx 核对；每项 MAX=min(3GiB, MemAvailable−1GiB)；生产、参考、ASan 分别隔离；30 项串行，首红停。
- 更正：10:08 的 begin 只是准备开始，不计作业——run.py 读 TSV 时 DictReader 漏了分隔符，首项前崩溃（KeyError flags），未执行任何一项。cdx 修好后重开矩阵，各项 begin 按实际记录（cdx 回执）。
- §24 首红（cdx 回执，实际跑过）：实际首项 begin 10:07:47.937+08:00；前 19/30 项生产/独立参考/ASan 均 rc0 且同字节。第 20 项 modeljob:warnparse（parse2 --warnings --errors），begin 10:13:20.882，生产 rc137：子组 max=peak=2065084416 B（1969 MiB），oom_kill +1，time RSS 2008560 KiB，7.97 s。OOM 峰只是下界，非 PASS；已停，未加上限、未重试；该项参考/ASan 与后 10 项 NOT_RUN。gen 源 b802e4f2 未变，O2 二进制 f997b25d。证据 /tmp/cdx38-freeze-matrix/（results.tsv、19-production.log）。
- 首红定位（cc，只读）：tests/seedmemory.py 既有证据记 warnparse 生产 RSS 2135 MiB，高于本次上限 1969 MiB——上限按 min(3GiB, MemAvailable−1GiB) 取，当时宿主 MemAvailable 约 2.9 GiB（16 GiB 总量，大部分被会话外的 ChatGPT/chrome/node 等进程占用）。即上限低于已知需求，不是生成器回归的证据；但新 gen.c 下的真实峰值仍未知。需要宿主可用内存 ≥ 约 3.7 GiB（上限 ≥ 2.7 GiB 留 1 GiB 余量）才能按既定口径测该项，待裁。
- 恢复前就绪核（cc，10:15）：MemAvailable 2739 MiB → 按公式上限 1715 MiB，低于参照需求 2135 MiB（旧 gen、RSS 口径，仅作参照），未就绪；不重复跑已知偏低配置。前 19 项身份与配置未变，证据保留，不从头重做；获裁后只从第 20 项起开新 attempt。
- §24 attempt2（第 20 项起，董秘裁：宿主上限偏低因会话外 ChatGPT 占内存，非生成器回归；已关桌面 ChatGPT）：启动 MemAvailable 7042154496 B，公式 MAX=3221225472 B（3 GiB），≥2200 MiB 门槛；日志 /tmp/cdx38-freeze-matrix/attempt2/run.log；关联原 OOM 账（第 20 项 attempt1 rc137 下界保留不覆盖）。矩阵生产数据仅当 cc/flags/并发/输入键与证据 margin 覆盖一致时才作为一次采证候选；ASan 或 cgroup 组 peak 不抵生产 RSS 证据；旧 2135 MiB 只是参照，不保证新路线（cdx2）。
- §24 attempt2 第 20 项 warnparse（实际跑过，cdx；脚本首启 NameError 未执行即崩，10:17:44 实际 begin）：生产 O2 rc0，time RSS 2186572 KiB（2135 MiB，与既有证据同值），cgroup peak 2318188544 B，7.80 s；参考 rc0，peak 722587648 B；生产与参考输出逐字节相同。ASan rc137，peak=max=3221225472 B，oom_kill +1——ASan 诊断在 3 GiB 公式上限下 OOM，是下界、非 PASS；首红停，21–30 项 NOT_RUN。§24 规定 ASan 与生产预算分开记，不把 ASan 开销套用生产预算；但 ASan 非零即红。待裁：ASan 诊断项是否可在宿主余量允许时用更高的独立上限重测（会改变'不抬限'），或记该项 ASan 为宿主受限 UNKNOWN 后续跑 21–30。
- §24 路线阶段状态（续接用；裁到后只续获准缺项）：
  | 项 | 生产 | 参考 | ASan |
  |---|---|---|---|
  | 1–19 | rc0 | rc0 同字节 | rc0 |
  | 20 warnparse | attempt1 OOM（下界，保留）；attempt2 rc0 RSS 2135 MiB | attempt2 rc0 同字节 | attempt2 OOM @3 GiB，未结 |
  | 21–30 | NOT_RUN | NOT_RUN | NOT_RUN |
  续接时不重跑已绿的生产/参考；若裁 UNKNOWN，第 20 项 ASan 作为未结诊断义务列入最终出口，不因 21–30 转绿而抹掉。
- §24 全绿（cdx 回执 + cc 逐字节复核 20–29 三路同字节、warnparse ASan 与生产同字节）：30/30 三路 rc0；选用 90 次隔离记录 oom_kill 0；attempt3 首 begin 10:28:11.467、结束约 10:31:53。生产最大 cgroup peak 2318188544 B / RSS 2186572 KiB / 7.80 s（warnparse）；ASan 最大 peak 3465375744 B / RSS 3304652 KiB / 48.43 s（warnparse，独立上限，不作生产证据）。前 19 项参考/ASan 进程 RSS NOT_RECORDED（仅 cgroup peak）。gen b802e4f2、O2 f997b25d、ASan 97efd271。导出 HEAD 6b127d73 至 761fb70c 仅 research 变化，产品输入未变。
- 版本提交 a5c1a995（0.0.37 → 0.0.38）；source_digest 5782090e。下一步：按最终身份一次采 seed 内存证据（cdx）。
- seed 内存证据 53fe285f（cdx）：C_SHA b802e4f2、BINARY f997b25d，30 路生产 C_RSS 刷新；自检 13/13、gate seedmemory rc0；seedmemorycheck 一处写死旧 RSS 的期望改为由 C_RSS 导出（公式断言不变）。
- 同源参考 UA：/tmp/cc38-final/ua（`--version` 0.0.38，HEAD 53fe285f，13 s）；build_candidate 构建中（cc 持令牌）。
- 准入路线静态差集（只读 tests/seedmemory.py SUITES ↔ C_RSS；未跑 seed，带入新 queue 出口表）：
  | 套件 | 族 | 准入 |
  |---|---|---|
  | seedgen、seedgen-2/3/4 | gen | 暖缓存下可准入（缓存冷/缺 → RESOURCE_UNKNOWN rc2） |
  | seedparse2-1 | parse2 | UNKNOWN：locations、warnings 无映射（x、errors 已证） |
  | seedparse2-2 | parse2 | UNKNOWN：四组复合 flags 均无映射 |
  | com-seedgen | com | UNKNOWN：COM 路线未测 |
  预期 queue 出口中 seedparse2-1/-2、com-seedgen（及冷缓存时的 gen 族）为 RESOURCE_UNKNOWN 未执行义务，按 preauth 记缺证，不自动放行、不计 PASS；扩展映射属协议改动，须另裁。
- 候选（cc，HEAD 53fe285f，实际跑过）：build_candidate shared+六 target+pack-prep-1..3 rc0（shared 119 s）；pack-models 首次 rc2：seed/compilerpack.c `realpath` 隐式声明（glibc 严格 C99，即 H37，本版裁顺延不实现）→ 按 0.0.37 已审核路线 PATH 前置 /tmp/cdx37-host-tools/cc（sha 5c036d42，仅对 compilerpack.c 加 -D_XOPEN_SOURCE=700，产品未改；seedmemory 登记的同一启动器）仅续跑 pack-models（10 s）、pack-driver（3 s）rc0，前 10 步未重做。候选 /tmp/cc38-final/cand/unisacc-next.com sha256 ba3f40cb…，ident check verified；种子对 /tmp/cc38-final/cand/seed/unisacc-seed.com sha256 ace3c040… 7581888 B。comboot stage1→stage2→stage3→fixedpoint 进行中。
- comboot（实际跑过）：stage1 种子 ace3c040；stage2 3 shard rc0（sidecar seed 与 stage1 同）；stage3 3 shard rc0；fixed point holds：安装 unisacc.com == stage3 == 候选 ba3f40cb（10:44:44）。交接核：安装 pair 与候选 build.json 的 artifact_sha256 ba3f40cb、sources_sha256 5782090e 一致（commit 字段 f3557cb2/3ede21b8 不同，二者间仅 research 变化），provenance check verified。
- queue 出口 10:45:01 起（release/tools/queue.sh /tmp/cc38-final/cand UA SEED，PATH 前置同一已审核启动器；状态 /tmp/cc38-final/cand.queue）。

## 0.0.38 新候选出口（2026-10-10 10:45:01–12:18:14；实际跑过）

- queue：650/650 完成，final rc=1（ver038 同口径），单段，INVALIDATE/INTERRUPTED 0；墙钟 5593 s。证据包 ~/.unisacc/evidence/c38-final（含 SHA256SUMS）。
- 出口表（release/tools/exittable.py，rulings/H1 现行）：**565 PASS**；RULED_BASELINE 34、HOST_TIMEOUT 11（H1 具名）、UNVERIFIED_HOST 6（rc77 宿主不适用）、RESOURCE_UNKNOWN 6（com-seedgen；seedgen-2/3/4 冷缓存；seedparse2-1/-2 未映射 flags——与准入静态差集一致，未执行义务，非 PASS）、BLOCKED 27（挂 rowcov-parse2-build 25+、rowcov-enc 系 1 等根）、NEEDS_RULING 1。
- 唯一 NEEDS_RULING `gate-infra`：compilercheck 闭包对照中 README 夹具编辑波及 7 个 exec-driver 套件，原因是 tests/gatedeps.json 已审核树哈希早于 SC1 修片与版本提交（§0：最后一步刷新）。28633d5e `make gatedeps` 刷新后单项 gate-infra rc0（16 s）。tests/ 不在产品闭包，候选 ba3f40cb 不变。
- 作业秒分解（c38-decomp.py，同口径）：有效 565 次 / 7457.7 s；延期 160 / 4130.2；终红/未执行 85 / 1509.2；合计 810 / 13097.0（ver038 799 / 13981.8）。
- ledgercheck --final 0 未结、carry 0。
- 复核补充（cdx 回执：SHA256SUMS OK、分类重算一致、定点身份匹配；cdx2 只独核结果守恒与日志：分类合 650、5593 s、INVALIDATE 0）：gate-infra 补验首跑输出未存档，已于 9612109f 后在同 gatedeps 状态下再跑一次并存 `~/.unisacc/evidence/c38-final/gate-infra-recheck.log`（rc0、16 s，含 HEAD 与时刻），SHA256SUMS 已重算；补验与原账 NEEDS_RULING 1 分列。**更正（机房主任代裁）**：仅刷新 gatedeps 哈希、未真重审，不得称具名补验；gate-infra 出口表与 ledger 记 NEEDS_RULING。state 中 `stalled=1` 原样保留，未凭字段判定曾停滞，末窗 1.34 s 与 BLOCKED 27 零耗时的事件解释待补。后续冻结时 gatedeps 应在稳定点核 reviewed snapshot 有效性：输入有变须真实再审，不只刷新 hash（cdx2）。DEFER 160 次 / 4130.2 作业秒非可省墙钟；与 ver038 是不同候选，不作净提速对比。
- 末窗与 stalled=1 的事件解释（只读 release-queue.log 与 tests/gatequeue.py:642）：window 114（12:18:09，rc75）跑完 com-luatests-build solo 142（46.09 s）；window 115（12:18:13，1.34 s）唯一事件是 `DONE com-luatests rc=1 0.00s predecessor com-luatests-build failed`——前驱失败即判 BLOCKED，不执行、零耗时（BLOCKED 27 均同此机制）。该窗没有实际执行的作业，`window.completed=0`（jobs_s 0.003，prologue 1.722 / epilogue 1.328），按 :642 规则 stalled 计数 +1 → 1；停滞判定须 ≥3（:669），未触发，非停滞。字段原样保留。
- gate-infra 补验经 cdx 复核通过：日志 HEAD 9612109f、12:20:24 rc0/16 s，SHA256SUMS 8 件 OK；28633d5e→9612109f 仅 research 变化，gatedeps 同状态。属新独立补验，首次未存档不追补；原 650 出口 rc1/85 非 PASS 不改写。
- 构建可复现性（机房主任 ②）：候选 pack-models/pack-driver 依赖仓外启动器 /tmp/cdx37-host-tools/cc。选“启动器内容+sha 入 release 证据”：release/c38-host-launcher.json（内容、sha256 5c036d42…、目标 cc 实体/版本、适用步骤、复现法）；不改源码（realpath 入源即 H37，改产品闭包须重冻结，已裁顺延 0.0.39）。
- 封存与 rc（机房主任代裁 1–3，实际执行）：freezecheck 0；封存前 source_digest 5782090e；seal_candidate → ghcr.io/partnernetsoftware/unisacc-candidate@sha256:9b9c1022…（sealed_from_commit 2298c31e，artifact ba3f40cb）；a9a90809 提交 candidate.json；封存稳定点 make gatedeps 无变化；rc/v0.0.38 → 4790de9d。
- CI release-check run 38023907999，head_sha 4790de9d（= rc 目标），12:26 completed success：source、ccinterop-x86、winsuite（win/arm64、win/x86_64）、六目标 candidate、matrix 全绿。
- Draft 前身份核：rc 提交的 candidate.json 版本 0.0.38、artifact ba3f40cb、sources 5782090e、digest 9b9c1022，version.h 0.0.38，一致。按推动授权开 Draft v0.0.38（target 4790de9d，说明=research/c38-release-notes-draft.md，暂无资产）；公开与签名未授权、未做。
- gate-infra 待裁材料（只读，非重审结论）：28633d5e 相对上次刷新 5d9a549b 改了三类已审核戳——compilercheck 的 `src` 树戳与 `src/version.h` guard（来自 a5c1a995 版本号 "0.0.37"→"0.0.38" 一行）、script-inventory/publish-order/gate-infra 的 `prd.md` guard（a5c1a995 增 4 行状态记录）。同区间产品源只此 src/version.h 一行；seed/gen.c（72120ea8）不在这些戳的输入内。即 compilercheck 家族退化为未审核的直接原因是版本号一行与 prd 状态行。真重审须由审核人确认这两处变化不改 exec-driver-* 7 套件的契约后再定，cc 不自判（cdx Draft 复核：isDraft、assets 空、target 4790de9d、CI 同 head 12 jobs success、launcher 三项 sha 匹配）。
- 启动器证据 release/c38-host-launcher.json（228c0321）提交于 rc/v0.0.38（4790de9d）之后，属后置证据，不在 rc 的 CI 覆盖内（cdx2）。
- gate-infra 具名裁定（机房主任 12:31）：通过，落 release/rulings.tsv（gate-infra FAILED README-real-reviewed-snapshot NAMED）。按现行 rulings 对冻结 state 重算：565 PASS、RULED 35、H1 11、77 共 6、UNKNOWN 6、BLOCKED 27、NEEDS_RULING 0（exit-ruled.txt 入证据包；原 exit.txt 保留不改写）；exittablecheck 绿；ledgercheck --final 0。
- Draft 资产（机房主任 12:31 授权，未签名、未公开）：从 GHCR digest 9b9c1022 拉取封存成员，unisacc.com 与本地候选逐字节同（ba3f40cb），build.json artifact ba3f40cb；上传 unisacc.com（2243480 B）、unisacc.com.build.json、models.json 与 SHA256SUMS 到 Draft v0.0.38（target 4790de9d），说明更新为 bd352750 版。只读冒烟：从 Draft 下载 unisacc.com，sha ba3f40cb，`--version` 输出 unisacc 0.0.38。
