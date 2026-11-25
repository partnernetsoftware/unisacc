# 0.0.107 与 0.1.0 发布说明"已知缺口"节草案(ccogweco,写作稿)

格式照 release/notes-0.0.105.md 的已知缺口节;每条附证据路径;状态词只用"满足/不满足/豁免(按 R37/R38 重述)/UNMEASURED"。

## 0.0.107 发布说明:已知缺口(草案)

- (1) 整体裁决以当次候选 strict 汇总为准:预跑完成前,下列候选上条目均为 UNMEASURED,不记 PASS。
- (2) B1 2/3 与 3/3:第二车已合 module-units 三分片(a7b2dd86c5);B1 的 7.2 与 t10 夹具与登记的完整重放以 v0.0.107 候选上的 c99-tentative-array-incomplete、c99-array-param-typedef、c99-extern-array-incomplete、c99-global-redecl-conflict 四批实跑为准,当前 UNMEASURED。证据:plans/v0.0.107.md、research/ccogweco/r35-b1-assets-fixedpoint.md。
- (3) CXM_MAXU 256 只到前端与计划层:-o 链接层仍为 128,超限以 `link unit ordinal bound exceeded (tokenlocations.UNIT_MAX=128)` 具名拒绝;产品级 CHECK_MAXU、UNIT_BOUNDARY 与 129/200/256 -o 全链须在新候选实跑,当前 UNMEASURED。证据:archive/research-2026-11-25/ccogweco/cxm-maxu-256.patch、research/ccogweco/cxm-maxu-256-evidence.md。
- (4) 单元缓存键不覆盖导入模块接口(E02):结构体扩字段类改动后旧布局对象可能被复用;决定不改产品键、只留检查例作已知红复现,按已知缺口入本版与封版说明,不改 kind。证据:research/ccogweco/e02-closure.md、research/ccogweco/unverified-extra-rows.md。
- (5) sy_proc_kill 回收后 pid 复用竞态:U33 保留 P1,随 0.0.125 的 sy_child_t 状态位收口;发布链 chain-step.cx:486-487 的"回收后再杀"同类 P0 已按 R38 升级并入 107 追加集(第七笔),修复须在 107 候选上实跑并附夹具。证据:research/ccogweco/p0-ledger.tsv、research/ccogweco/u33-callsite-audit.md。
- (6) UNKNOWN 两项无生产者(cx-booter-ape 的 booter-raw-elf 与 old-checker):按已登记无生产者路径处置,不冒充 PASS。证据:release/evidence/0.0.105/expected-unmeasured.tsv(同口径沿用)、s9 的 *-deferred.tsv。
- (7) 余量缺口:release-chain 批冷态 KILLED 68 秒,登记 0.1.x;其声称 U30(--dry 端到端)由 107 发布时真实五步原行关闭。证据:research/agy-h/margin-packages.md、research/ccogweco/claimed-needs.tsv。
- (8) 106 的六目标不动点与 netregen 12 批 PASS 只作 107 的前置背景,不替代 107 自身链的固定点回执与 strict 汇总。证据:research/ccogweco/fp-106-extraction.md。

## 0.1.0 发布说明:已知缺口(按准入七条,草案)

- (1) 六目标自举不动点连续 3 版:满足的前提是 106、107、0.1.0 三趟各自归档六目标 g2==g3 与回执;106 已实测 slices=same count=6、六对终片 sha 相同(ape-receipt 六切片),107 与 0.1.0 待各自链。任一缺失则本条记"不满足"。证据:research/ccogweco/fp-106-extraction.md、release/evidence/0.0.106/。
- (2) CI 六格 QUALIFIED 且严格证据 FAIL=0:CI 六格满足(0.0.105 verdict=QUALIFIED pass=42 fail=0;106/107 待各自 CI);严格证据在 106 预跑 netregen 两族 12 批已 PASS,汇总行待预跑完成确认 FAIL=0;UNKNOWN 两项无生产者按已登记项处置(不超 2、各带负责人与到期版)。证据:release/evidence/0.0.105/ci-45347740771.log、release/evidence/0.0.105/strict.tsv、research/ccogweco/seal-0.1.0-evidence-map.md 条 2。
- (3) 零已知 P0:发布链 chain-step P0 在 107 修复并实跑后满足;E02 不判 P0(需显式接口变更才触发,非默认路径),以已知缺口入说明;U33 为 P1 随 0.0.125。证据:research/ccogweco/p0-ledger.tsv。
- (4) state-unverified 不超过 3 行:可能不满足。按 R38 口径声称类基数 10;E02 确定为已知缺口保留,U05(mac 主机)与 U33(P1)保留,预期 3 到 4 行(U06 多半留),若为 4 则如实记未满足,不改 kind、不搬账。证据:research/ccogweco/unverified-classification.md、research/ccogweco/seal-0.1.0-evidence-map.md 条 4 三值。
- (5) 前端覆盖三期:不满足(豁免按 R37 仅本条):A 在 0.0.100,B1 在 0.0.107,B2(0.0.109)未开工并顺延,封版说明写明未满足。证据:plans/v0.0.109.md、plans/v0.0.107.md。
- (6) 整链起到公开连续 3 版不超过 120 分钟:仅第三样本按 R37 重述取封版车到 READY_TO_EXECUTE;0.0.105 链止到 F 为 161 分钟(prd_mempalace:43;R30 另记 222 分钟,口径不同),106 起链 20:47:41、链 28 分钟、公开预计 120-160 分钟,107 预计同量级;若 106/107 超 120,本条如实记未满足并注明样本不足。证据:prd_mempalace.md:43、archive/research-2026-11-25/2026-11-24/R37-seal.md 条 6、链目录 checkpoint.tsv。
- (7) 能用清单 30 过 25:可满足。30 项里 29 项就绪(module-units 由 107 车 a7b2dd86c5 覆盖;其余由 WP4 在 107 追加集关闭),唯一保留缺口是 release-chain 本身(不换项);清单须 107 公开前冻结并绑定 strict.tsv 批次行机械计数。证据:research/ccogweco/usable-list-draft.md。
- (8) 其它已知缺口:E02(见上)、U33 的 P1 竞态、余量包里 release-chain 与未拆批次在 0.1.x 收口;不把上述缺口改写成 PASS。证据:research/ccogweco/p0-ledger.tsv、research/agy-h/margin-packages.md。

## 公开前须再核的 5 件

1. 106 六目标固定点四项归档齐全(ape-receipt 12 行、fixedpoint-6targets 六行 SAME、INDEX 两行、notes 原句),107 链完成后按同口径归档;证据入口 research/ccogweco/archive-106-steps.md。
2. 106 与 107 的 strict.tsv 汇总行 FAIL=0、UNKNOWN 两项处置写明;CI 六格各版 QUALIFIED 记录在案。
3. 条 4 声称类最终计数:U08(加负控)、U11(复审已出)、E01(agy-h 正负控 2-3 分钟)、U09/U30/U21 证据到齐;U06 结案或如实留行;E02 保留。
4. chain-step P0 修复在 107 候选实跑原行(补丁+夹具+实跑),p0-ledger 关闭证据落行。
5. 能用清单冻结:29/30 项与 strict 批次行的机械映射由 agy-h 核对;release-chain 缺口登记 0.1.x;fable 对配额与选项过一眼。
