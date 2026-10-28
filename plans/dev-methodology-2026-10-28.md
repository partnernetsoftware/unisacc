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

- 2026-10-28 追加（fable 子代理裁定：A1 暂不启动，先去风险）：产品的 E2 运行核不是 exec/c/core.c——`exec/c/run.c:28-33` 在 UNISA_CORE_BLOB 下排除 core.c，`core_run` 来自 exec/c/asm/run_x86_64.S / run_arm64.S（布局 exec/c/asm/layout.h）；C 版 core_run 仅在由 compiler.c 构建的开发驱动里运行。因此 A1 的检查点要在两套 ISA 的汇编核里加钩子与恢复，属慢车道纪元项，不是"最小改动"。与 cdx 的碰撞风险低：cdx 当前工作集是 exec/parse2 三表、tests/*、tools/ 若干 .cx；6b0ffc7901 只改 cx0 自身对 `\0cli/includes` 的应答，不改 run.c 与 E2 表；A1 文件集（exec/c/run.c、core.c、runreuse.h、asm/run_*.S、layout.h）与之不相交，A2 则会碰 exec/pp 表。
  - 实测（out/c43-cc-probe-a1-01，产品 d65c1474）：`int main(void){return 3;}` 作 .cx：E2 推理 1.44–1.55 s，E1 37 ms，E3 45 ms，`-E` 输出 481,573 B；两份不同名/不同目录/不同内容的文件 `-E` 输出到第 481,545 字节前逐字节相同——前奏展开不依赖源名与内容，前奏边界检查点原则上存在。
  - A1 设计（待汇编核移植时执行，按构造字节等价）：触发于 core_run 循环顶首次窥视用户字节（顶帧 b==s.x 且 mode[q]==0），守卫 F->i==0、主输入未执行动作、xattr 全零、`\0cli/source` 未取用且无其它 blob/intern 等于 src；序列化 q/steps/regs/r/ot/osel/out/err/scratch/stack/memory/blobs/strings/resources 与宿主取用收据（命中时重放以保持 E3 runreuse 收据一致）；键 = 检查点版本 + 包身份（runreuse 已绑）+ PCRC[m] + 阶段/路线 + 除 `\0cli/source` 外全部 `\0cli/*` 资源（defines/undefines/includes/include-dir/nostdinc/quote-dir/fno-trim-libc/werror/object/funit/error-limit），命中时重发触发前每次 core_host_fetch 并逐字节核对；资格同 rr_eligible 去 nsources（RF_STRICT、capture、UNISACC_RUN_CACHE=0 下关闭）；位置 rr_dir()（$UNISACC_E3CACHE 或 $TMPDIR/unisacc-e3cache-UID）文件 pre-<hex>.ckpt。证明：`-E` 冷（UNISACC_RUN_CACHE=0）对命中在 tools/*.cx(75)+tests/preprocess/*.cx(20)+sim 探针上 stdout/stderr/rc 逐字节相等；-D 变更、头字节翻转、包更换必须未命中；tests/phase-timing-stamps.cx RR_TIME 前后。
  - 下一步：仓外导出里做 C 核可行性探测（开发驱动 + 外部包复现产品 -E 字节；触发守卫是否成立、检查点大小、命中路径 E2 时间），结论回写本节为"A1 确认、汇编移植已定范围"或"A1 源绑定无效、转 A2"。
  - 2026-10-28 C 核可行性探测结论：**A1 作为纯执行器检查点无效，判为"源绑定"。** 方法：仓外导出 `/home/devali/repos/unisacc-cc-a1-spike-1e4a6af2ed` 用产品从 exec/c/compiler.c 构建开发驱动（27 s），配 cdx fullname 纪元的 compiler.pkg 对 t.cx 跑 `-E`，输出与产品逐字节相同（481,573 B；C 核 E2 6.4 s 对汇编核 1.5 s）；在 core.c 循环顶加 stderr 追踪（取用键、帧数变化、回到主帧时的机器状态与 x 前缀 FNV）。观测：第 3 步即窥视用户输入首字节（"首次窥视用户字节"的触发在第 1 步就成立，不是前奏边界）；第 32 步前取用 `hdr/cx.h`，之后 E2 把主输入 x 逐轮重写（xn 从 56,992 增到 481,663，2042 次 2→1 帧回落），用户源字节从一开始就在 x 尾部参与每一轮整文扫描——两份不同源（28 B 与 123 B）在首个回落点的 steps/xn/x 前缀/内存条目已全部不同（697783/69832 对 542891/56992），且末态 xattr 几乎全非零（481,638/481,663）。因此不存在一个与用户源无关、可按前奏键命中的执行器状态；A1 的字节等价前提不成立。
  - 真正的杠杆是前奏体量：include/cx.h 17 KB 展开到 ~456 KB（取用 40 个头，含 sys/*、unisa_sha256.h、unisa_safedel.h），E2 步数 3.04×10^7 几乎全花在这里。按需模块（`#pragma unisa use`）把前奏缩到只含所用模块，才是每次 .cx 编译省 ~1.4 s 的路——这正是 cdx/grk 的 cx 结构化工作范围；A2（表级预展开前缀）同样在 E2 表域。cc 不再推进 A1；本条交 cdx/grk 作为结构化的量化依据。
  - 附：045306290b 的 RR_TIME 戳已在 d65c1474 里（tests/phase-timing-stamps.cx 在已装产品上 57/0，含测量分支；out/c43-cc-phase-02），HANDOFF 的"待下一纪元装入"已过时。
## 10. 下一事实生成用例：`e4.gold.peep-1632`（已实现：tools/peep-cases-gen.cx）

- 事实：`weights/gold/peep.tsv` 1632 键 (a,b,rel)→y；y 分布 keep 1494、drop_a 104、fold_copy 16、to_mov 8、retarget_dest 5、retarget 2、load_to_mov/drop_b/fold_imm 各 1。
- 关系的实现规格（`src/opt.c:770-845`，`pk_*`）：`store64 [M], rX`+`load64 rY, [M]`/`store64 [M], rY` 同槽（M 非 `[r7+…]`）→ same_slot / same_slot_same_reg(x==y)；`imm rK, N`(K≤5) + 三操作数 `OP rD, rS, rK`（rS≠rK，rK 其后死）→ const0/const1/pow2；`imm rK` + `mov rY, rK`（Y≠K，rK 死）→ copy_dead；`mov rY, rX`(Y≤5,Y≠X) + 读 rY 不写 rY 的 simple 指令，rY 其后死 → copy_into；simple 非 store 指令 + `mov`（dest_to_mov）；写 rD(≤5) 且其后死 → a_dead；`jump/jumpz L` + 紧随标签 L → to_next；跳到另一 `jump` → to_jump；其余 none。寄存器"死"= 对后续块粒度活性（ol_dead），两指令片段后接 `ret` 即死。
- 生成：每键一份两指令 tape（`_start:` + 片段 + `ret`），能按上述规格实现的键给精确输入；a/b 类与 rel 不相容的键（如 a=jump 且 rel=same_slot）按"不可实现，y=keep 为定义值"显式计数并单独列出，不冒充观测（同 lex-144 折叠的处理）。
- oracle：keep → 输出 == 输入；drop_a/drop_b → 去掉 a/b；load_to_mov → `load64 rY,[M]`→`mov rY, rX`；其余 34 键（fold_copy/to_mov/retarget*/fold_imm）用矩阵已允许的经典参考（`make ref`，`src/opt.c` O2）产出期望 tape，并在同一批内核对 E4 流输出字节相等。
- 入口：E4 流容器 `streams/unisacc-stream4.com INPUT INPUT INC`（见 tests/stream-e4.cx），材料 pin 走现行七流；新批登记为 `stream-e4-peep`，不改既有批。
- 2026-10-28 peep 规格实测（out/c43-cc-peep-01，stream4 容器直跑）：结果寄存器必须在 `ret` 前仍活（用 r0），否则 O2 的死码消除先于窥孔把整对删掉；按规格实现的键与金表一致：load_to_mov（`mov r0, r1`）、const0/const1→to_mov、copy_dead→fold_imm、pow2→keep、同槽 store/store→drop_a、同槽同寄存器 store/load→drop_b。一个待裁定差异：`mov r1, r2 / add64 r0, r1, r3`（金表 copy_into→fold_copy）E4 保持不变。经典参考 `/tmp/ua_ref -S` 不接受 tape 输入（原样回显），34 个非机械类键的 oracle 需手推或经 C 源路线。
- 2026-10-28 全表实跑（`sh ./unisacc.com tools/peep-cases-gen.cx -- . STREAM4 ABS_OUT FIRST COUNT`，8 片各 ≤200 键、每片 <60 s；日志 out/c43-cc-peep-01/gen）：1632 键 = PASS 277 + FAIL 0 + UNREALIZABLE 1339（y=keep 为定义值）+ TABLE_SUSPECT 16 + NOT_RUN 0。所有 y 都是机械 oracle（含 fold_copy/to_mov/retarget*/fold_imm），不再需要经典参考产出期望。
  - 上条"待裁定差异"撤回：`mov r1, r2 / …` 不折叠是因为参考 `ol_scan` 在 `ret` 处视 r0/r1 为活（结果寄存器），r1 永不死；复制目标改用 r3 后 11 个 copy_into 键全部 fold_copy 一致。E4 与参考在此无分歧。
  - retarget（to_jump）：E4 在重定向后同时删除已不可达的落点 `jump`（死码，非窥孔关系）；oracle 期望删除后的形态。
  - TABLE_SUSPECT 16（金表给出非 keep 的 y，但参考分类器对该类对永不产生该关系）：a_dead×{other,none}（load64/imm/mov/alu，8 键：`ol_scan` 把 tape 末尾与 OK_OTHER 行视为读全部寄存器）、copy_into×{load64,imm,jump,jumpz,other}（5 键：b 不读 rY 或非 simple）、to_next×none（2 键）、other.mov.dest_to_mov（1 键）。不可达，无行为影响；金表若按参考收紧应改这 16 行为 keep（经生成器路线）。
  - 待办：登记批 `stream-e4-peep`（tests/batches.tsv 当前由 cdx 暂存，待其提交后再加）。
  - TABLE_SUSPECT 16 行的归属路线（fable 子代理裁定 2026-10-28，仅记录为线索、本窗不改）：`weights/gold/peep.tsv` 由 `unisa/gold.py:peep_label` 经 `gold-export` 生成，并经 `build-weights --from-tsv --check` → `emit-kernel` 进入 kernel/ 与 SID；改 16 个 y 等于改权重字节与内核，需慢车道纪元与表主人（主人/cdx）决定。建议：把 `peep_label` 收紧到参考分类器可达集合。

