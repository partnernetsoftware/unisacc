# 推送备清单: main 领先 origin/main 31

日期: 2026-11-23; 只读; 未执行 push。
HEAD=`2e2473c7c0`; origin/main=`6abd4a3847`; merge-base 同 origin; behind=0(fetch 后无他车道推进,无需合并计划)。
作者窗口: 31 笔 first-parent 均为 wanjo(cc 车道署名); 无他窗作者。

## 风险分类计数
- CORE 核心门类: 10
- TOOL 普通工具与检查: 15
- DOC 纯文档/research/记分: 6

## 核心门 gatekeeper 标注
- 待复核 PENDING: `d8cb48cc37`(precommit/chain-ready)、`647e166860`(ratchets/precommit)、`4c9406d48e`(precommit C6/C1/C2/P2/P3)
- 已推过已复核(在 origin): `9f3e41f842`、`efd8e525bb`
- 其余 CORE 未在本次点名列表: `7bb64c2bf8` install-ape、`7093632d70` release-exec、`6ec5c34c9a` premerge-union、`55206270c2` ci-early、`b60dc8b230` prerun-inputs(+release-chain)、`30df43a734` release-chain、`152f8f58e9` envhygiene/chain-ready -> 标 UNMARKED(非本次三笔待复核)

## first-parent 清单(旧->新, 1..31)

