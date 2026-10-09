# 0.0.37 云机构造交接（cdx，2026-10-09，实际跑过）

版本提交 b398a194。同源参考 `/tmp/cdx37-linux-ref`；产物目录 `/tmp/cdx37-linux-candidate`，含 `unisacc-next.com`/`.build.json`、`unisacc-seed.com`/`.build.json`。详细身份、哈希、探针在同名 JSON。产品与 seed 的 --version 均为 0.0.37。

沿既有 SEED_GEN=0/SEED_C=0 路线完成 shared、六 target、三 pack-prep、pack-models、pack-driver；每步 bound58。seed 由原 build_seed.sh（无 Python）构造，bound58 内成功并实跑 hello。新产品 g4 五组与 C99 64/65/66 同参考 rc0；provenance check 成功。

保留失败：并行 shared 首次超时；收窄为串行后原预算通过。cc 记录同时另一条 comboot parse2 构造导致 OOM；本会话原生 Cgen 测量也被 OOM 杀（RSS峰值5576016 KiB/18.03s），jemalloc试验50s超时，未作为通过或采用默认方案。重构造停止，交 cc 独立克隆串行接续原 comboot（现有 COMBOOT_BUDGET=1，一窗一步）与 Linux 全量642套件。此前旧版本私有产品的独立预验见 c37-linux-precheck；不替代新候选完整门禁。

本记录不是封版回执；自举定点、全量门禁与正式发布尚待完成。未删测试、未改验收措辞、未提高预算。