## 11. 事实生成用例二：`e4.gold.opinfo-73`（已实现：tools/opinfo-cases-gen.cx）

- 事实：`weights/gold/opinfo.tsv` 73 个 op × 三头（simple/acls/bcls）。三头都通过 `weights/gold/peep.tsv` 可观测，因此 oracle 全部是查表，不含手写期望。
- A 测（每 op）：op 写 r2（若写寄存器）+ `add64 r0, r1, r3` + `ret`；期望 = peep(acls, add64, a_dead)（simple 且写寄存器时）或 peep(acls, add64, none)。drop_a → op 行被删；keep → 原样。jump/jumpz/call 用远标签避免 to_next/to_jump；`ret` 作为 op 时仅自身。
- B 测（acls=alu 的三操作数 op）：`imm r2, 0` + `OP r0, r1, r2` + `ret`；期望 = peep(imm, bcls, const0)（to_mov → `mov r0, r1`；keep → 原样）。其余 op 的 bcls 与 acls 同词，记 SAME_AS_A。
- 2026-10-28 实跑（`sh ./unisacc.com tools/opinfo-cases-gen.cx -- . STREAM4 ABS_OUT FIRST COUNT`，两片 <60 s；日志 out/c43-cc-opinfo-01）：87 用例 = PASS 87（其中 drop_a 方向 17、to_mov 方向 7、keep 方向 63）+ FAIL 0 + ORACLE_MISSING 0 + NOT_RUN 0；SAME_AS_A 59。`.ld/.st/.lea`（simple=1 但 acls=other）按金表保持不删，`slt64/sle64/ult64/eq/ne`（acls=alu、bcls=other）A 删 B 保持，`ule64`（simple=0）保持——三头都有区分性观测。
- 待办：登记批 `stream-e4-opinfo`（与 `stream-e4-peep` 同，待 cdx 提交 tests/batches.tsv）。

