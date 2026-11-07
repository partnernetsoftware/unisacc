
## ① 生产可用定义与外部语料(fable 只读评审,2026-11-07)
1. 三大缺口:c-testsuite 仅跑 5/220(`corpus.c-testsuite` 仍 BLOCKED,无登记材料);A12 证实 8/12 约束违例被静默接受(负例矩阵 63 行中 0 条覆盖);差分预言只有 pp-differential,真实程序(Lua 0/1、SQLite 拒于 structure member capacity)无 gcc/tcc 对照通道。
2. 最便宜实验:对解包后的 220 例单文件用例并行跑默认-run 并与 .expected 比对,出 accept/wrong/首错直方图(<5 分钟);A12 十二例登记为负例矩阵行(期望 rc1+具名诊断)让红可见;同 220 例以 gcc -std=c99 -pedantic-errors 产基线与 unisacc 逐例 diff(hostcc 仅作 oracle 文件)。
3. 0.0.47 前三项:全量 c-testsuite 入仓+期望配对+批次;A12 八个错码关闭;SQLITE-MEMBER(有界状态表容量合同,决定大程序能否进入)。
4. 最可能错的假设:「optimization-corpus 对经典参考即可替代 host cc 差分」——参考与网同源于 gold 规则;内部 42/0 全绿与外部 Lua 0/1、SQLite 拒绝并存,内部一致性不是正确性信号。
(cc 处置:采纳 3 项排序;实验 1、2 派 grk 在 s8b 抽干后做;第 4 点并入「金标循环」评审题。)

