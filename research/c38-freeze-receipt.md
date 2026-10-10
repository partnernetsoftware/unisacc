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
- 隔离 WAIT 时刻：发现（cdx 回执 / cc 实测）约 10:0x；报裁 10:1x（给董秘三选一：改宿主 cgroup / 换有隔离宿主 / 改测峰口径）；恢复 —；采证 —（记录于 09:59 +0800，仍 WAIT）。
- 采证前宿主就绪出口（cdx2）：可验证的 memory 硬上限（memory.max + memory.peak）。MemAvailable、单令牌、ulimit、超时都不等于总 RSS 硬隔离；UNKNOWN 拒启。旁路已备：30 个调用位置及身份（cdx 仓外 /tmp/cdx38-freeze-matrix/），全部 NOT_RUN。
- 隔离 WAIT 结束：董秘代裁选 1（改宿主 cgroup；不改测峰口径、不换机、禁 prlimit 兜底），background 已启用 memory 控制器。cc 10:04 实测包装 /tmp/cc38-memscope.sh（sha256 66c04dac1caf77f9…，仓外）：子组 memory.max + swap.max=0，以 uid 1000 运行；256M 限下 100 MiB 分配 rc0 peak 108158976；64M 限下 200 MiB 分配被 OOM 杀 rc137 oom_kill=1；子组用后删除。§24 采证 begin 另记（cdx）。