## 12. 事实生成用例三：`e4.pos.o2-*` 四格（已实现：tools/o2-cases-gen.cx）

- 机械 oracle（2026-10-28 实测）：经典参考 `REF -O0 -S S.c` 给未优化 tape，`REF -O2 -S S.c` 给参考 O2 tape；E4 O2 流容器跑 O0 tape 必须与参考 O2 tape 字节相等。此前"参考不接受 tape 输入"的障碍绕开：输入走 C 源，tape 由参考自己产出。
- 用例：12 个内联 C 程序，四格各 3（basic-blocks：if/else 汇合、循环携带、store 后标签；liveness：死临时量、值携带、未用即覆盖；read-fusion：局部 int/char/两局部读；copy-retarget：复制后用、结果复制、复制链）。每例要求 O0≠O2 且参考 O2 含该格的见证子串（`jumpz`/`jump`/`[r6-`/…），否则记 VACUOUS 不算 PASS。
- 实跑（`sh ./unisacc.com tools/o2-cases-gen.cx -- . REF STREAM4 ABS_OUT`，单次 <60 s；日志 out/c43-cc-o2-01）：12 PASS / 0 FAIL / 0 VACUOUS / 0 NOT_RUN，O2 与 E4 输出逐例同字节（907–1220 B）。
- 限制：本次 REF 用的是 /tmp/ua_ref（2026-10-24 宿主构建，源身份未绑定）；正式换标时 REF 必须由冻结导出的 `make ref` 重建并记录源 SID。liveness/copy-retarget 的见证子串目前仅 `ret`/`add64`（弱），区分性主要靠 O0≠O2 与字节相等；可再细化。
- 待办：登记批 `stream-e4-o2`，四格 MISSING→PASS（同样待 cdx 提交 tests/batches.tsv 与 stream-e4 矩阵）。

