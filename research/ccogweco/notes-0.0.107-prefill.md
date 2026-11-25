# 0.0.107 发布说明预填页(基于 agy-h 骨架;2026-11-25 ccogweco)

本页预填已确定内容;占位只留链后才有值的字段,并逐项给回填来源命令。无家目录绝对路径、无 emoji。
本页预填已确定内容;占位只留链后才有值的字段,并逐项给回填来源命令。无家目录绝对路径、无 emoji。

## 状态行(固定句,原样保留)

    状态:同一候选通过六格原生 CI,结论为 QUALIFIED(run <CI_RUN_ID>)。本版未执行全套 s9,未取得全套 s9 资格;
    同候选六格原生 CI 与全套 s9 是两件事,不得混称。

回填:`gh run list -R partnernetsoftware/unisacc --json databaseId,headSha,conclusion -L 5`,取 headSha=<COMMIT_40HEX> 的 run。

## 真实身份(占位与回填命令)

| 对象 | 值 | 回填命令 |
|---|---|---|
| 冻结提交 | <COMMIT_40HEX> | `git -C .claude/worktrees/v0.0.107 rev-parse HEAD`(F=起链提交) |
| 发布 APE SHA256 | <APE_SHA256> | `sha256sum <pack>/layout/unisacc.com` 或候选表 ape_sha256 列 |
| CI run | <CI_RUN_ID> | 同上 gh run list,按 headSha 过滤;失败/重派各留一行 |
| 源快照 SID(可选行) | <SOURCES_SID_64HEX> | pack 输出 `RELEASE_RUN	SOURCES_SID	VERIFIED	<sid>` 原行 |
| pkg / win-arm64 / 两个 build.json / SHA256SUMS(可选行) | <...> | 候选表对应 sha256 列;`sha256sum <layout>/<name>` |

## 产品改动(A 底盘,已落 7 笔)

- (1) B1 前端声明符与重声明 1/3 到 3/3:全局对象重声明冲突、函数/对象双向种类冲突、7.2 文件域不完整数组暂定定义;
  证据:`ad5c3e72a6`、`1028de9b92`、`a183bd4e5d`。
- (2) CXM_MAXU 256 保守方案:前端与计划层容量 256,链接层仍 128 具名拒绝;证据:`9199f8dbbf`(源自 `38ea37666a`)。
- (3) F1 转发桩边界与覆盖增量,module-units 三片与 cxmod-tri 两片;证据:`ac483247c7`、`a7b2dd86c5`、`351be964ef`。
- (4) 不动点证据工具 v4;证据:`d750ad123c`。
- (5) 同步 origin/main(0.0.106 发布与车上六笔非 SID 修复:W0087/W0148/W0189/W0191/W0206 与 freeze 刷新);
  提交 <0.0.106 公开提交>;派生表重算后 `tables-gen --check` 期望 `stale=0 hard=0`(回填原行)。

## 107 追加集逐笔(按实际入车状态保留或删除)

