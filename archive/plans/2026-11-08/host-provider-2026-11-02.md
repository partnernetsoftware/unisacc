# --host-provider 归因与二选一（2026-11-02）

根因：`cli.cx:462-471` 只把路径存进 `opts.host_provider_path`；`drive.cx:512` 交给 `bx_diag_opt`
（`entrybind.cx:221-227`），该函数仅判断"provider 非空 + mode/target 组合合法"就返回 0，**从不 open 文件**；
`cx/compiler/hostbind.cx` 的 `hb_provider_filter`/`hb_bind` 在产品里零调用者。故 6 条断言中
absent/badmagic/wrongtgt/bits/trunc 全绿通道、valid 也"通过"——参数对外是死合同。
叠加 #27：33 对象入口链接不导入宿主符号，provider 的 mask 无消费者。
选 A（最小实现）：`entrybind.cx` 加 ~40 行（open 12 字节 + 4 条具名拒绝 `invalid-provider framing` /
`provider-target differs` / mask / reserved），`drive.cx` 写镜像前调用。代价：src 增长违反 srcshape 只降棘轮；
更重要的是 mask 仍无消费者 → 造出"校验通过却不生效"的假合同，把红变绿而不解决 #27。
选 B（撤销，推荐）：删 `--host-provider` 全链——`cli.cx:252,462-471`（及字段 54/205、帮助文本）、
`drive.cx:48,512` 实参、`bx_diag_opt` 的 provider 形参、`tools/check-host-provider.cx`、刚登记的
`tests/batches.tsv`/`coverage.tsv` 两行。净减 src，srcshape 自然向下，外部无依赖（参数从未生效，
坏输入一直 rc=0）。`--emit-host-symbols` 另行判定。