## 13. 事实生成用例四：prune 正向九格（已实现：tools/prune-cases-gen.cx）

- 格：`tests/stream-prune-domain-matrix.tsv` 的 prune.pos.{entry-roots, jump-fallthrough, data-address-root, unreachable-cycle, transitive-closure, indirect-call-preserved, host-abi-preserved, unknown-prologue-preserved, reference-differential}（fable 子代理裁定 2026-10-28：该矩阵未被 cdx 暂存，九格同为"有限事实→生成 tape→机械 oracle→容器字节比对"形状）。
- 三方 oracle：(1) 经典 `src/tapeprune.c`+`src/tapescan.h` 原文（去 `#include`）+ 12 行 main，由产品自身编译运行（`sh unisacc.com ABS_OUT/tp-ref.c -- INPUT`），不用宿主 C 编译器、不改经典源；两源 sha256 写入 results.tsv 作 oracle 身份。(2) 生成器按每例声明的单元/边/取址名自行算根闭包（根 `_start/main/__init/__main_ret` 与 `.lea` 目标；无 `_start` 时首代码单元为根），存活单元 + 全部数据语句即期望；`whole` 类（callr/callm、.hostcall/.hostaddr/.librarycall、`.global`/缺标签/重复标签）期望等于输入。(3) stream5 容器（pin 1306de94…，`out/c43-streams-riroot-01`）。PASS 要求三者字节相等；经典≠声明记 ORACLE_SPLIT，闭包例输出==输入记 VACUOUS。
- `oracle-only` 模式先用 tests/stream-prune.cx 的 dead 夹具核经典驱动（期望为该夹具手推字节）。
- 2026-10-28 实跑（`sh ./unisacc.com tools/prune-cases-gen.cx -- . STREAM5 ABS_OUT [oracle-only]`，两次各 <60 s；日志 out/c43-cc-prune-01）：oracle-only PASS；24 例 = PASS 24 / FAIL 0 / ORACLE_SPLIT 0 / VACUOUS 0 / NOT_RUN 0（闭包例 15 个均实际删除 68–208 B，whole 例 9 个输入输出同长）。
- 追加 prune.pos.capacity-preserved（同日实测：60 s 风险不成立，stream5 处理 2.1 MiB 127 ms、经典驱动 0.9 s）：三例合成输入——超 2 MiB（2139627 B）整体保留、超 tp_ROWMAX 行（32775 行）整体保留、刚好低于两者（2070612 B）仍剪除死函数；经典驱动缓冲放大到 4 MiB 以便超限输入被完整读入后按合同原样返回。全部 27 例 PASS，整次 4 s。
- 未做：prune.pos.library-exports（库路线不在材料内）。待办：登记批 `stream-prune-cases` 后十格 MISSING→PASS（该矩阵文件未被 cdx 暂存，批行一出即可换标）。

