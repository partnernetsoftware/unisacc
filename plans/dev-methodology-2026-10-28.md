# 开发方法学调整（2026-10-28，主人裁定：凡能加速整体开发的都做）

依据：0.0.43 最近两周的实测。产品与自举链本身可靠（同源两代固定点 25 分钟闭合），慢在围绕它的流程：
每个修复都走发布级重证、表接线只能构网后用探针发现、完成门含本机不可达项、协作靠串行窗口。

## 1. 两条车道

| 车道 | 触发 | 内容 | 时限目标 |
|---|---|---|---|
| 快车道（开发） | 每个修复 | 当前已装 Root 直接 `-run` 或私有网；探针集 + 表 lint + `sim --cov`；UCN-free/变更无关输入的 `-E`/token 字节等价回归；交付 = diff + 生成器 + 探针 + 报告（仓内 `out/` 仅放日志与产物） | 改→探针绿 ≤ 1 h |
| 慢车道（纪元） | 每周一次，或主人点名 | 正规 context 107 → PC → 两代 GEN → kind2 → 安装 → 全部 SOURCE 批 → 矩阵换标 | ≤ 2 h（构造 25 min + 批 ~10 min + 换标脚本化） |

规则：
- 矩阵证据只绑定"最近一次慢车道纪元"的候选；两次纪元之间的提交不迁移矩阵，也不因 SID 变化把已有证据标失效（证据带候选 SHA，读者自己判断年龄）。
- 快车道的绿不授 PASS，但快车道的红可以阻止合入。
- 慢车道一次只由一个主刀执行；其余协作者只做只读探针/审查/生成器草稿。

## 2. 表的变更单位 = 事实 + 生成器 + `--check`

- 有限表（exec/pp、exec/lex、exec/parse…）的新增/修改一律通过生成器：事实表（`exec/facts/*.tsv`）→ 生成器（`tools/*-gen.cx`）→ 表；生成器必须提供 `--check PRISTINE TARGET` 做幂等重现（参照 `tools/ucn-pp-gen.cx`）。手改表行只允许在生成器尚不存在的表上，并同时登记"待生成器化"。
- `seed/gen.c` 的 stem 白名单、manifest 硬编码等"表系统中的半硬编码"列为产品债务，按 SID 纪元逐项移入数据（manifest 行声明即可注册新表对）。

## 3. 构网前的"编译期"检查（新工具，.cx）

- `tools/table-lint.cx`：对一组表做静态检查——宿主入口/返回键闭合（每个 `UCNK`/`CANRET` 类键在结果表有对应行）、`*` 行完备、不可达状态、寄存器读前有写（按行内动作序列）、SB 跨状态存活冲突（SBCLR/SBINTERN 与 SBSPAN/SBSAVE 的配对）、观测区间重叠。红即拒绝构网。
- `tools/sim.cx --cov`：把 `exec/pp/sim.py` 的执行器语义移植为 `.cx`，支持覆盖集（状态,观测）输出与两次运行的差集。本周 `1##é` 的根因（`EBN` 无 UCN 行）由此在几秒内定位；这是快车道的主诊断工具。
- 两者都以当前产品运行，不新增 Python。

## 4. 用事实推导覆盖格

- MISSING 格优先用"事实枚举"关闭：从 `weights/gold/*.tsv`、`exec/facts/*.tsv` 生成边界用例（范围端点、两种拼写、首位/续位、每类一正一负），一次产出几十到几百条，oracle 来自同一事实而非手写。
- 新增矩阵格必须同时写明 oracle 来源与证据批；写不出的不进必选。

## 5. 完成门拆分（需主人确认后改 prd/plans）

- 本机门：19 域的本机必选格 + N22 本机可证部分；`--all` 的本机子集 rc0 作为日常完成信号。
- 外宿主门：79 个 NEEDS_FOREIGN_HOST + hosts 域，单列，待真实宿主通道；不计入本机进度。
- 6 个具名 BLOCKED：两周内裁定（删格或定合同）。
- 36 个 NEEDS_PRODUCT_CHANGE（15 个已有源修复）：下一个慢车道纪元集中清理，这是唯一真正的产品工作。

## 6. 协作与命令边界

- 一机一主刀：同一时刻只有一个 agent 改产品输入或写证据；其他 agent 的交付是可独立复现的 `diff + 生成器 + 探针 + README`，由主刀串行审入。
- 每个 agent 的暂存改动在 `plans/HANDOFF.md` 登记所有权与状态，会话中断时他人可接。
- 60 秒规则适用于测试与未知命令；**正规构造链（`source-job-wave`、`pc-wave`、`kernel-build`、`product-build`、`selfhost`）内部已逐作业 bound，允许一次调用跑完一个相位**，不再拆成几十次 harness 调用。

