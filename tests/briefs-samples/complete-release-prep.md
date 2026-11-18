# 简报节选(改写自 v0.0.75 发布单 release-prep 无历史布局修复)

目标:release-prep 批与 release-prep-neg 批在 qualify 式全新布局(无 0.0.74 历史)下通过,断言数不变。

写域(只许改这些路径):
- tools/check-release-prep.cx
- tests/batches.tsv(只追加或改本单两行)

验收命令(含预期值):
- `sh ./unisacc.com tools/check-release-prep.cx` -> 末行 pos 13/0、neg 11/0
- `sh ./unisacc.com tools/handoff-check.cx -- --base 开工提交` -> 末行 HANDOFF failed=0 scope=LOCAL
- 验收口径块:local-evidence-only、newcx-0、ratchets-15-0、cold-fresh-layout-33s、fresh-layout-no-history、no-gate-bypass
- 新批次与改动批次在全新布局冷缓存口径 UNISACC_RUN_CACHE=0 UNISACC_UNIT_CACHE=0 下实跑,不超过 33 秒。

截止:开工后 15 分钟,12 分钟收尾。

禁区:不得改写域外路径;不得删断言;不得放宽时限;不得使用 UNISACC_E3CACHE=0;不碰 src/ 与 tools/precommit.cx;不提交他人路径。