## 14. 事实生成用例五：properties 十三格（已实现：tools/properties-cases-gen.cx）

- 格（fable 子代理裁定 2026-10-28，`tests/properties-domain-matrix.tsv` 未被 cdx 暂存）：prop.det.{image-repeat-six-targets, no-path-or-time, host-env-ignored, run-repeat}、prop.meta.{tape-roundtrip, O2-fixpoint, e4-idempotent, prune-idempotent, forward-reverse, comment-whitespace, identifier-rename}、prop.bound.{loop-0-1-255-256, int64-arith, div-zero, tape-integer-range}。optimization 矩阵的 closure.o2-idempotence / determinism.repeat-bytes 与 O2-fixpoint / image-repeat 是同一观测，可引用同批证据。
- oracle 全部机械：SHA-256/字节相等或 tests/pending/properties.cx 的字面金值；非空洞守卫（重复编译为独立进程、异目录+TZ/LANG、O0≠O2、prune(t)≠t、变体源字节≠基准）失败记 VACUOUS。分三组各 <60 s（bound 10 s、meta 14 s、det 18 s）：`sh ./unisacc.com tools/properties-cases-gen.cx -- . STREAM4 STREAM5 ABS_OUT det|meta|bound`。
- 2026-10-28 实跑（产品 94b549d1，stream4 9d3119df，stream5 1306de94；日志 out/c43-cc-props-01）：PASS 60（det 14 / meta 28 / bound 18）/ FAIL 0 / VACUOUS 0 / NOT_RUN 0。六目标两次编译 SHA 相同；异目录异 TZ/LANG 相同；O2 tape 回灌 `-O2 -S` 字节不变（解决矩阵"tape 输入 -O 接受未验证"注）；E4(O0)==E4(E4(O0))==产品 `-O2 -S`；注释/空行/冗余括号变体的 lnx/x86_64 镜像字节与基准相同（无位置数据）；`.div/.mod` 除零 rc 136；TP-9 边界 ±2^63/2^64−1 接受、越界与下划线拼写 rc1 且 stderr 含 integer 与行号。
- 发现（记录，不改产品）：(1) `.print` 在 lnx/x86_64 不保留 r1/r2（产品与经典参考 /tmp/ua_ref 一致：`.print r5` 后 `.exit r1` 得 1、`.exit r2` 得 24），spec.md:570 对寄存器保留沉默——int64 用例改为每个 op 前重载操作数；建议 spec 明示 `.print/.write` 的寄存器破坏集。(2) `INT64_MIN .div/.mod −1` 在 lnx/x86_64 镜像 rc 136（x86 idiv 溢出陷阱）；C99 6.5.5 未定义、TP-2（模 2^64）会期望回绕，本用例不断言该格，待 spec 裁定。(3) 十六进制拼写 `0x…` 产品 tape 验域记"not covered"显式拒绝；TP-9 只说"支持的各进制拼写"，用例记 INFO 不计 PASS/FAIL。
- 未做（子代理裁定延后）：prop.det.jobs-invariance 与 prop.meta.single-vs-multi-tu（多 TU 入口待探）、prop.det.stream-replay.lower、prop.bound.frame-256 / data-layout-mod8、prop.meta.random-programs。待办：登记批 `properties-cases` 后换标（同样等 cdx 的 tests/batches.tsv）。