## 7. 两周目标与指标

1. 第 1 周：`table-lint.cx`、`sim.cx --cov` 落地并接入快车道；E2/E1 至少三张表生成器化（含 `--check`）；本文件规则写入 CLAUDE.md（已做）。
2. 第 2 周：快车道清 36 个产品格（先验 15 个已有修复），一次慢车道纪元换证；完成门拆分后重算宽行。
3. 指标：修复"改→探针绿" ≤ 1 h；纪元换证 ≤ 2 h；每周正规纪元 1 次；矩阵迁移由脚本完成（`@旧候选:N/0;旧日志` → `@新候选:N/0;新日志`，按批日志 candidate 字段核）。

## 8. 落地记录

- 2026-10-28 `tools/ucn-pp-gen.cx`（e99742f847）：事实+生成器+`--check`；主树六表由 `c96909e11f` 原始表幂等重现。
- 2026-10-28 `tools/cxlib/delta.cx` + `tools/sim.cx`（d727dec705/5596cb4310）：delta 模拟器，`--cov` 覆盖集；oracle = 与 `exec/c/run.c` 在 103 探针 + 18 真实源上 stdout/stderr/rc 逐字节相等；`run.c` 1.2 MB 输出 11 s（net 执行器 2.7 s，Python 20 s）。用法：`sh ./unisacc.com tools/sim.cx -- DELTA.json INPUT [SRC] [INCLUDE_DIR] [--cov A.tsv]`，两次运行 `LC_ALL=C comm -13 A B` 即得差异状态。
- 2026-10-28 `tools/table-lint.cx`（5596cb4310）：构网前静态检查 total/reach/stale-r/t-keys/unwritten/maybe-unwritten/dead-end/sb-read，4242 态 2 s；当前 pp delta 0 ERROR。已知局限：共享子自动机按寄存器 k 返回时，must-write 数据流路径不敏感，会对宿主寄存器报 maybe-unwritten（WARN，不阻断）；UCN 生成器的 `UCN.D2/D3` 行不可达（下次换纪元时在生成器里只生成被引用的链）。
- 快车道闭环示例：改表 → `seed-gen pp X.json` → `table-lint X.json` → `seed-tbl/seed-net` → 探针 + `sim --cov` 差异 → `-E` 字节等价回归；均不触 SID。
- 2026-10-28 快车道首例：E3 `undefined function` 诊断带 file:line:col（提交见 git log "Report undefined functions at their first call site"）。改表→私有 parse2 构网 32 s→lint 零新增→探针→70 源+90 探针字节回归，全程约 2 小时（含三次按键约定试错，`sim --cov`/读表本可更快）。已装产品尚未重建：`negative-diagnostics` 的 reach 两项在旧 Root 上为红，待下一次慢车道纪元换证；negative/stream-e3 矩阵行届时更新。发现：`exec/facts` 的槽基址 REFERENCED=TOKEN_POS=34·2^40、STATICF=SPLICES=35·2^40 同基址（unresolved 与 tokenlocations），目前因阶段先后未冲突，属潜在别名债务。
- 设计记录（未做）：`diag.gap-cont-user-column`/`e3.ref.diag-expanded-column`。E3 的 `diag_source` 是展开后文本，`dp_col` 在展开行上计数；要给用户列号与用户行摘录，需要 (a) E2 在 UNIPP1 locations 变体里多一段"展开映射"（展开偏移→源偏移，每个宏调用边界一对），(b) tokenlocations 读入并在 DP 里把列映射回源，(c) DP 从原始源（`\0inc/q/`+`\0cli/source` 路径经文件字典）读摘录行；头文件内错误需按 include 记录解析路径。公开 locations 格式变更 + 三组表 + oracle，约一天；不在本次快车道内。
- 2026-10-28 快车道第二例：产品内分相时间戳（`UNISACC_RR_TIME`：construction / 每流 model inflate|cache / 每流 inference / output），`tests/phase-timing-stamps.cx` 读入八个剩余格（无戳产品报 UNMEASURED+原因，批仍绿；hit 必须零流）。快车道验证用"驱动切片 + 外部包"的 shim 产品（`driver.elf --models PKG`），57/0；已装产品 54/0（UNMEASURED 分支）。待下一慢车道纪元后八格翻绿。
- 实测发现：header-free `.cx` 探针冷编译的产品内推理合计 **1.94 s**（同样的 `.c` 约 0.1 s）——内置 cx 前奏（`include/cx.h`）在 E2/E1/E3 的代价；这是 cx 进化最大的单项性能靶（分相数据见 out/c43-cc-phase-01/stamps-shim/run.cold.stderr）。

