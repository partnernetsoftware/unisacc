# 生成 NN 的 TSV 表只读审阅与体检摘要 (2026-11-18)

本报告基于只读实测整理(明细见 [nn-table-review-2026-11-18.tsv](../release/evidence/nn-table-review-2026-11-18.tsv)), 未修改任何仓内产品输入或既有表; 未在 arm64 硬件运行验证的 ELF 行为统一标注为 UNMEASURED。

## (1) 盘点: 规模、分类与 --check 覆盖

- 配方网全量: 使用 seed/gen.c 与 tools/table-lint.cx 跑完 42 个配方网, 共 137282 状态、265521 条边、24137 寄存器引用; 全仓 rule table 行键零重复零冲突 (dup_rows=0, conflict_rows=0)。
- exec 目录 1102 个受追踪 tsv: facts 243、manifest 182、byte 198、result 196、template 86、fresh 49、other 148; 按阶段集中于 parse2 (450)、facts (243)、enc (113)、lower (99)、pp (64)、根级 (29) 与 lex (23)。
- 生成器与校验覆盖: 243 个 facts 中仅 45 个有生成器或校验标记 (facts-gen.cx 37 个 + bank-owners 10 常表 PASS, 另 8 个非 facts-gen 派生表), 其余 198 个为手写。
- 结果表校验缺口: 196 个 result 表仅 8 个受 tests/check-decide.cx 规则等价校验, 189 个仅受 check-literal 计数约束; facts-gen 与子阶段规则表生成器尚未纳入 exec/facts/derive.tsv 统一派生物门。
- 权重与引导层: weights 共 48 个文件(20 个 gold 表全过 gold-table --check, 其中 11 个带 seed 规则字节对齐、9 个仅做域和类集合校验), kernel 含 41 个生成产物, seed 含 45 个文件。

## (2) 体检: 健康度、字面量棘轮与跨目标一致性

- 高危阻断项 (table-lint): librarycontract 与 librarypartition 因转移到 DONE 未写状态行被 delta.cx 拒载 (rc=2, 2 网完全跳过 lint); lex 3 网 (shared-e1/warnlex/tokenlex) 因 domain 与 257 键全检冲突输出 44 个常驻 ERROR total (rc=1)。
- 真实 unwritten 边界漏写: 52 例 (15 寄存器) 扣除 Z0/tf_fault_valid/tf_fault_pos/AI_LINES 等 36 例故意读 0 惯用法后, 仍有 5 组跨网未赋值: lower 6 网 of_unit 恒 0, prune 未填 ms_blob/ms_len, nativeabi 死导入 MG.compare, e4/o1 组未写读。
- 未清缓冲 sb-read (46 例): exec/pp/directive-body-result.tsv 第 85-86 行 INC2.f 进入共享入口 KEY.S 前未调 SBCLR, 有残留缓冲读风险; 另有 parse2 LCI.named/LI.named 与 lower LBI.named。
- 噪声与棘轮歪曲: maybe-unwritten 达 9976 例 (占告警 72.8%) 掩盖真实退化; check-literal 全仓 189 表中 188 个余量为 0 (仅 opt/rounds-result.tsv 余 1), 倒逼 objectplan-manifest.tsv 膨胀至 80KB (重复粘贴 106 绑定)。
- 跨目标与静态债务: x86_64 (d0) 与 arm64 (d1) 在 objectplan-manifest.tsv 已分化 (d0.s23 44 行 vs d1.s24 40 行, arm64 滞后 W2a-3); facts 含 ~15 个非模板孤立死标量; exec/README.md 与 exec/lower/README.md 仍停留在 python3 gen.py 旧描述。

## (3) 差分线索: GCC 与已装产品 (0571f88105) 14 例对拍与 ld 互操作

- 确认缺陷 (1) 全局初值与 const/.rodata 缺失: gcc 将 const 置 .rodata、mut 置 .data; 已装产物不产出 .rodata (ro=-1) 且 .data=0, 初值全入 .bss 由 __init_u* 赋值且无 .init_array, ld 链接运行 p03/p10 均读得 0 (期望 5 与 32) 且 const 可写。
- 确认缺陷 (2) weak 丢失: p07_weak_attr 的 WEAK=0 (gcc 为 1), 与强定义链接报错退出 (link_weak_rc=1)。
- 确认缺陷 (3) -funit 非 main 序言污染: p02_nomain_ret 经外部 main 链接调用返回 161 (期望 11); 开发树 fa99b5e470 正修 x86_64, arm64 仍待验证 (UNMEASURED)。
- 可疑线索: 字符串字面量重复入可写 .data (ro=-1); .bss 对齐固定 16B (忽略 struct Big 32B 对齐); 每 .o 附 168B 匿名 .bss、27(x86_64)/54(arm64) .bss 重定位与 st_size=0。

## (4) 结构性风险: 高扇出爆炸半径与选批真空

- 高扇出依赖: net-deps 统计 42 网共依赖 938 文件, 928 非生成器输入中 876 个 (94.4%) 被 >= 2 网共享; 修改 tokenlocations.tsv 波及 32 网 (rel impact 537/1476 批, affected 181 批), parse-constants.tsv 波及 30 网 (rel impact 449 批)。
- 选批真空与前缀两难: 全仓 955/1102 表 (86.7%, 含入网的 359 张核心规则表如 enc/elfobject、elfimage、armlayout、tapeframe、parse/numeric) 没有直接单元测试闭包, 全靠 batch-product-inputs.tsv 41 条粗 exec 前缀编整候选间接覆盖; 若先收窄前缀即成测试真空。另有 102 表处于零闭合状态。

## (5) Top 10 优先级建议 (避开 cx/cx0/plan.cx)

- R-01 [R2收尾 | high | 1-2h]: 补齐 librarycontract 与 librarypartition DONE 状态或放行 delta.cx, 消除 2 网 lint 盲区。
- R-02 [R2收尾 | high | 半天]: 修复 of_unit、ms_*、mg_* 与 e4/o1 共 5 组跨网 unwritten 漏写 (需测字节等价)。
- R-03 [R2收尾 | high | 1-2h]: 在 exec/pp/directive-body-result.tsv 第 85-86 行 INC2.f 进入 KEY.S 前补 SBCLR, 审计其余 sb-read。
- R-04 [R2收尾 | high | 半天-1天]: 将 W2a-3 的 COMMON _unisa_proc_* 与 funit-noprologue 对齐到 arm64 (d1.s24), 并抽 d0/d1 共用块。
- R-05 [0.0.77 | high | 1-2天]: 补全标准 ELF .data/.rodata/.init_array/STB_WEAK 与 >16B .bss 对齐, 关闭外部 C 互操作 3 类缺陷。
- R-06 [0.0.77 | high | 1天]: 先为 359 张规则表补直接表级单测并接 batches.tsv, 再按 net-deps 自动替换 41 条粗 exec 前缀。
- R-07 [0.0.77 | medium | 2h]: 对齐 lex domain:[0,20]/[0,3] 在 tbl.c 与 table-lint 中的边界口径, 把 44 个 ERROR 打零并开启 --strict。
- R-08 [0.0.77 | medium | 1天]: 在 manifest 增加入参寄存器标记以剔除 maybe-unwritten 72.8% 假阳性噪声。
- R-09 [0.0.77 | medium | 半天]: 改革 check-literal 对编码/ASCII 事实模板的零余量惩罚, 并把表生成器接入 derive.tsv。
- R-10 [0.0.78 | low | 2h]: 清理或接线 102 个零闭合表与 ~15 个 facts 死标量, 重写 exec/README.md 与 exec/lower/README.md。