## 15. 批登记提案与 pin 复核（2026-10-28）

- §10–§12 的 E4 实跑原用 out/c43-streams-da4e-01 容器（df480481…，非矩阵 pin）；已在 pin 容器 9d3119df（out/c43-streams-riroot-01）复跑：peep 277 PASS / 0 FAIL / 1339 UNREALIZABLE / 16 TABLE_SUSPECT（out/c43-cc-peep-02），opinfo 87 PASS（out/c43-cc-opinfo-02），o2 12 PASS（out/c43-cc-o2-02）——与 da4e 结果逐项相同。
- `tests/batches.tsv` 本窗由 cdx 暂存，五组共 15 行批登记写在 `plans/batches-cc-proposal.tsv`（peep 8 片、opinfo 2 片、o2 1、prune 1、properties 3 组），cdx 提交后原样并入即可；o2 行需要 run.cx 提供 `{ref}`（冻结导出 `make ref` 产物）占位，目前没有。
- §14 追加组 `tu`（2026-10-28 探后落地）：两 TU（各自 `static int hidden` 7/100）与单 TU 同程序输出同为 507，静态名隔离成立；`UNISA_JOBS=1` 对 `=4` 与默认的 tape/镜像字节相等。关闭 prop.meta.single-vs-multi-tu、prop.det.jobs-invariance；5 PASS / 0 FAIL；批提案增 `properties-cases-tu` 行。properties 合计 65 PASS，十五格。
- §14 追加组 `lower`（2026-10-28）：stream6（pin c89eabb6，与 STREAM5 同目录）对 stream-lower.cx 三夹具各跑两次字节相等且等于夹具字面；prop.bound.frame-256 的 lnx/x86_64 部分（300 B 与 320 B 帧，输出 898 117）执行通过，其余五目标仅编译 rc0，arm64 编码字节显式 UNVERIFIED，该格不能仅凭此换 PASS。12 PASS / 0 FAIL；批提案增 `properties-cases-lower`。关闭 prop.det.stream-replay.lower；properties 合计 77 PASS。
- `lower` 组追加 REF 可选参数（2026-10-28 探测：产品与经典参考 /tmp/ua_ref 对 frame.c 在 lnx/x86_64、lnx/arm64、osx/arm64、win/arm64 的镜像字节相同）：六目标产品镜像逐字节等于参考镜像，作为 arm64 I-22 寻址编码的差分 oracle；13 PASS / 0 FAIL，UNVERIFIED 行仅在无 REF 时出现。prop.bound.frame-256 由此完整覆盖；REF 身份与 §12 同一告诫（正式换标须冻结导出 `make ref`）。
- prop.bound.data-layout-mod8 探后延后：stream6 容器只出 lnx/x86_64 的 `@sym` 布局，产品无六目标 lowering 文本出口；六目标差分需 258 序列 × 6 目标 × 2 编译，远超单次 60 s，须另设计（或接受 stream6 单目标 + 产品镜像差分的降级口径，待裁定）。

## 16. 事实生成用例六：optimization 矩阵 11 格（已实现：tools/opt-corpus-gen.cx）