## ② 金标循环与独立 oracle(fable 只读评审,2026-11-07)
1. 循环最实质处:tests/nets.cx(网逐键核 gold,头注自认非 C 语义 oracle);seed/gold_*_label.c 与 gold 表同出一源,seedconstructmatrix 的「独立再生」只是同规则再跑;weights/gold/*.tsv 头注的生成入口 unisa/ 已不在仓内,gold 源头不可再生。
2. 独立 oracle 仅 7 个 hostcc 批次(optimization-ccdiff×4、pp-differential、pp-negative、c-negative),占 42 项≤7,全在 lnx/x86_64;其余为同源自洽。
3. 最便宜三处:① 把 A12 十二例喂 tools/c-negative.cx(已有 host cc 框架);② opt-corpus-gen ccdiff 加 tcc 作第二 oracle,并把 c-testsuite 220 例作 ccdiff 新组;③ 随机整型表达式生成器对 gcc -O0 做值差分(唯一不经 gold 覆盖 isel/peep/abi 的路径)。
4. 永远抓不到:gold/网/规则一致地错(如运算符优先级、%/>> 的符号、整型提升)。
(cc 复核:6&2==2、-7%3、-7>>1、-7/2、1+2<<3 等 c46-3 与 gcc 一致;`(uchar200<<24)<0` 两者不同但属有符号溢出 UB,不算缺陷,探针需避开 UB。处置:①② 派 grk 副线;③ 列入 0.0.47 备选。)

## ③ NN 名实与 VM 字节码(fable 只读评审,2026-11-07)
1. 最大误导:README「neural-network-based compiler」+「weights」暗示学习/泛化,实为有界键的确定性查表(gold 表→生成器→一热阈值网)+普通 VM;docs/glossary.md:11 已承认「net」双义。建议措辞:「模型驱动(事实表→生成器→VM 程序)的编译器,权重由表确定性构造,无训练、无泛化」,「NN」只留给 kernel/nets 的 18 张事实网。
2. 成败判据:6×切片减量 ≥ 1×pkg 增量 且 墙时 ≤1.2×;比较 APE 总字节(pkg 压缩、切片 .text 不压缩),不比 .text。m14 仅 8KB,试点只证方法;收益要看 m27/cxmod/m22(~440KB UNBOUNDED)能否迁。
3. 最危险未证明:① OOM/资源错误走 core_host_panic→exit,不可恢复不清理(execute.cx:54-55);② 输入 8× 属性(每字节 64 位)+inputn+1 有符号溢出,大 TU(SQLite 类)撞墙。
4. 2 周试点:rtsharedname(m14/rts_at),不用 asmtext。验收:80 键 gold 全枚举对现 C 逐字段 cmp 全同;-o 全批+13 棘轮 0 回归;6 片 .text 减量/pkg 增量/APE 总字节/E2·stream 墙时各 ≥3 样本(冷热分列),缺项标 UNMEASURED;执行器 OOM 改为具名可恢复错误并逐分配点注入测试通过——这是决策门。
5. 最可能失败:74% 不是可查表的有限状态,而是 UNBOUNDED 42%+MIXED 32%,编成 VM 字节码≈在稀疏哈希内存 VM 上重写编译器(解释开销 10–100×、transition 预算 4e11 无墙钟取消合同、无界键使 gold 全键穷举验证失效),净收益可能为负,信任基从 C 挪到未证明的 VM。
(cc 处置:这对「切片瘦身」是不利证据,但不改冻结需求;采纳措辞建议改 README/glossary(0.0.47 文档项);VM 路线先过「OOM 具名可恢复」决策门再谈迁移。)

## ④⑤ 身份系统统一与信任债(fable 只读评审,2026-11-07)
1. 依赖图:源树 rev → SID/作业身份 → ctx/jobs →(kernel-context 独立分支)→ GEN1/GEN2 包字节 → APE 字节 = pp_product_digest = manifest 根(唯一该有的根)→ {ape-receipt、build.json.ape1、root-identity 块、pins(kind=pin)、materials 指纹、all-waves candidate_sha+batches_sha+coverage_sha} → 批次收据。pins/root-identity/manifest/kernel-context 四红全是最后一层手写/落盘投影没随 APE 重算。
2. 根治:`unisacc.com.manifest.tsv` 作唯一主记录,新增 `tools/identity-derive.cx -- ROOT [--check|--write]` 重算全部派生记录(串起现有四个 --emit/--write 入口),`--check` 登记为棘轮并前置于 commit-own;release-build slice / install-ape --write 末尾强制调用;UNDECLARED_PIN 由该命令自动补行。首步≤3 天,不写新逻辑只做编排。
3. 信任债前三与最便宜实验:① Python 构造协调器——去掉 PATH 中 python 跑 chain-precheck reuse,得真实依赖清单;② host C 引导——用 0.0.43 APE 代替 host cc 做 gen1,看 gen2 sha 是否等;③ Windows/macOS 原生自举未声称——CI 上对 osx/x86_64 跑 gen2==gen3 同款检查,不达则如实标 UNCLAIMED。
4. 最易再漂移点:① all-waves init 后主树被 install-ape/commit-own 改动→ init 用私有快照(cp -a --reflink)并在每波前复核 ROOT/unisacc.com sha==candidate_sha,不等 REFUSE;② release-build slice 后手工对齐记录→ identity-derive --check 进棘轮。
(cc 处置:identity-derive 派 grk(仅工具,可合 main);all-waves 快照+sha 复核一并做;信任债实验①②排 0.0.47。)

## ⑥ 流水线失败模式与诊断质量(fable 只读评审,2026-11-07)
1. 失败模式:静默接受(`x=p` 指针赋整数、重复 case 编出可执行,根因 e3 约束缺位);拒绝但信息差(`reject: not covered: a branch to an undefined label`、`tape domain line 1: unknown opcode`,无文件名/标签名/阶段,根因 elf/lower 入口只做一热域校验);截断(tape.text 无结束标记,TAPE-END 实测 10670 处中 3 处产出可跑崩的可执行文件,127/SIGSEGV;e4/prune/lower 对坏输入 rc0 透传);缺分号、未声明标识符诊断完整,与 gcc 同形。
2. 对 gcc 三大差距:语义约束层空白(类型不匹配/重复 case/void 当值);中后段拒绝无定位;无警告层与多错恢复。
3. 最便宜三改:① A12 十二例+两例登记负例矩阵(~20 行表,不触产品输入);② `reject: not covered: X` 统一带被拒 token 与阶段名(src/ 40–60 行,触产品输入);③ e3 加「指针/整数赋值类别不符」与「同 switch 重复 case 常量」两条约束表行(表 10 行+生成器 30 行,覆盖 A12 最大两类)。
4. TAPE-END 最小方案:tape.text 末尾固定 `.end` 终结行,e3 产出/e4·prune·lower 透传/elf 入口校验,否则 `reject: tape truncated (no .end)`;6 目标产物不变,但所有 e3/e4/prune/lower 网与 tape 材料指纹全变,须重生网并重跑 stages/stream 批次;-S 输出多一行需进 CHANGELOG。
(cc 处置:TAPE-END 有实测崩溃产物(10670 处中 3 处),属 0.0.47 最高优先的「可信」项之一,排在 A12 八错码之后/并行(不同文件域);②③ 并入 A12 修复;① 已由 grk 登记。0.0.46 发布说明的「手工拼 stream 管道」限制保留并补上这组实测。)

## ⑧ 工具/棘轮/批次体量(fable 只读评审,2026-11-07)
1. toolsloc 只有一个 TOTAL 行,度量是全树总行数,新增工具必然「违约→手动上调」,近 15 次提交几乎全是上调,棘轮退化成签到簿。更合适:按文件行上限(≤600;>400 行的 16 个、facts-gen 4239 行单列豁免)+ 新增文件必须带 check-*.cx 与批次登记 + 重复度(相同 use 集合与函数签名 hash 的工具对计数);总量只作报表。
2. 可合并类别:cx-* 54、stream-* 15、crypto-*/core-* 各 13;check-* 209/442=47%,多为「一个 main 调同一份读表逻辑」,可并为「一个 check 驱动+事实表」,保留原名薄入口,不删覆盖。
3. 批次分层:判据=(source 是否属被改域)×(materials 是否 -)×预算;每次提交≈281 批(无材料、≤55s、affected 命中),每日≈175 批有材料绑定可本机构造者+negative/properties,发布=platform/product/六目标证据。batches.tsv 加一列 tier。
4. 全批结构障碍:162 批 NOT_RUN 因 materials≠- 而无 PREPARED 证据;最小改造=`tools/materials-prepare.cx --tier daily` 产出 out/materials.tsv(身份→路径→PREPARED),把「材料缺失」从会话期移到准备期并可缓存命中。(与 M7 合并)
5. 一条命令:register.cx 扩成四步事务(batches 行+toolsloc 按文件登记+prd 速查追加+ratchets --only),任一失败整体回滚;precommit 调它(并入 check-precommit-toolsloc)。
(cc 处置:4 并入 M7(grk);5 并入 grk 已在做的 precommit toolsloc 钩子;1+3 作 M8「工具治理」排 cdx2 在 M4 之后:先加 tier 列与 per-file 上限度量,总量降为报表;2 先出聚类统计报告再谈合并,不删覆盖。)