## 9. 决策记录：.cx 前奏 E2 代价（2026-10-28，fable 子代理裁定）

数据：header-free `.cx` 冷编译 E2 1.78 s / 总 1.94 s；`include/cx.h` 折入 23 个系统头，E2 吞吐约 0.75 MB/s（线性）。裁定：(A) cc 做前奏级 E2 结果缓存（内容寻址，键 = 预定义宏集 + cx.h + 传递头字节；命中仍核内容；smoke 里保留冷/缓存字节等价探针）；(B) 前奏精简归 grk/cdx，只送测量；(C) 执行器提速不做；通知 grk 而非等待。
实现路线（待选，均为 SID 变更需慢车道纪元）：
- A1 执行器检查点：E2 处理完 `\0cli/includes` 后把机器状态（W 稀疏表、数值内存、blobs、intern、SB、输出及属性）序列化进 runreuse；命中时恢复并从用户源继续。不碰 E2 表；需 run.c 新增状态序列化与"检查点状态"约定，正确性风险在遗漏状态。
- A2 宏摘要 + 预展开前缀：E2 结束态转储活宏为 `#define` 文本（需在宏表新增名/参数文本 blob 字段，F_NAMEBLOB/F_PBLOB，FSZ 23→24+），驱动缓存（前奏 E2 输出文本, 摘要）；命中时 E2 以 `\0cli/preexpanded` 资源整段 SPAN 复制前缀、只处理摘要+用户源。改 E2 表 + facts + 驱动；locations 变体需同时缓存位置记录。
碰撞提示：cdx 的 cx0 工作正在改 E2 对不可变资源的消费（6b0ffc7901），A2 与其同区；A1 在 run.c。先与 cdx 对齐再开工；期间按既定顺序做 MISSING 格的事实生成用例。

## 10. 下一事实生成用例：`e4.gold.peep-1632`（设计，待实现为 tools/peep-cases-gen.cx）

- 事实：`weights/gold/peep.tsv` 1632 键 (a,b,rel)→y；y 分布 keep 1494、drop_a 104、fold_copy 16、to_mov 8、retarget_dest 5、retarget 2、load_to_mov/drop_b/fold_imm 各 1。
- 关系的实现规格（`src/opt.c:770-845`，`pk_*`）：`store64 [M], rX`+`load64 rY, [M]`/`store64 [M], rY` 同槽（M 非 `[r7+…]`）→ same_slot / same_slot_same_reg(x==y)；`imm rK, N`(K≤5) + 三操作数 `OP rD, rS, rK`（rS≠rK，rK 其后死）→ const0/const1/pow2；`imm rK` + `mov rY, rK`（Y≠K，rK 死）→ copy_dead；`mov rY, rX`(Y≤5,Y≠X) + 读 rY 不写 rY 的 simple 指令，rY 其后死 → copy_into；simple 非 store 指令 + `mov`（dest_to_mov）；写 rD(≤5) 且其后死 → a_dead；`jump/jumpz L` + 紧随标签 L → to_next；跳到另一 `jump` → to_jump；其余 none。寄存器"死"= 对后续块粒度活性（ol_dead），两指令片段后接 `ret` 即死。
- 生成：每键一份两指令 tape（`_start:` + 片段 + `ret`），能按上述规格实现的键给精确输入；a/b 类与 rel 不相容的键（如 a=jump 且 rel=same_slot）按"不可实现，y=keep 为定义值"显式计数并单独列出，不冒充观测（同 lex-144 折叠的处理）。
- oracle：keep → 输出 == 输入；drop_a/drop_b → 去掉 a/b；load_to_mov → `load64 rY,[M]`→`mov rY, rX`；其余 34 键（fold_copy/to_mov/retarget*/fold_imm）用矩阵已允许的经典参考（`make ref`，`src/opt.c` O2）产出期望 tape，并在同一批内核对 E4 流输出字节相等。
- 入口：E4 流容器 `streams/unisacc-stream4.com INPUT INPUT INC`（见 tests/stream-e4.cx），材料 pin 走现行七流；新批登记为 `stream-e4-peep`，不改既有批。