- 格（fable 子代理裁定 2026-10-28，矩阵未被 cdx 暂存）：corpus.{int64-edges, float-double, calls-abi, switch-dense, pointer-memory, large-frame, multi-tu}、closure.reference-tape、default.native-image、refusal.other-spellings、cross-target.compile-bounds（六旗×五外目标镜像等于参考，强于"容器边界有效"）；determinism.repeat-bytes 与 closure.o2-idempotence 引用 properties det/meta 同一观测。
- oracle：每程序六旗（-O0 -O1 -O -O2 -O3 -Os）× 六目标：lnx/x86_64 镜像执行到手推字面（stdout+退出码），产品镜像逐字节等于经典参考镜像（REF 参数，`make ref` 冻结导出告诫同 §12）；守卫 O0 tape ≠ O2 tape。closure：产品 -O1/-O2 -S tape 等于参考 tape；无旗 -b 镜像等于 -O0 镜像并执行。cli：-Og/-Ofast/-O-1/-O4/-O5/-O1x 产品具名拒绝 "option not migrated" rc1 无镜像。
- 2026-10-28 实跑（产品 94b549d1，REF /tmp/ua_ref；日志 out/c43-cc-opt-01；`corpus-N` 每程序一组 ≈28 s，两程序一组会到 56–60 s 故按程序切片）：9 组 = PASS 328（corpus 7×42、closure 28、cli 6）/ FAIL 0 / VACUOUS 0 / NOT_RUN 0。
- 发现（记录）：经典参考对 -Og/-Ofast/-O-1/-O4/-O5/-O1x 静默接受并产出镜像（rc0、stderr 空），产品具名拒绝；矩阵行要求"具名拒绝或文档化别名"，产品满足，参考宽松——若参考被当 CLI oracle 须先裁定。
- 延后：corpus.cc-differential（行声明宿主 cc 为 oracle，cx-only 下不可做，需改行为"经典参考"）、closure.o1-round-bound（内部不可观测）。批提案增 9 行（optimization-corpus-0..6、-closure、-cli，材料 `ref`）。
- §14 追加组 `random`（2026-10-28，fable 裁定可按行口径做）：种子 LCG 生成 16 个仅含已定义行为的 unsigned 64 位直线程序（回绕、<64 位移、非零常数除模、计数器只读循环、一个辅助调用），生成器自行模拟得到"预录参考输出"；lnx/x86_64 镜像 stdout 与退出码等于模拟值，lnx 镜像等于参考镜像，每第 8 个程序六目标镜像等于参考。守卫：16 个输出互异（最初 24 程序含 `v-v`/`v&v` 退化两例全零，改为操作数不同）。43 PASS / 0 FAIL，35 s。关闭 prop.meta.random-programs；properties 合计 120 PASS、十七格（仅 data-layout-mod8 延后）；批提案增 `properties-cases-random`（材料 `ref`）。

## 17. 参考身份绑定与产品更换复核（2026-10-28）

- §12/§14/§16 的 REF 告诫已闭合：在仓外冻结导出 `/home/devali/repos/unisacc-ref-500d6cf3d8`（`git archive` HEAD 500d6cf3d8 + 拷入当前产品供 export_ref 使用）里跑 `tools/build_ref.sh out/ua_ref.c out/ua_ref`（23 s），得 REF `0ed46192…`，source-flat 身份 `b95cec77…`（out/ua_ref.c.flat.*.source.json）。build_ref 的 safedel 清理在仓外路径被白名单拒绝，仅影响临时文件，不影响产物。
- 产品在本日 13:09 被 cdx 的 fullname 纪元（`unisacc-epoch-fullname-778fc68ebc`，artifact d65c1474…）替换：§10–§16 的早期证据产品为 94b549d1，之后的复跑产品为 d65c1474。
- 用新 REF + 新产品复跑全部参考依赖组：o2 12 PASS（out/c43-cc-o2-03）、properties lower 13 PASS、random 43 PASS（out/c43-cc-props-02）、optimization 9 组 328 PASS（out/c43-cc-opt-02）；全部 0 FAIL，与旧 REF/旧产品结果逐项相同。换标时引用这些目录（REF 与产品身份均在各 results.tsv 的 IDENTITY 行）。

## 18. 事实生成用例七、八：data-layout-mod8 全枚举与 Mach-O/PE 结构（2026-10-28，fable 裁定）