| 序号 | 件 | 一句话 | 证据入口 |
|---|---|---|---|
| #1 | precommit-list-cap v7 | 提交门大输出按上限具名拒绝,`from-run`/`runlist` 截断 fail-closed | `research/agyneon/precommit-list-cap.patch`(8b9e7eec…) |
| #2 | chain-step-reaped-kill | 进程回收后不再按 pid 发信号,失败分支保留原行 | `research/ompgpt/chain-step-reaped-kill.patch`(1271d5c8…) |
| #3 | dvqualify-selector-reuse v3 | 选择器候选与闭包和容器身份入键,避免误复用 | `research/ompgpt/dvqualify-selector-reuse-v3.patch`(61ba39ee…) |
| #4 | r39c-release-exec-sha | `sha_file` 改标准输入取摘要,解除 106 W0150 的 200 字节截断 | `research/cdx/r39c-release-exec-sha.patch`(2e9e41e4…) |
| #5 | producer-cx-booter-ape(3a) | cx-booter-ape 的 booter-raw-elf 增生产者,把无 RAW BATCH_RUN 转实跑 | `research/cdx/producer-cx-booter-ape.patch` |
| #6 | producer-tools-default-regression(3b) | tools-default-regression 的 old-checker 增生产者并核摘要 | `research/ompgpt/producer-tools-default-regression.patch`(4a3aa9ca…) |
| #7 | seal-precheck v2 | 七条封版机械判据的工具与检查,缺项具名拒绝 | `research/cdx/seal-precheck-v2.patch` |
| #8 | check-tool-material-macro v3 | 新增 -tool 材料前的 __FILE__ 八字节扫描(依赖 106 pinscan);**本车不入,顺延 0.1.x** | `research/ompgpt/check-tool-material-macro` |
| #9 | wp4-install-ape v4 | install-ape 缺 APE 时 fail-closed 的负控夹具与材料 | `research/agyneon/wp4-install-ape.patch`(2e4a07d6…) |
| #10 | wp4-material v2 | batch-timing、tsv-lint、ratchets-tool 的材料与分片 | `research/agyneon/wp4-material.patch`(8958b5bf…) |
| #11 | u08-source-missing v3 | 缺源具名拒绝的负控,保留事实行结构 | `research/agyneon/u08-source-missing.patch`(32affee7…) |
| #12 | gen-all-drift-fixture-tests | 漂移夹具负控(防夹具假绿) | `research/ompgpt/gen-all-drift-fixture-tests.patch` |
| #13 | r39e + cc 遮蔽三批修复 + v0107-criteria | 候选与遮蔽批修复,107 准入收口;**本车不入,顺延 0.1.x** | `research/ompgpt/r39e*`、`v0107-criteria.patch`(7d3dc50c…) |
| #14(若入车) | 读回前自动预热 | 公开读回前按精确 argv 预热,失败具名(106 事故固化) | 107 追加集新增三条之一 |
| #15(若入车) | warm_note 收据位置与 scrub | 收据写 release/evidence/<V>/warm-<key>.tsv,家路径先 scrub | 同上 |
| (可选 #16) | rollback 写 INDEX | 回滚动作本身写 INDEX 行,不只 stdout | 同上 |

入车判定:每笔以最终车上实际提交号回填;未入车的行删除,不留"待合入"字样。

## 已知缺口

- (1) cx-booter-ape:若 #5 未入车或仍无生产者,保留"状态 UNKNOWN(无 RAW BATCH_RUN 行),未覆盖,不记 PASS;0.0.106/0.0.105 同形"。
- (2) tools-default-regression:若 #6 未入车,同上句。
- (3) s9 全套本版未跑;严格证据所列批次之外的批次 UNMEASURED。
- (4) U05(provenance):本车 workflow 无取证步骤,记未测(UNMEASURED);关闭条件为 CI osx 格一次带 sha 对照的实跑行。
- (5) #8 与 #13 顺延 0.1.x;不写成已修。
- (6) 本车若发生 CI 重派或读回回滚,按 0.0.106 第 (192) 条格式补写首个 run ID、失败格、源码行与重派 run ID。
- (7) build/pkg-gaps.tsv 的 OPEN 行由 `tools/release-prep.cx:316-322` 自动抽取,格式 `(%d) <task>:<need>(build/pkg-gaps.tsv,OPEN)`。

## 严格证据(占位)

    证据 release/evidence/v0.0.107-strict-<DATE>.tsv:rows=<ROWS>,expected_fail=<N>,superseded=<N>;
    预跑原 merge 为 <PRE_BATCHES> 批 PASS=<P> FAIL=<F> UNKNOWN=<U>;合并后 SUMMARY <FINAL_BATCHES>/<PASS>/<FAIL>/<UNKNOWN>/<INVALID>。

回填:`grep -E 'SUMMARY|rows=' release/evidence/v0.0.107-strict-*.tsv`;预跑原 merge 数取 prerun-supervise.log 的 SUMMARY 行。

## 链后补跑与不动点(条件节)

- 无链后合入:写"零 chain_rev 之后合入、零 SUPERSEDED";有则照 notes-0.0.106.md:233-251 写 chain_rev 到 post_chain_rev、
  放行路径、SOURCES_SID 与 APE 逐字不变、各批原 FAIL 原行与复跑。
- 不动点:fixedpoint-6targets.tsv 含六目标行与原链 compare 原行(compare 四同 count 102/2/6);占位 <COMMIT_40HEX> 与
  <APE_SHA256> 同上回填。

## 落盘规则

- 正式 release/notes-0.0.107.md 只保留七章结构;本页元说明与回填命令不进正式件。
- 任一占位未回填前,不得过 Y1;不得把占位写成通过。
