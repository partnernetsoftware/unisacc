# v0.1.4 待办汇总(各线交接单合并)

口径:只汇总,不改任何被引文件;预计小时为排期估算。来源路径均相对仓根。

| 项 | 来源文件 | 写域 | 依赖 | 验收规格在哪 | 建议负责窗口 | 预计小时 |
|---|---|---|---|---|---|---|
| C3 S3 重做 | research/cdx/c3-s3-v014-check-design.md(规格 v2,58 行 + B1-B5);agyneon 交接单(本树未见,待补) | cx/sys/proc.cx、tools/check-proc-s3*.cx、tests/{batches,coverage,manifests} | S1e/S2 已落地;批次容量 4096 | 设计文档「PROC_S3_ROW rows=58 asserted=58 failed=0 unmeasured=0」与 B4 四条具名腿 | cdx(规格)+agyneon(实现) | 8-12 |
| rebase-rev 只读 plan(设计+规格) | research/agy-h/toolline-series/README.md:151-160;research/agy-h/toolline-roadmap.md(S1) | tools/prerun-drive.cx、tools/check-prerun-rebase.cx、tests/{batches,coverage} | 先于短批装箱(同文件串行) | README Step 1:`check-prerun-rebase.cx` 与 `check-prerun-drive.cx` 全绿,10% 抽样兜底 | agyneon | 3-4 |
| 互斥 v5.x | research/agy-flow/mutex-v014-handoff.md;research/agy-flow/commit-gate-mutex-v5-only.patch | precommit/devsvc 提交门互斥实现 | 批次容量 v4 已落 main | 交接单的过门序列与安全子集 | agy-flow | 4-6 |
| 短批装箱 | research/agy-h/short-batch-packing.patch、short-batch-packing.md | tools/prerun-drive.cx、tools/check-prerun-drive.cx、tests 表 | rebase-rev 合入后 | 补丁说明:红 18 项短批用例;绿 5 轮尾窗 + 527 批守恒 | agy-h | 1-1.5 |
| 门 23 PIK1 + launch-env + freeze-cut | research/agy-h/gate23-pik1.patch、chain-ready-launch-env.patch、freeze-cut.patch、toolline-series/README.md:13-17 | tools/chain-ready.cx、tools/check-chain-ready.cx、tests 表 | launch-env 与 gate23 同文件严格串行;cdx 首轮返修 | README Step 1-3 的串行过门与 PASS 原行 | agy-h | 2-3 |
| tape T1.6 与 tl_dce 路线 | research/ompgpt/tape-v014-route-options.md、tl-dce-mark-complexity.md、tape-m0-final-status.md、research/agy-flow/tape-t14c-speed-baseline.md | tape 运行时/`tools/tapepack.cx`/tl_dce 算法(SID) | 静默窗三档原始数据;102 对象专用容器 | route-options「相位分开、二选一、不掩盖解释器速度」与 15 倍门不等式 | ompgpt | 10-16 |
| 对象不可复现(`.unisa.tape` 嵌绝对路径) | research/ompgpt/object-nondeterminism.md、public-asset-path-audit.md | tape 产物路径生成 | 无 | 两文的复跑定位与路径审计清单 | ompgpt + agy-flow | 2-3 |
| notes-evidence 入库 | research/cdx/notes-0.1.3-skeleton.md:45 | tools/notes-evidence.cx(+check、批登记) | 各证据路径就位 | 缺输入保留占位,数字/哈希从证据提取 | cdx | 3-4 |
| dvqualify `--out` 唯一性护栏 | research/agy-h/toolline-roadmap.md:36(S7);tools/dvqualify.cx:705-718 红例 | tools/dvqualify.cx、tools/check-dvqualify.cx、tools/prerun-drive.cx | 与 S6 串行(同文件族) | 非 `--resume` 下 `--out` 已存在必 rc=2 且原 sha 不变;W99xx 不 unlink | ompgpt + agy-flow | 1.5-2 |
| S5 裸调用棘轮 v3 与 proc 残留(I9、裸 fork/waitpid) | research/ccogweco/s5-bare-proc-ratchet-v3.patch、bare-proc-calls.tsv;research/cdx/c3-invariants.md:23(I9) | tools/check-bare-proc.cx、tools/check-check-bare-proc.cx、build/bare-proc-baseline.tsv、tools/ratchets.cx | 版本树合 main 后登记 | 夹具 13 断言;ratchets 20/0(私有树原行);I9 归 S4 的 `-ENOSPC` 用例 | ccogweco | 2-3(棘轮入库)+4-6(I9) |
| osx/win 缺口 | research/agy-flow/tape-osx-win-gap.md | tape osx/win 路径与证据链 | tape M1 范围裁定 | 该文缺口清单与 UNCOVERED 标法 | agy-flow + ompgpt | 6-10 |

## 备注
- 来源缺失项:C3 的 agyneon 交接单、对象不可复现的部分原始复跑记录,本树未见,排期前请对应线补路径。
- 所有 cx/ ext/ tools/cxlib 改动按现规则:先预热 canary、单次 `--whole` 多路径、带 git 索引私有仓跑 cx-lint 与 precommit 同款。