| # | short | 类 | GK | 路径摘要 | 说明 |
|---|---|---|---|---|---|
| 1 | `7bb64c2bf8` | CORE | UNMARKED | research/2026-11-23/freeze-runbook-draft.md, tools/check-install-ape.c | tools/install-ape: auto-backup existing APE Root |
| 2 | `7e33b832f2` | TOOL | - | tests/pp-equiv-corners/backslash-include.c, tests/pp-equiv-corners/con | pp-equiv: 13 例预处理边界语料入库 tests/pp-equiv-corners ( |
| 3 | `282753ec87` | TOOL | - | tests/pp-equiv-corners/sub/layer-044.h, tests/pp-equiv-corners/sub/lay | pp-equiv: 13 例预处理边界语料入库 tests/pp-equiv-corners ( |
| 4 | `b69eb2f14b` | TOOL | - | tests/pp-equiv-corners/sub/layer-104.h, tests/pp-equiv-corners/sub/lay | pp-equiv: 13 例预处理边界语料入库 tests/pp-equiv-corners ( |
| 5 | `47fe9e13bd` | TOOL | - | prd.md, tests/batches.tsv, tests/coverage.tsv, tests/pp-equiv-waivers. | pp-equiv: 预处理等价对拍(manifest/run/status,清单 sha 入每条 |
| 6 | `d8cb48cc37` | CORE | PENDING_GATEKEEPER | tools/chain-ready.cx, tools/check-precommit-derive.cx, tools/precommit | tools/precommit,chain-ready,check-precommit-deri |
| 7 | `7093632d70` | CORE | UNMARKED | tests/release-exec-assertions.tsv, tools/check-release-exec.cx, tools/ | tools/release-exec: add read-only POST_ROOT_BACK |
| 8 | `6ec5c34c9a` | CORE | UNMARKED | build/table-index.tsv, tools/check-premerge-union.cx, tools/premerge-u | tools/premerge-union: add --shard merge-traps ch |
| 9 | `647e166860` | CORE | PENDING_GATEKEEPER | tools/check-ratchets.cx, tools/precommit.cx, tools/ratchets.cx | tools/ratchets,precommit,check-ratchets: dup-cen |
| 10 | `c0da7baf14` | DOC | - | research/2026-11-23/freeze-rehearsal-plan.md, research/2026-11-23/free | research/2026-11-23: land freeze-runbook load-si |
| 11 | `55206270c2` | CORE | UNMARKED | tools/check-ci-early.cx, tools/ci-early.cx | tools/ci-early: add read-only --preflight self-c |
| 12 | `e02700b905` | DOC | - | research/2026-11-23/freeze-rehearsal-plan.md | research/2026-11-23: update freeze-rehearsal-pla |
| 13 | `91e072a0b7` | DOC | - | research/2026-11-23/freeze-rehearsal-plan.md | research/2026-11-23: add prerun-drive window-lan |
| 14 | `e2616a921f` | DOC | - | research/2026-11-23/freeze-runbook-draft.md | research/2026-11-23: update freeze-runbook S1/S2 |
| 15 | `4c9406d48e` | CORE | PENDING_GATEKEEPER | build/table-index.tsv, tools/check-precommit-derive.cx, tools/check-pr | tools/precommit,check-precommit-freeze,check-pre |
| 16 | `b36a22129e` | TOOL | - | tests/batches.tsv, tests/coverage.tsv, tests/tools-compile-sources.tsv | unit-e2-compare: 单元层 UNIPP2 流对拍工具与检查(同包全 SAME/变异 |
| 17 | `b60dc8b230` | CORE | UNMARKED | build/links.tsv, tests/batches.tsv, tests/coverage.tsv, tests/tools-co | prerun-inputs: 严格预跑输入冻结(ids 上版窗口经批次表过滤、times 只读副 |
| 18 | `0b1f07896e` | DOC | - | build/pkg-gaps.tsv, prd.md, prd_mempalace.md, skills/unisacc-orch-supe | prd/prd_mempalace/pkg-gaps/记分表: 登记 pp-equiv 全量对拍 |
| 19 | `2d55a45236` | TOOL | - | tools/check-prerun-inputs.cx, tools/prerun-inputs.cx | prerun-inputs: 自上版冻结 rev 起新增的 IMPLEMENTED 批次默认并入 |
| 20 | `02cf40e95b` | DOC | - | research/2026-11-23/freeze-rehearsal-plan.md, research/2026-11-23/free | research/2026-11-23: finalize S3 pilot summary a |
| 21 | `30df43a734` | CORE | UNMARKED | tools/check-release-chain.cx, tools/release-chain.cx | release-chain: show_prerun 的 P0 改 --normal-jobs  |
| 22 | `80a8d8ea53` | TOOL | - | build/links.tsv, build/pkg-gaps.tsv, prd.md, research/2026-11-23/v095- | pair-o: -o 双包对拍工具与检查(必写 rc/UNMEASURED/SAME/DIFF/ |
| 23 | `07ee4ff320` | TOOL | - | release/notes-0.0.94.md, research/2026-11-23/freeze-runbook-draft.md,  | release: tighten 0.0.94 notes honesty wording an |
| 24 | `152f8f58e9` | CORE | UNMARKED | release/evidence/0.0.94/prerun-add-ids.tsv, tools/chain-ready.cx, tool | envhygiene 覆盖未登记 UNISACC_* 变量(导出 UNISACC_PPEQ_PK |
| 25 | `de7b0c4c46` | TOOL | - | tools/check-unit-e2-compare.cx, tools/unit-e2-compare.cx | unit-e2-compare: run-all 子命令(分片循环+逐片一行+末行 STATUS |
| 26 | `a2d449d7ee` | TOOL | - | prd.md, tools/check-team-pulse.cx, tools/team-pulse.cx | team-pulse: agy ●时长/Running command 判 WORKING;回报 |
| 27 | `d8984ffc5a` | TOOL | - | release/decision-0.0.95.txt, release/notes-0.0.95.md, release/rollback | release: prepare 0.0.95 notes/rollback/decision  |
| 28 | `de26907a9e` | TOOL | - | tests/unit-e2-rescan/inc1.c, tests/unit-e2-rescan/inc2.c, tests/unit-e | unit-e2-compare: 0.0.95 rescan 永久夹具(plain/inc1/i |
| 29 | `70221df268` | TOOL | - | build/table-index.tsv, prd.md, tests/coverage.tsv, tests/manifests/tea | team-pulse: 加 --freeze-watch 只读冻结纪律(脏树与近窗提交 VIOL |
| 30 | `418d2370d1` | TOOL | - | build/links.tsv, tests/batches.tsv, tests/coverage.tsv, tests/manifest | cxlib.e2steps 入库(unit-e2-compare 已引用):stderr 归一保 |
| 31 | `2e2473c7c0` | TOOL | - | build/links.tsv, build/table-index.tsv, prd.md, skills/unisacc-orch-su | freeze-broadcast: 冻结日 mux 广播(start/phase/end/abo |

## 两套推法(等下令再执行)

命令形态(已核 usage): `sh ./unisacc.com tools/devsvc.cx -- git push FULL_SHA...`

### A 全量 31
```
sh ./unisacc.com tools/devsvc.cx -- git push \
  7bb64c2bf8aac61afbde8ba3136164367def885a \
  7e33b832f2d802a6e690da0abdb9e08828da05b6 \
  282753ec8756652b15172281556a1e4dff1a6857 \
  b69eb2f14b0b8ddae46a2776f70c28d8ab015eac \
  47fe9e13bd99f2ecd56ce3accb43343ffae4d95b \
  d8cb48cc3718c076f90e0589d3353f65ed66d3be \
  7093632d70c4646358dad6421f7ced86a9a329a4 \
  6ec5c34c9a75e7018a09ea0589ae48653d86dd7b \
  647e166860c8b93edae5b4bd628dc7153cb58de6 \
  c0da7baf14e53875b0d5693a46992a2d2727156f \
  55206270c2c0ffe646d6b099084c05597db49d9c \
  e02700b9050de6dc2804d44ffb873aea4b033b07 \
  91e072a0b7bedc85814dbf6ccfdfaada47e34f79 \
  e2616a921f4d2683b94479dcb72f061df08bf8ec \
  4c9406d48e11f533b6af11778567192243ba879f \
  b36a22129e4ed47630ad15101fcbce5bcace83f0 \
  b60dc8b230708d37aeafb5d746efe4d7e06d6940 \
  0b1f07896ec2d06385c7d14bae087b141601f417 \
  2d55a45236d9cda7d0962cd585bb7c62d05361a3 \
  02cf40e95b955bb8bae13e105b6015fe815937e5 \
  30df43a734657c9359c8c92e537bec9ed9707793 \
  80a8d8ea53439476de2dc67da2f0cfd383ef90fe \
  07ee4ff320a0277dfee43717983ed5c43b6ff59c \
  152f8f58e98025658840bab3e3d844ca5491b003 \
  de7b0c4c46ee0a0871b6a090f30b45f524800be7 \
  a2d449d7ee5cccefac912790ab78f203324cfe57 \
  d8984ffc5a5f274a4aab911673965aebfc772af5 \
  de26907a9e154aaf7cf7d5be7c393d3c5b4f7139 \
  70221df26873fa3d9e5783bd3fed347cbbe3b702 \
  418d2370d17bba9790a1b4bb81ba1ed0819037ff \
  2e2473c7c065da6a8b1e1ae88b5c4ab0bbe98e51
```

### B 只推已清零前缀(5 笔: 至 `47fe9e13bd99` 止, 下一笔即首个 PENDING `d8cb48cc37`)
```
sh ./unisacc.com tools/devsvc.cx -- git push \
  7bb64c2bf8aac61afbde8ba3136164367def885a \
  7e33b832f2d802a6e690da0abdb9e08828da05b6 \
  282753ec8756652b15172281556a1e4dff1a6857 \
  b69eb2f14b0b8ddae46a2776f70c28d8ab015eac \
  47fe9e13bd99f2ecd56ce3accb43343ffae4d95b
```

前缀含义: first-parent 上、首个待复核核心门提交之前的最长连续段(含 UNMARKED 的 install-ape 与 pp-equiv 语料/工具)。
若要求前缀内也不得含任何 UNMARKED CORE,则前缀为空(首笔 `7bb64c2bf8` 即为 CORE UNMARKED);默认按「仅三笔点名待复核」口径用 B。

## origin 他车道
`git fetch origin main` 后 behind=0, merge-base=origin/main=`6abd4a3847`。期间无他车道推进;合并计划:无。

未下令,不执行 push。
