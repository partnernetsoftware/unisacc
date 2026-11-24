# state-unverified 逐行分类(第 4 条)

事实: build/state-unverified.tsv 共 23 个数据行(表头 1 行另计):22 行 UNVERIFIED + U33 OPEN。R37 文本写"24 行(23 UNVERIFIED 加 U33)",与文件实际差 1 行,以文件为准。

分类规则(R37 第 4 条 (c)): 计数只含对已发布行为的声称缺实跑类: stale-claim、no-run、provenance、violation、flaky、pre-existing-bug、e2e-dry,上限 3;compiler-gap、product-gap、contract-gap、oracle-gap、coverage-gap、ratchet-debt、planned、hygiene、process-gap 属已知缺口与债,转发布说明并保留行,不计数。harness-gap 不在两清单内,单列待裁。

## 声称类(计入上限 3,现 7 行)

| id | kind | 主题 | 现状 | 能否 107 前诚实关闭 |
|---|---|---|---|---|
| U05 | provenance | osx cache-manager 借用副本来源与 macOS 实跑 | 0.0.105 evidence 有 cache-manager.tsv(x64/arm64 sha 与备份源路径),但无 macOS 实跑 | 否(需一次 mac 实跑) |
| U06 | flaky | tsv-lint 同日 PASS FAIL PASS | 未定因 | 否(需红时留 git status 与失败行) |
| U08 | stale-claim | slice-migration-gen 的 source-missing 是否仍在 | 代码仍有该失败路径: tools/slice-migration-gen.cx:90 `if (sy_access(x->src, 0) != 0) return fail("source-missing", x->src);` | 可能:补一条 grep 级证据即可关(建议 107 前) |
| U09 | no-run | affected-iface2 批次无实跑结果 | 未跑 | 否(需实跑) |
| U11 | violation | tools/status.cx 测试夹违规改动内容重审 | 未重审 | 否(需人工重审) |
| U30 | e2e-dry | release-chain 端到端 --dry 未用真实证据复刻 | 只有分项检查 | 否(需 release-chain run 五步实跑) |
| U33 | pre-existing-bug | sy_proc_kill pid 复用竞态 | OPEN,P1 待 fable 裁 | 否(0.0.96/125 线) |

## 缺口与债类(不计数,保留行)

U02 coverage-gap、U07 planned、U12/U13/U18 compiler-gap、U14 hygiene、U19/U20 contract-gap、U22 coverage-gap、U23/U25 oracle-gap、U26 product-gap、U27 process-gap、U28/U29 ratchet-debt: 共 15 行。

## 待裁一类

U21 harness-gap(全套入口分片聚合,423c274c42 已修入口但新候选分片端到端未验收): R37 的两张清单都没有 harness-gap,按字面既不计数也不能自动转缺口;建议 cc/fable 明示归"缺口"(则保留行不计数)或归"声称"(则计入上限)。

## 结论

声称类 7 行,离上限 3 行还差 4 行可关。按当前证据,107 前最可能诚实关闭的是 U08(补一条代码行证据);其余 6 行都需要实跑或人工重审,不能凭现有材料关闭。总计可诚实关闭估计 1 行(U08),远低于 R37 的 8-12 行估计。
