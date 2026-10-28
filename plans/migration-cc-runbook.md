# cc 矩阵证据迁移手册（f3705dfa → 下一准入候选，写于 2026-10-28 17:4x）

前提：新候选已由 cdx 准入并装为 `unisacc.com`（身份记录在 prd/HANDOFF）；csr 授予重活窗（约 15 min）。未准入不迁移。

1. 冻结导出：`git archive HEAD | tar -x -C /home/devali/repos/unisacc-cc-mig-<HEAD>`，拷入已装产品为 `unisacc.com`（chmod 700），`mkdir out/reg`。核 `sha256sum unisacc.com` = 新候选。
2. 逐批 run-batch（每行 ≤55 s，顺序，nice -n 5，监视器绑定循环 PID、总期限 1500 s）：
   `for id in $(awk -F'\t' '$1 ~ /^(stream-e4-|stream-prune-|optimization-|properties-|formats-|stream-model|link-undefined)/ {print $1}' tests/batches.tsv); do timeout 55 sh ./unisacc.com tests/run-batch.cx -- $E $E/out/reg $id <materials> > $E/out/reg/$id.log 2>&1; done`
   材料按行：`streams=/home/devali/repos/unisacc/out/c43-streams-riroot-01`、`ref=/home/devali/repos/unisacc-ref-500d6cf3d8/out/ua_ref`、`hostar=/usr/bin/x86_64-linux-gnu-ar`、`hostld=/usr/bin/x86_64-linux-gnu-ld.bfd`、`hostcc=/usr/bin/x86_64-linux-gnu-gcc-11`（规范路径，run-batch 拒绝符号链接）。`link-undefined-function` 预期仍红（缺陷未修）——它不在矩阵换标清单内。
3. 核对：每个日志 `BATCH_RUN ... status=PASS candidate=<新候选>`；任一 FAIL/UNKNOWN 的格保持旧纪元证据并记原因，不迁移。
4. 生成：`sh ./unisacc.com tools/matrix-flip-gen.cx -- . plans/matrix-flip-cc-map.tsv <OUT_DIR> --migrate f3705dfa <新候选短SHA> out/c43-cc-reg-03`，再 `--check <OUT_DIR>`；拷日志到 `out/c43-cc-reg-03/`；用 OUT_DIR 文件覆盖五张 `tests/*-domain-matrix.tsv`；提交 `-- tests/*-domain-matrix.tsv plans/matrix-flip-cc-map.tsv plans/matrix-gap-notes-cc.md`。
5. 回窗：按真实对象分类（控制器=新候选；被测旧 riroot 流不换标）。