## ⑨ 切片薄层(FFI/dl/syscall/ABI)封装质量(fable 只读评审,2026-11-07)
1. 接口面:内存4、fd2+posix15、时间9、进程17、dl5、FFI10。最不一致=进程/posix 类:Windows 整类 -ENOSYS,Darwin 与两 Linux 走手写 syscall 号表(cx/sys/posix.cx:14-32 四套常量;proc.cx:273-291 setpgid 五分支;posix.cx:117 dents 记录布局不同)。
2. ABI 混合:表驱动=host bridge guard 链(enc-hostbridge*.tsv,SysV/AAPCS64/Win64/WinARM64 分体生成);散落=include/unisacc_ffi.h:14-22,54-55(aarch64 nfixedargs、x86_64 分支)、cx/sys/ffi.cx:25-31(RTLD_GLOBAL Darwin 8/glibc 0x100 写死)、posix.cx ioctl 号写死。libc99posix.tsv 仅名字映射,不含调用约定。
3. 标注部分如实:Windows -ENOSYS 在头注与 cx/sys/README.md:24;FFI 的 Windows 限制只在 unisacc_ffi.h:133 与 coverage.tsv:351,未入计划;osx 的 sy_dl_addr 走 dlfcn 直取无文档。
4. 最小第一步:把 posix.cx:14-32、proc.cx:273-291、ffi.cx:25-31 的六目标常量合并为 exec/facts/sys-syscalls.tsv(name×六列),由 tools/target-gen.cx(已生成 hostid.cx)生成 cx/sys/sysnums.cx;验收=target-gen --check 退出0、生成物字节等价、ratchets 的 cxplatform 对 cx/sys 只降不升。
5. NN 化后易成瓶颈:sy_ffi_call(ffi.cx:59,依赖私有 libffi dlsym 与 cif 布局;调用约定若进表而 cif 手写,六目标会分叉;Windows 全无);sy_dl_addr(dl.cx:246,静态产品手解 ELF 取 dlopen 地址,host.cx 依赖,osx/win 路径不同)。
(cc 处置:4 作 M10a 的前置 L0——「sys 常量表化」,owner cdx2(L1 去重之后);3 把 Windows ENOSYS/FFI 限制补入 plans 薄层节与发布说明模板;5 两个接口列为 M10b 资源合同门的联审对象。)
(更正,cc 2026-11-07:评审⑦第1/4条「cx/driver/sys 与 cx/sys 重复」不成立,git ls-files -s 显示 cx/driver/sys、cx/driver/mem 为 120000 软链;该试点收益作废,L1 仅保留 trace/net/reaper 移出 L0。教训:评审里「重复」类断言须先用 git ls-files -s / diff -r 复核再排期。)
