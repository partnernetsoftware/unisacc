# 0.0.107 夜间实测教训简录

窗口: `agy-h` | 范围: 第二轮(`63354b4a2c..8a0b293973`) 实测暴露问题与处理定论 (只记事实)

1. 冷材料类 vs 设计性贴限的判别法:
   - `frozen-root` 更新会触发 `dvqualify` 生成新布局目录(`tree-NNNN`), 且工具材料键含布局目录身份。
   - 有外部材料且可落 `prep-cache` 的批属冷材料构造: `tsv-lint-root` 在新布局 `tree-q7bjblqd` 首次构造 `tsv-lint-tool-HEX.bin` 耗时 `57633ms` 记 `UNKNOWN rc=3` (`W9917b`), 同布局缓存命中后 `W9917c` 仅 `11285ms PASS` (`assertions=2`)。
   - 无外部材料(`materials=-`)且 `headers=tree`、每次新建临时仓重跑多轮编译器工具的批属设计性贴限: `precommit-list-cap` 在同布局 `tree-q7bjblqd` 连跑 4 次(`W9915 55557ms load1=7.49`, `W9915b 55530ms load1=1.82`, `W9915c 55486ms load1=1.93`, `W9915d 55492ms load1=2.21`)均稳定触 `run-batch.cx:264` 的 `53s` 子进程限(`transport=-1`), 不随低负载消失, 按 `expected-unmeasured.tsv` (`reason=window-timeout`) 登记并留待 `0.1.1` 拆批。
2. 补跑须在稳定布局下串行推进:
   - 每同步一次新 `B-t` 提交至 `frozen-root` 都会刷新布局路径并导致工具材料重编; 应集中落完 `B-t` 提交并更新一次 `frozen-root` 后, 在同一稳定 `tree-NNNN` 布局目录内完成材料落盘与低负载复跑(`W9931`、`W9917c`、`W9993b` 均于稳态布局过关), 外层 `timeout` 须大于 `55s` 窗加准备开销(例如 `75s`), 避免 `60s` 外限截断写出(`W9915b` 初次曾只留 `443B` 表头)。
3. `G1` 放行补丁经 `cdx` 阻断评审后收窄:
   - 对 `window-timeout` 的 `G1` 放行补丁不可泛化匹配普通 `UNKNOWN`; 经 `cdx` 核查阻断后, 提交 `2d387e524d` 将 `tools/release-exec.cx` 与 `tools/check-release-exec.cx` 的 `window-timeout` 严格绑定到对应批的 `RAW` 传输失败行与单批 `precommit-list-cap`, 保持 `55s` 时限与其余批零豁免。
4. `W0000` 轻窗口锁 (`module-units-forward` 教训):
   - 第二轮将置顶批 `module-units-forward` 放入冷首窗 `W0000` 致连续两次 `rc=124 INCOMPLETE`, 而同一批在 `attempt=2` 热材下实测仅 `8520ms PASS` (`assertions=7`)。置顶批不应占用 `W0000` 冷启动窗口, 须由极轻预热窗口承担冷布置。
5. `premerge-selfcheck` 拆批门补跑:
   - `tools/release-exec.cx:575-642` 对 `tests/split-parents.tsv` 要求分片全在 `strict.tsv` 内且 `PASS`, 但 `premerge-selfcheck-main` 与 `premerge-selfcheck-rd` 不在 `batch-product-inputs.tsv` `517` 批内, 致使 `--dry` 停于 `GATE 1 STRICT_SPLIT_INCOMPLETE FAIL premerge-selfcheck`; 用同候选 `APE2` 单跑 `W9994` (`50251ms` 与 `49898ms` 全 `PASS`) 并 `merge` 入 `519` 批后即一次过 `--dry`。
