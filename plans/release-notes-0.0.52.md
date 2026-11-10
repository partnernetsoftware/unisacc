# unisacc 0.0.52 -- 发布说明(草稿,未发布;占位以 {{...}} 标出,未实跑一律写未跑)

本版产品候选尚未冻结。下面只记已经进 main 的构造体系事实,以及明确未完成的缺口。产品身份段留占位。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。当前已发布仍是 0.0.51(见 `plans/release-notes-0.0.51.md`)。

## 身份与证据

| 对象 | 身份 | 说明 |
|---|---|---|
| APE `unisacc.com` | {{APE_SHA256}},{{APE_BYTES}} 字节 | 尚无 0.0.52 候选;勿把 0.0.51 的 d5d6d002 当作本版 |
| 构造源 | {{SRC_COMMIT}},源 SID {{SRC_SID}} | 未冻结 |
| 六片 | {{SLICE_TABLE}} | 未跑 |
| 不动点 | {{FP_COMPARE}} | 未跑 |
| P4 包 | {{P4_SHA}} | 未跑 |
| s9 | 未跑(commit/daily/release) | 不称全批通过;native 缓存已在 0.0.51 落地(`bce6f28ef8`),0.0.52 夜跑编排工具已写未提交、未实跑 |
| 原生 CI | 未跑 | 无 0.0.52 ci_run_id |

## 行为变化(升级前请读)

产品行为相对 0.0.51 的增量以未来候选表为准。本草稿不声称任何产品语义变化。

## 本版已落地的构造体系(以 git log 为准)

- **N1 干净克隆冒烟进门禁**:`08f65bc124` `tools/chain-smoke.cx`;补丁触及链文件时门在 `clone --shared` 的干净克隆里跑 candidate-chain 前几相位(`--until list`,总 280s);`candidate-chain --until PHASE`;chain_work 的 mkdir 移到 snap 之后。
- **顺序域与 D2**:`39f59c7dee` `build/order-domains.tsv`;graph-check 对 `cs_target` 按 frozen_order 校验,`order_mismatch` 1 -> 0;check-graph-check 29 项 failed=0。
- **A1 一条命令缺口 2/3/6**:`2f9b4b79a5` `--bootstrap` 缺省 ROOT/unisacc.com;`--bootstrap` 同时安装旁边 `.build.json`;list 后 warmup 相位预编重工具,`graph.tsv` 206 节点。缺口 1(全新 rev 真起)已在 0.0.51 `4411153ff8` 修过 mkdir-before-snap。
- **N6 研发耗时账本**:`08dd41cee5` `agentwork/devtime.tsv` 与 `tools/devtime.cx` append/report;回填 20 条;strcpy 改 tx_copy。账本汇总 `d98fa00eac`(窗口停摆 40%/链缺陷 23%,样本小待复核)。
- **N3 提交器**:`feac30a7cf` `tools/submit-patch.cx`(私有克隆 git add -A、写新名补丁、编译涉及工具、新增 .cx 调 newcx-check、校验登记行);同提交已含 `tests/cxapis.cx --file` 单文件模式与 np_run 带上限;check-submit-patch、check-newcx-check 负例全绿(以提交说明为准)。
- **A0 产品全景表阶段一**:`1f85946110` `build/product-map.tsv` 与 `tools/product-map.cx`;prd 标记段 `--check`;阶段二夹具拆分补丁 `/tmp/grk2-pm-split.patch` 针对 4d984e16ec,对现行 HEAD 未核 apply,未跑不算落地。
- **newcx-check**:`8f43bdfe1e` 本地自查 cxtrunc/outdep/cx-lint;cxapis 判据已由 `feac30a7cf` 改为直接调 `tests/cxapis.cx --file`。
- **集成门预算与 --land**:`4b919b20ea` 阶段剩余不足 57s 转 pending 由 `--until-done` 续跑,不再缩短子进程超时;`--land` 用 git `-F`;推送失败 `--soft` 回滚。H6 基线 `13c97a1444` `429183aa03` `de419e8569`。
- **land 不再整包重跑 affected**:`05a154977e`(integrate-patch la4):入口检查修正、`--land` 不重跑 affected、land 夹具拆独立 daily 批次 `integrate-patch-land`;call() 不再把子进程上限缩到剩余预算(transport=-1 的一个根因假设,未证实已关闭),预算不足改 PENDING;需另一原生宿主的 RUN_REFUSED 记 INTEGRATE_DEFERRED 不算红。门:仅 GATES_ONLY(READY_TO_COMMIT),非全批。
- **基线修复**:`b751e8f51e` tools-compile-sources 补登 13 个新工具、tools-compile-089 分片;部分分片单跑 PASS,其余未重跑;runner-platform 与 osx-* 需另一原生宿主、tools-default-regression 缺材料,均未记 PASS。
- **0.0.53 计划草稿**:`b2b9bd21fb`(随 72 小时裁决分两支)。
- **s9-native 缓存**:已在 0.0.51 `bce6f28ef8`(热 init 16.8s)。0.0.52 A7 夜跑编排工具 `tools/s9-nightly.cx`、`check-s9-nightly.cx` 已写、尚未提交入库,未实跑。

## 已知限制与未声称的性质

- **p3 贴线未拆**:0.0.51 记冷编加本体约 80s 超 60s;`2f9b4b79a5` warmup 意在去掉冷编占用,但 p3 本体是否仍贴线未跑。
- **Mac/Win g2/g3 未自动编排**:0.0.51 用 `--from-image`/`staged-from-image`;0.0.52 无新的自动编排实跑。
- **s9 未跑**:commit/daily/release 三档均未跑;不得用提交档或账本顶替 QUALIFIED。
- **A1 增量未做**:G2 增量(<=5 分钟实测)未做;仅落地缺口 2/3/6。
- **A2 未做**:产品改动,未做。A3(chain_sha 收窄)、A8 为计划包,无合入提交可引用,状态未完成。
- **模型层 M1-M4 未达**:顺延 0.0.53(见 `b2b9bd21fb` 草稿),以 72 小时裁决为准。
- **72 小时裁决**:待定(cc 填)。
- **affected-run 间歇 transport=-1**:门日志 5-9s 空输出(A1 oneshot2 078,N3d 084);未在本草稿称为已修。land-affected 改为按批 `--ids` 降低一次 `-j8` 风暴,根因(np_run/sy_spawn_pipe)未关闭。
- **pm-split 补丁**(`/tmp/grk2-pm-split.patch`)前 80 条提交无落地,对现行 HEAD 未核 apply,不写入身份表。
- 身份表 `{{...}}` 占位(审计实为 6 个,非 7)均无冻结候选收据,保留占位,对应项未测/未跑。

## 验证入口

1. 核哈希:待 0.0.52 候选后填 `sha256sum unisacc.com`。
2. `sh ./unisacc.com --version` 待输出 `unisacc 0.0.52`。
3. 冒烟:未跑。
4. 复现:`release-run --pack` 未跑。
5. 原生 CI:未跑。
