# 0.1.0 封版车风险与停手表(2026-11-25 ccogweco)

依据:research/cdx/v010-run-order.md、research/agyneon/v010-prerun-plan.md、f2/round2 各页与 107 夜事故记录。
一页速查;触发只看命令原行,不听口头;停手与升级条件写死。

## 1 分阶段风险表

| 阶段 | 已知风险 | 触发信号(命令原行) | 第一动作 | 停手条件 | 升级 cc |
|---|---|---|---|---|---|
| 取号 | 版本号取错/重复;cxlib 已 bump 过 | `grep -P '^tools.cxlib\t' build/tlib-packages.tsv` 与 `layer-api --sha-only tools.cxlib` 结果与预期版本不符 | 停,重核 base 与上一号 | 号码非 0.4.12 之后下一号 | 立即 |
| 冻结 F | 冻结行写错 base/deadline;旧 106/107 行残留 | `grep -P '^0\.0\.107-FREEZE|^0\.1\.0-FREEZE' build/freeze.tsv` 出现两行或 owner 非 cc | 只留 0.1.0 行;旧行删/注释 | 两行并存或 owner 不符 | 立即 |
| 整链 | 链内红;helper 路径丢失(107 夜) | `exit.tsv` 非 0 或 `CHAIN FAIL n name rc=R`;或 `CHAIN INPUT ident-helper` 类缺路径 | 保留链目录原行;核 UNISACC_IDENT_HELPER 路径存在 | 失败相位属产品/身份 | 5 分钟内 |
| 链前置 | 冻结重写被 prepush 拦;verseq R3;cold product-map | `PREPUSH_REFUSE`;`verseq-gen --check` fail 非 0;product-map 生成超时 | 按门名修输入,不跳 | 同一门两次失败 | 立即 |
| 预跑前 W0000 | W0000 被重批占用吃冷窗 | `head -n 25 windows.tsv` 的 W0000 批名与预估时长;`rg '^W0000' windows.tsv` | 重生成计划,保证 W0000 轻载 jobs=1 | W0000 非轻批 | 立即 |
| 四批贴限单跑 | 51-57 秒贴 55 秒限,rc124 | `PRERUN_WINDOW ... rc=124` 或 `BATCH_TIMING total_ms` 接近 55000 | 单批串行重跑,原窗并发口径 | 同一批两次贴限 | 报,不升级 |
| 预跑红停 | 产品红;G1 window-timeout 误判 | `PRERUN_STOP`/`PRERUN_PRODUCT_RED_STOP`;`no-BATCH_RUN` | 产品红:停并报;夹具红:2 分钟 resume | 产品红或两次同因 | 产品红立即 |
| strict 折叠 | 原始 FAIL/INCOMPLETE 与补跑行混叠 | `grep -E '^(FAIL|INCOMPLETE)'` 原行数 vs 折叠后 SUPERSEDED | 按文件名字典序后者在后;保留原行 | 折叠后仍 FAIL | 立即 |
| CI | 瞬态 404、真红、attempt 混淆 | `CANDIDATE_QUALIFY` 非 QUALIFIED;fetch 格失败 | 瞬态用 G4 `--redispatch` 新 run;真红停 | 重派后仍同格红 | 立即 |
| 公开 | 首次公开后读回失败(106 夜) | 读回非 READY / `RECEIPT_DIFFERS` | 不公开;同 argv 再 warm | 再次非 READY | 立即 |
| 读回 | 冷缓存超时;frozen-root 布局目录错(107 夜) | `readback` smoke rc 非 0;或 `frozen-root` 缺 .git/布局 | 同 argv 预热后重跑;核 frozen-root 的 `config/HEAD` | 两次失败 | 立即 |
| release-post | R33 登记值不符;备份缺失 | `OLD_PRODUCT PREPARED` 缺或 version 非 0.1.0;`POST_ROOT_BACKUP_GATE FAIL` | 补备份/重登;不改公开资产 | 登记值与公开 APE 不符 | 立即 |

## 2 107 夜六个具体故障的对应预防

| 故障 | 预防动作(0.1.0 车) |
|---|---|
| helper 路径丢失 | 起链前 `test -x "$UNISACC_IDENT_HELPER"`;未设则按手册路径核存在,不临时改 env |
| 冻结重写拦截 | 冻结行只由 cc 在起链前写一次;任何后续取号先重写行并复跑 23 |
| verseq R3 | 合 main 后先 `verseq-gen --check`;fail 非 0 不写冻结行 |
| cold product-map | 起链前预热 product-map;超 55 秒则拆或预生成,不改界 |
| frozen-root 布局目录 | 链止后核 frozen-root 的 `.git/config`、`HEAD`、layout 目录齐全再跑 P1-P3 |
| window-timeout G1 | 预跑红停按 S1 分类;G1 类先看 `no-BATCH_RUN` 与 transport,不当产品红 resume |

## 3 边界

- 本表只列触发信号与动作,不改任何判据;真实原行来自当轮日志与 INDEX。
- 同一故障两次即停手报 cc;不换说法重试;不放宽 45/120/55 秒与 2 lane。
