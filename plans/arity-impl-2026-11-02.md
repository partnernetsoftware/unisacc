# arity 表实现计划（2026-11-02）

决策：parse(E3) call 闭合读独立有限表 `weights/arity/arity.tsv`；不新增第 19 网（`tools/net-gen.cx` NNET 18 与 `tests/nets.cx` 不动），不放进 `weights/gold/`（`rule-gen.cx` 的 NTOT/NRULE 固定）。
路径：`seed/gold_arity_label.c`（规则唯一文本，`table` 243 行 / `call P N M` 一行）、`tools/arity-gen.cx`（`--check`/`--write`/`call`；表头 6 行逐字节、键序=proto×npar×nargs、label∈#head、无多余行、与规则 `table` 输出逐字节相等）、`weights/arity/arity.tsv`（6 header + 243 rows）、`tests/arity-controls.cx`（negative 批次，16 断言）、`tests/batches.tsv`+`tests/coverage.tsv` 各一行、`prd.md` §2 能力速查一行。
不做（下一纪元）：产品 parse 接线要重建 `weights/built.json` 与 `kernel/` 并重装；诊断文本入 `exec/facts/k2-errors.tsv`（`too few/many arguments to function call`）。
受影响批次：新增 `arity-controls`；必跑 `nets`、`net-controls`、`check-cx-gold`、`assembler-cache`（证明未碰 18 网）、`toolsloc`、`ratchets`，另 `batches.tsv` literal 4 条（candidate-identity-lnx-x86_64、runner-faults、cx-register、cx-npfault）。
棘轮：新工具静态零命中 cxraw/cxapis/cxlibc/cxplatform（无 `#include`、无平台宏、无 strcpy/strcat/sprintf/gets，输出走 `sy_printf`）；`tests/toolsloc.tsv` 现在是零余量（438 文件 60538 行 = 基线），`tools/arity-gen.cx` 256 行使总数 60794 —— cc 已于 2026-11-02 批准落地时把 TOTAL 上调到实测值（预计 60794），随同一次提交，纪元结束前不落地。
验证：`sh ./unisacc.com tools/arity-gen.cx -- --check "$PWD"` → `arity-gen	CHECK	keys=243`，rc=0；`--write` 两次 byte-identical 且之后再 `--check` 仍 rc=0；`mkdir out/ac-batch && sh ./unisacc.com tests/run-batch.cx -- "$PWD" "$PWD/out/ac-batch" arity-controls` → `BATCH_RUN id=arity-controls status=PASS rc=0`。
证据：预演全在仓外 `/tmp/arity-impl`（candidate 41e4660d）完成并全绿，主树未动；静默期内不再跑批次。
