# U4.3 与 U4.5 回执

基线 2943677b7f。先套 u4-1b.patch，再套 u4-3.patch 与 u4-5.patch。三片在基线私有索引 git apply --check 通过。对齐补丁修改 10 个既有 A 路径，其中 4 个恢复到 U4.2/U4.2b 前的内容。U4.3 新改 control-result.tsv（71 加、7 删），U4.5 新改 errors-template.tsv（4 加）。压叠后 U4.1 五路径、对齐六路径、U4.3 与 U4.5 各一，总计 13 个 U4 A 类手改路径（U4.4 两路径待并）。派生物与版本总数由集成者以 lane-inputs 重算。

(1) seed-gen parse2 --errors --warnings --locations rc=0，图含 13437 状态。四项事实生成 SAME，见 u4-3/out/facts-final.log。严格表 lint rc=1，报 errors=0、warnings=1709；当前片未建立同图旧基线的警告差集。
(2) u4-3/out/def.e3:22 及 same.e3:22 均为 named_elf:，无 f:，两例 rc=0；同名重复声明合法。a05、a06、a07、a08 的对应 .err 首行分别具名冲突、晚于定义、块作用域对象和非字符串，四份 rc=1 且 tape 为零字节。block.err:1 为块作用域函数声明定位拒绝，object.err:1 为全局对象定位拒绝。
(3) u4-3/out/u_asm.err:1 的实际 c10 源探针是第 3 行第 5 列、inline asm、rc=1；osx-target.err、osx-arm-target.err、win-target.err、win-arm-target.err 各第 1 行是文件坐标与 asm label unsupported for target，均 rc=1。探针用 E1 与 E3 真生成网、模拟器资源选择，候选 -c/-o 的 stderr、nm 及正式 check-diag-contract --live 留待 U4.6。既有 E1 volatile 拒绝保留；本片 c10 的位置通过普通内联 asm 分流验证。实际非 ELF 平台名由目标选择收据给出。
(4) 统一键见 wire-form.md。U4.4 须继续保留 C 名在类型与作用域通路，仅在发射符号时查询标签。若合并后的 U4.4/其它片已改同一绑定事实，先复核 constant 同名定义与生成配方。

补核 eb8473d91c 线形已合、绑定未合的精确基线。旧严格 lint 为 0 errors、1704 warnings，与新片相比增加 5 条 maybe-unwritten，分别为 u4_at、u4_new、lw_res、u4_key、u4_old。原始前后表在 u4-3/out/lint-e3-wire-baseline.log 与 lint-e3-errors.log。分支读写顺序的有效探针见上。
