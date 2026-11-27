# notes 对照: 草稿对已提交

未跑冷测, 生成器, 批次. 只读.

草稿: v0.1.5 树 `.work/tmp/release-prep-015/notes-0.1.5.md`.
已提交: `f4537e875:release/notes-0.1.5.md`, blob `e3ba57d51e17dcdc4097307a3ecc530d5793a7c9`. 与 pack-ba12b08dfd 工作树 `release/notes-0.1.5.md` 同一 blob. v0.1.5 工作树 HEAD `a03168fa9` 没有该文件.
预跑: `~/.cache/unisacc/prerun/v0.1.5-ba12b08dfd`. frozen-root HEAD `ba12b08dfddefea5a5e9241f095ede4e7f83473d`.

## 实跑否定的句子

(1) 草稿第 25 行: cx-booter-ape 状态 INVALID. 实跑第 3 列是 UNKNOWN, 注记 `no-BATCH_RUN rc=3` (`strict-final.tsv:188-189`). 同文件无 `id=cx-booter-ape` 的 RAW 行. SUMMARY 写 `INVALID=0` (`strict-final.tsv:1190`). "无 RAW BATCH_RUN" 被支持, "INVALID" 被否定.

## 实跑新支持, 已提交 notes 没有这些句子

(1) 草稿第 11 行冻结提交 `ba12b08dfddefea5a5e9241f095ede4e7f83473d`. frozen-root `git rev-parse HEAD` 同值. 已提交第 9 行仍写冻结提交待 freeze-cut 回填, 第 16 行发布 APE 与 F 待正式链回执. 这两句被实跑否定.
(2) 草稿第 12 行 APE SHA256 `824e3f05f72a485293bf0c33ea877c3f905ca3ef7c31a447473931f6ffa1fb1c`. strict-final 数据行 candidate 列同值, SUMMARY 的 tape_sha256 同值.
(3) 草稿第 21 行 593 批, PASS 585, FAIL 7, 未覆盖 1. SUMMARY: `batches=593 PASS=585 FAIL=7 UNKNOWN=1 INVALID=0`. 七条 FAIL: slow-split, check-ln-noderef, chain-ready-launch, chain-ready-launch-b, release-board, check-prerun-drive-infra, retcheck. 未覆盖 1 对上 UNKNOWN=1, 不是 INVALID.
(4) tools-compile 唯一 id 179, 状态都是 PASS. `awk` 对 `tools-compile-*` 数据行 `sort -u` 得 `rows 179` 与 `PASS 179`. 两份 notes 都没有这句. 新支持, 不是对已写句子的否定.
(5) bpidecl 跑完且无 INCOMPLETE. `strict-final.tsv:4` 为 PASS rc=0, 下一行 RAW `marker=CHECK_BPIDECL`. `strict-W0000.tsv.stderr:6` 为 `QUALIFY_BATCH bpidecl PASS`. 本目录 `supervise.log` 检索 `PRERUN_INCOMPLETE` 为 0 行. `run.done` 为 `status DONE`, `ran 216`, `skipped 78`. frozen-root 里旧版 evidence 的 INCOMPLETE 不是本轮. 两份 notes 都没有 bpidecl 这句. 已提交第 47 行 "彩排链两次失败后通过" 说的是彩排链, 不是本预跑, 本预跑不否定也不支持那句.
