# 0.0.39 方案 1 准备清单：真冻结 → 签名 → Draft 换挂 → court → 再议公开（2026-10-10，只读核对后起草）

R3 已按方案 2 APPLIED（0.0.38 签后字节 579f6525 于 Draft 408747179 六格+Defender 绿）；本清单是真 0.0.39 链，前者不抵。每步标“已具备 / 缺 / 须裁”。

## A. 冻结前置（须先裁清）
| # | 项 | 现状 | 状态 |
|---|---|---|---|
| A1 | 0.0.39 范围与 ledger 结语 | 现：13 行结语已落（82331eca）。历史：plans/v0.0.39.md 13 承接行（K2a K2b K2c K1 H37 F4″ COV1 X3 L2 L1′ L1b′ A1 N1）只有“承接：0.0.38 顺延 N”标记；ledgercheck --final 报 0 未结系语法通过，**无一行真结**（COV1 实已覆盖待写已完成） | 已裁 R9：COV1 已完成，余 12 行顺延 0.0.40（结语已落 plans） |
| A2 | WF1–WF5 收口口径 | 各有首刀与复核闭合，均为“要求/工具”，非产品验收 | 已裁 R9：流程工具已交付，不抵产品验收 |
| A3 | 产品闭包变化 | 自 a5c1a995（0.0.38 版本提交）仅 8a433c5d（exec/enc，COV1）；seed/ exec/build exec/facts 未变 | 已具备 |
| A4 | §24 生成器矩阵 | COV1 已跑 30/30（历史证据成立）；冻结时须按当前矩阵完整键核复用，键不符即重跑 | 可复用修片后矩阵（COV1 快照，完整有序输入一致，非旧 0.0.38 矩阵；e1bbe7bb；cdx2 独审通过，ASan 首 OOM/非 LSan 限定保留） |
| A5 | seed 内存证据 | 现 key 6ec1fe9d67dd0733 已接线（df2bae7e）。历史：原证据 e8d10ecb97107e7b 失配（parse2 相符）；gen.c 未变不等于证据有效 | 补证已有（失配唯一来源 exec/enc/x86-procs-result.tsv；30 条 C 路线 rc0/oom0，gen sha f997b25d；e1bbe7bb）；gen 证据常量已接线（df2bae7e = 613f410b 同内容，key 6ec1fe9d67dd0733，30 项 RSS 按本轮实测、余量算法不变；cdx2 独审通过）：暖 C 具名路线具备；COM/Python 冷/四路实峰仍 UNKNOWN |
| A6 | 宿主 | m4pro 无（F4″ 缺口照列）；H37 启动器路线 5c036d42 证据在 release/c38-host-launcher.json | F4″ 缺口；其余已具备 |
| A7 | 共享检出脏树 | examples/*.c 九个未认领删除，gate-infra 在共享检出红（干净 HEAD 绿）；queue 在独立 worktree 跑不受影响 | 须认领方（不代恢复） |

## B. 冻结链（授权后按序，一令牌串行）
1. 版本提交 0.0.39（src/version.h）→ freezecheck。
2. build_ref 同源参考；build_candidate（PATH 前置已登记启动器）；ident check。
3. comboot stage1→2→3→fixedpoint；pair 与 build.json 对核。
4. gatedeps 冻结稳定点刷新并核有效性/真审（P4：版本提交后、queue 前，否则 src/version.h 变更会红；刷新≠审核）。
5. queue 完整出口（约 1.5 h）；exittable 对账；新红一次报裁。封存后 gatedeps 另核一次（见 6 之后）。
6. seal_candidate（GHCR）→ candidate.json 提交 → rc_tag v0.0.39 → release-check 对 rc 绿。
7. unsigned_receipt → Windows qualification → company + approve → 下载签后，核 before=候选、after、Authenticode。
8. Draft v0.0.39（release 408747179）同步 target_commitish 为新冻结链完整 SHA（现 134d7e9e 非新冻结，draftfetch 会拒），换挂签后 0.0.39 字节、新 asset_id、签后 sha，独核 tag → release-smoke + defender-scan（draft 模式，preflight 写、矩阵读、测前测后核 sha）。
9. r39 acceptance 回执（release_eligible、courts、published.public_assets；R2 资产集合）→ publish.sh 资格闸（owner-promotion 明列）→ **公开须另授权**。

## C. 已知风险
- queue 本机 ~5600 s 墙钟；内存隔离 memscope 与 ASan 独立上限口径沿用 0.0.38 已裁。
- Draft 目前挂 0.0.38 签后字节（方案 2 演练）；第 8 步须换挂，资产 id 会变，court 须绑定新 id。