- prop.bound.data-layout-mod8（tools/layout-cases-gen.cx）：fable 裁定全枚举而非抽样——tape 编译仅 ~0.04 s。六类 {b1,b8,s1,s7,s8,z8} 长度 1..3 共 258 序列，按 len12 与 len3-0..5 七片（每片 <40 s）。布局规则移植自 tests/stream-lower-domain.cx layout()（= src/back_lower.c bk_repack，目标无关）：声明序分配（.bss 8 对齐、.str 不对齐）→ 以排序唯一起点切 blob → 非零 blob 先、零 blob 后各按起点序保持 mod 8 余数重排 → `@sym = 256 + 新起点`。三方 oracle：stream6 头部文本等于表、lnx 镜像执行写出 addr(dk)−addr(d0) 字节等于表、六目标产品镜像等于冻结 REF 镜像；表写入 layout-table-SLICE.tsv。实跑（out/c43-cc-layout-01，产品 d65c1474、REF 0ed46192）：258 序列 2064 PASS / 0 FAIL / 0 VACUOUS。
  - 移植教训：末尾 `.bss o 8` 暂存格必须作为真实定义参与 blob 切分（前一 blob 的区间延伸到它的 8 对齐起点），否则 s1.z8.s8 等 3 例的零串位置差 8；第一版漏掉导致 6 行 FAIL，修正后全绿——stream6 与镜像行为与参考规则一致。
- formats Mach-O/PE 八格（tools/formats-macho-pe.cx）：osx/win × x86_64/arm64 × 两夹具 = 8 镜像；oracle 全部按公开布局（loader.h/codesign.h、PE/COFF）手写：四段与权限/区间、载入命令集合（DYLINKER/DYLIB/MAIN/PIE、零符号表 8-NUL strtab、零 dysymtab、仅 bind 的 dyld info、无 UUID）、ad-hoc CodeDirectory（0x20400、SHA-256、4096 页、codeLimit=签名偏移）且每页哈希以 cx_sha256 重算相等；PE32+ 四节对齐/区间/标志、入口在 .text、KERNEL32 导入 INT==IAT（22 项）、IAT 目录、320 字节 load config 与 .data 内 cookie、单条 DIR64 重定位恰指向 cookie 字段；每格带变异控制（翻转页字节、RVA +16）必须被拒。实跑 out/c43-cc-fmt-01：96 PASS / 0 FAIL，12 s。
  - 发现：矩阵行 macho.*.load-commands 写"header slack ≥256"，实测（且 src/back_image.c 显式 `wz(156)`）为 156 字节；工具只断言命令区结束于 __text 之前并记录实测值，矩阵注应改为 156 或说明来源。
- 批提案增 `properties-cases-layout-len12`、`-len3-0..5`（材料 streams;ref）与 `formats-macho-pe`；提案共 39 行。

## 19. 批登记提案的真实运行器验证（2026-10-28）

- 在仓外冻结导出 `/home/devali/repos/unisacc-cc-batches-cba0b16589`（git archive HEAD + 当前产品）里把 `plans/batches-cc-proposal.tsv` 与 `plans/coverage-cc-proposal.tsv` 追加到其 tests/batches.tsv / coverage.tsv，用真实 `tests/run-batch.cx`（占位符展开、材料绑定、55 s 预算、候选 SHA 前后一致、expect 标记行）逐行执行全部 39 行：39/39 `status=PASS`（候选 d65c1474；材料 streams=out/c43-streams-riroot-01、ref=冻结 REF 0ed46192）。日志在该导出的 out/rb/ 下。共享检出未改动。
- 运行器暴露并已修正的登记错误：materials 分隔符须为逗号（原 `streams;ref`）；layer 只接受 contract/product/library/interfaces/negative/properties/platform（formats 行改 platform）；`properties-cases-lower` 的 args 用了 `{ref}` 但 materials 漏列（coverage id 必须存在于 coverage.tsv，故配套 15 行 coverage 提案）；`tools/peep-cases-gen.cx` 原把 16 个 TABLE_SUSPECT 当非零退出，批会 FAIL——现仅 FAIL/NOT_RUN 非零，TABLE_SUSPECT 仍在输出与 results.tsv 中计数（它是金表线索，不是运行失败）。
- 结论：cdx 提交 tests/batches.tsv 后，两份提案可原样并入；run.cx 还需提供 `{ref}` 材料绑定（run-batch 已支持任意 `name=ABS_PATH` 材料，批行 materials 列 `ref` 即可）。
