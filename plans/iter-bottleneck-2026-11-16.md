# 高速迭代三瓶颈:设计与实测(2026-11-16)

状态:第 2 节已落地两项(tools/cx-dev.cx,各带改前改后实测与 check-cx-dev 实跑);第 1、3 节只有取证与设计,代码未动。数字来源逐项标注;标"推算"的不是实测,标"任务书"的本轮未复测。

## 0 按收益排序

| 序 | 项 | 状态 | 实测或推算 |
|---|---|---|---|
| 1 | s9 map-register 合批(第 3 节) | 设计,未落地 | 32 相每相内层固定 17.3 至 17.7 秒(历史报告实读),合批后推算省约 9 分钟 |
| 2 | cx-dev 按配方缓存键(2.4 第 1 项) | 已落地 | 夹具第二次重建 10761 到 3409 毫秒,built 3 到 1 |
| 3 | cx-dev 基线网共享缓存(2.4 第 2 项) | 已落地 | --close 后重建 20874 到 13878 毫秒,built 6 到 3 |
| 4 | 产品 tmpfile 命名修复后去构造器锁(2.3 第 2 点) | 设计 | 13 张网串行 175 秒,推算降到约 60 秒 |
| 5 | 检查取产品的五个集中点加 UNISACC_PRODUCT,cx-dev 基座增量(第 1 节) | 设计 | 937 个文件不必逐个改 |
| 6 | lint 与 tbl 并行、gen 输出二级键 | 设计 | 推算各省约 30 秒与最多 43% |

## 1 检查与批次选用 cx-dev 作为产品

### 1.1 引用位置统计(git grep,tools 与 tests 下的 .cx,含 cxlib)

| 形态 | 文件数 | 出现次数 | 说明 |
|---|---|---|---|
| 含 unisacc.com 的 .cx 合计 | 937 | 2887 | 含文件头用法注释 |
| 字面 `./unisacc.com`(靠 cwd) | 722 | 1759 | 多数是头注释与 `sh ./unisacc.com` 命令串 |
| 裸 `"unisacc.com"`(与 root 拼接) | 236 | 370 | as_join、pj、path 帮手 |
| 格式串 `%s/unisacc.com`(ROOT 拼接) | 185 | 338 | cx_fmt、snprintf |

其中 check-*.cx 共 450 个文件引用:字面 380、格式串 117、裸名 65(有重叠)。

按出现次数排前的执行类文件(不含 md 与表):check-guard-hook 60、install-ape 33、check-devsvc-core 33、chain-step 33、devsvc-core 30、precommit 27、check-manifest 27、check-cx-lint 27、check-register 25、check-install-ape 23、check-precommit-gates 22、precommit-preview 21、cxlib/productpkg 21、check-productpkg 21、check-commit-own 21、release-ape 18、check-on-change 18、affected 15、release-build 15。

集中点(改一处覆盖很多调用者):
(1) tests/run-batch.cx 第 83、104 行:批次执行器把候选固定为 ROOT/unisacc.com,并在前后取 product_digest。
(2) tests/run.cx 第 42、65 行。
(3) tools/affected.cx 第 841、861、906、940、1035、1113、1137 行。
(4) tools/cxlib/ckrun.cx 第 38 行:检查公用的"写源再 `sh ./unisacc.com` 编跑"帮手。
(5) tools/cxlib/flowfile.cx 第 642、1455 行;tools/cxlib/allsession.cx 第 154、163 行(候选身份 sha256(ROOT/unisacc.com))。

现状:仓内没有任何 UNISACC_PRODUCT 读取点(git grep 为 0)。

### 1.2 方案

结论:不逐个改 937 个文件。分两层。

(1) 位置层(假根,已有):字面 `./unisacc.com` 的 722 个文件只认 cwd,假根已覆盖;ROOT 拼接的也覆盖,前提是调用者把 ROOT 取为假根。假根的缺口是基座:壳转发到 cx-dev,cx-dev 的切片仍是已装的 base.bin。
(2) 基座层(结构性根因,待做):cx-dev 在 BASE_STALE 时不再只报告,而是用已装产品把工作树的 src、cx、include 编成本机切片 base.local,manifest 的 base 行记 origin=built、trust=local,scope 记 LOCAL。这样假根里的 unisacc.com 同时带新网与新基座,检查无须改。缓存键取 src、cx、include、exec/c、kernel、weights/gold 的内容摘要,与本轮已落地的基线网共享缓存同一目录结构。
(3) 环境变量 UNISACC_PRODUCT 只加在五个集中点(run-batch、run、affected、ckrun、flowfile):设了就用它代替 ROOT/unisacc.com 作执行器,候选身份照旧取 ROOT/unisacc.com 并在输出里加 `product=LOCAL` 标注,凡 candidate= 身份相关的批次遇到该变量直接拒绝。
(4) 铁律不变:cx-dev 结果只标 LOCAL,不进候选证据。

未验证:基座增量构造的耗时(切片编译一次多久)没有测;本节没有落地项。

本轮发现的一处相关缺陷:CXDEV_WT 指到只含 exec 与 seed 的夹具时,net-deps 因读不到夹具下的 tools/cxlib/sourcepackage.cx 与 include 而不可用(实测输出 `net-deps: cannot read tools/cxlib/sourcepackage.cx`,补上 tools 后又报 REF_MISSING include/unisa_safedel.h),cx-dev 回落到只用阶段目录规则并每次打 CXDEV_STALE recipe=*。假根与版本工作目录是完整树,不受影响;夹具需带 tools 与 include。

## 2 cx-dev 重建流水线

### 2.1 流水线(tools/cx-dev.cx reload、build、run_jobs)

每张受影响的本机配方两份作业(wt 与 base),每份四步串行:gen -> table-lint -> tbl -> net,再 UNINETB1、M 记录、换包。并发度 CXDEV_PAR 默认 2,但 gen、tbl、net 三步持全用户锁 ctor_lock(产品运行库 tmpfile 用固定名 /tmp/unNNN.tmp,见 plans/cx-dev.md 第 6 节第 1 条),实际只有 lint 能与别的作业重叠。

### 2.2 实测分相(毫秒,取自仓内 bundle 的 work/ID-wt/result,即上一次重建各网的记录;13 张 wt 网)

| 配方 | gen | lint | tbl | net |
|---|---|---|---|---|
| errorparse | 36195 | 12577 | 7521 | 568 |
| warnparse | 35867 | 13034 | 7669 | 634 |
| object-lower-x86_64 | 4430 | 3102 | 2842 | 196 |
| lnx-x86_64-lower | 4323 | 3245 | 2689 | 197 |
| warnunits | 3982 | 740 | 870 | 63 |
| warnpp-shared | 3833 | 2412 | 2603 | 229 |
| shared-e2 | 3740 | 2335 | 2494 | 222 |
| object-enc-x86_64 | 2917 | 1299 | 1316 | 96 |
| lnx-x86_64-elf | 2709 | 1126 | 1190 | 82 |
| tokenpp | 791 | 474 | 623 | 75 |
| shared-e4 | 567 | 661 | 628 | 68 |
| shared-prune | 512 | 525 | 572 | 37 |
| shared-o1 | 163 | 158 | 172 | 13 |
| 合计 | 100029 | 41688 | 31189 | 2480 |

串行合计 175.4 秒;gen 占 57%,lint 24%,tbl 18%,net 1%。两张 parse2 网(errorparse、warnparse)占 114.1 秒,即 65%。任务书的"12 张网 137 秒、parse2 264 秒"本轮未复测。

### 2.3 根因与各点

(1) 按网增量(根因,已修):wt 网的缓存键原先用 cdig,即"全部已改非基座文件的内容加已装提交号"的一个摘要,对所有配方相同。工作树相对已装版本只要再多改任何一个文件,所有受影响配方的 wt 网缓存全部失效。当前工作树相对已装提交 e02a8f8099 在 exec、seed 下有 24 个文件不同,net-deps 给出全部本机配方受影响;此后改一张编码表,13 张网全部重建,而真正依赖它的只有 enc 两张。这就是"改一张表重建十几张网"的来源。
(2) 并发:
 (a) ctor_lock 是全用户一把锁,使 gen、tbl、net 全局串行。根因在产品运行库 include/cx-flat.h 的 tmpfile 命名(产品输入,须按"产品输入变更先出补丁给集成者核"走)。修好后去锁,两张 parse2 网可并行,按 6 路推算 175 秒降到约 60 秒(受最长单网 56.9 秒限制)。在此之前把并发交给 flow 没有收益。
 (b) lint 与 tbl 互不依赖(都只读 d.json),lint 不持锁,可在 gen 之后并行;每张网省 min(lint, tbl),13 张合计约 30.4 秒(推算)。
 (c) 接 flow:每张网一个节点,节点键用按配方摘要;属 devsvc 的 local.reload。
(3) 缓存复用:
 (a) 基线网(已修):原先只在 bundle 的 cache/m,--close 与换 bundle 后要重建整套基线。
 (b) 二级键(未做):gen 之后对 d.json 取摘要,命中则跳过 lint、tbl、net;对"改了表但该配方 gen 输出不变"的情形最多省 43%,gen 仍要跑。
 (c) parse2 单步 gen 36 秒,贴近 55 秒步限,负载高时记 DEFERRED;属 0.0.71 生成码提速范围。

### 2.4 已落地

第 1 项:按配方缓存键(提交 3e45021436)。
做法:对每个已改的 exec 文件各起一个 net-deps --touch(并发 fork,单次实测 588 毫秒),加阶段目录规则,得到"文件 -> 配方"映射;每个配方的键只含影响它的已改文件内容加已装提交号。回落到旧的统一摘要的三种情形:某文件的查询失败(该文件算作影响全部配方);逐文件并集与原受影响集合不一致(全部配方用旧摘要);exec 已改文件超过 64 个或 seed 有改动。选网逻辑本身未动,正确性假设与原选网相同(net-deps 的可达关系)。带 `--reload` 或 CXDEV_VERBOSE=1 时多打一行 `CXDEV_KEYS<TAB>per_recipe=0|1<TAB>files=N`。
实测(夹具为已装版本树加 tools 与 include 链接;先把 exec/enc 的 x86-emit-result.tsv 与 gen-manifest.tsv 换成工作树版本并重建,再把 exec/prune/prune-result.tsv 换成工作树版本并重建;两边基线网都已缓存;各 1 次):

| 第二次重建(只多改 prune 一表) | built | gen | lint | tbl | net | total_ms |
|---|---|---|---|---|---|---|
| 改前 | 3 | 6164 | 2962 | 3084 | 214 | 10761 |
| 改后 | 1 | 498 | 531 | 566 | 40 | 3409 |

第一次重建(改 enc 两表)两边都是 built=2,改前 9429、改后 10011 毫秒(多出的是逐文件 net-deps)。两边最终 pkg.bin 用 cmp 比对逐字节相同。
检查:`sh ./unisacc.com tools/check-cx-dev.cx -- --cases a|b|c|d|e` 逐组实跑,断言 6、14、13、7、3,失败 0,各组 33、10、37、8、42 秒。
未测:仓内真实工作树的 13 张网在新键下的首次重建(键格式变了,首次必全部重建一次,约 137 至 175 秒);parse2 类表的改动;按配方键没有专门的断言(check-cx-dev 现有断言不区分 built 数),应补一条"改 A 表不使只依赖 B 表的网重建"。

第 2 项:基线网共享缓存。
做法:基线网(由已装提交号的干净树建出,键含配方、标志、构造器键、提交号)在 bundle 缓存未命中时先从 `$CXDEV_CACHE/m` 或 `~/.cache/unisacc/cx-dev/m` 取,建成后写回;工作树网不共享(check 的 DEFERRED 断言依赖 wt 网未命中)。`CXDEV_CACHE=-` 关闭;共享目录建不出或不可写时静默不用。命中时带 report 打 `CXDEV_SHARED<TAB>baseline_nets=N`。
实测(同上夹具,独立 CXDEV_CACHE,空共享缓存首次重建对照 --close 之后的重建,各 1 次):

| | built | total_ms |
|---|---|---|
| 空共享缓存(等于改前的 --close 后重建) | 6 | 20874 |
| --close 后,基线取自共享缓存 | 3 | 13878 |

最终 pkg.bin 与第 1 项的结果逐字节相同。对仓内工作树(13 张网)推算每次 --close 或换 bundle 省一整套基线,约 175 秒串行量。
已知限制:共享键信任 bundle 内 base-tree 与已装提交号一致(base-tree 由 --build 的 git archive 产生);人为改过 base-tree 的 bundle 会把错的基线网写进共享缓存。补救是 `rm -r ~/.cache/unisacc/cx-dev/m`。
检查:见文末第 4 节本轮实跑清单。

### 2.5 后续顺序

(1) 产品 tmpfile 命名补丁(pid 加 O_EXCL)交集成者,合入并安装后去 ctor_lock,CXDEV_PAR 默认提到核数的一半。
(2) lint 与 tbl 并行;gen 输出二级键。
(3) check-cx-dev 补按配方键与共享基线两条断言。
(4) 基座增量(第 1 节第 2 点)。

## 3 s9 daily 冻结路径的 map-register 串行

### 3.1 已读事实

(1) tools/s9-nightly.cx:相位名由 init_name(n) 给出,前 9 个固定,其后 n 对应 map-register-(n-8);每相一份收据 init_state/NAME.done。init_one(n) 开头对全部 j 小于 n 的相逐个核收据,缺一个就拒(`prerequisite NAME`);所以 map-register-N 必须在 1 至 N-1 之后,这是收据链强加的顺序。
(2) 每相在 init_one 里算两次 init_identity(其内 as_identity 带 sid),每相预算 init_left 为 50 秒。
(3) 登记本体:init_materials 取 RUN.materials.tsv 第 N 行的材料名,调执行器 `prepare ROOT RUN material NAME`;执行器内部转到 tools/all-waves.cx 的 material_run 这一步是按命令名与块格式推断的,执行器源码本轮未读。
(4) material_run 的真实共享状态:持会话写锁,做一次身份核对,然后对每个 NAME 量 fp、content、fp,向 session.tsv 追加一个带序号的链式块(seq 为 m.seq+1 起)。链式追加必须串行,这是真实依赖;各材料的测量互不依赖。
(5) all-waves 已有批量入口 `materials ROOT SESSION NAME...`:同一把锁、一次身份核对、按给定顺序逐个追加。批量原语是现成的,s9-nightly 没有用它。

### 3.2 实测(读历史报告,不是本轮重跑)

/tmp 与 out 下找到一轮 32 份 `s9-nightly-*-map-register-N.md`,其中 30 份末行可读:内层登记步墙钟 17336 至 17713 毫秒,各材料之间几乎不变。结论:耗时是固定开销(执行器启动与身份核对),不是材料内容哈希。任务书的每相约 26 秒与此相符(外层 s9-nightly 自己的两次身份与启动约占其余 8 秒,此分项未测)。32 相合计约 14 分钟。

未测:17.5 秒在执行器内的细分(启动编译、as_identity、registry 装载各多少)。

### 3.3 方案(最小改动)

(1) s9-nightly 加相位 `map-register-A..B`(或 `map-register-all`):一个进程内,先核 A 之前全部收据,做一次身份核对,调一次执行器 `prepare ROOT RUN materials NAME_A ... NAME_B`(执行器若只有单名入口则补多名转发),完成后按 material_run 输出的逐条 MATERIAL 行为每个 N 各写各的 intent、outputs 证据与收据,相后再核一次身份。单相入口 map-register-N 保留不变。
(2) 合同不变的部分:链式块仍一材料一块、序号连续、顺序与逐相相同;每个 N 的收据与证据文件形状不变;身份在批前批后各核一次,批内任何一材料 fp 前后不一致照旧整批拒绝(all-waves 已如此)。变的只有一处:批内两材料之间不再重算身份,需在收据里注明批号,由集成者确认这不算放宽。
(3) 推算收益:32 乘 26 秒约 832 秒,降到一次固定开销约 26 秒加 32 份哈希;受 50 秒相预算约束时按每批 8 至 16 个切分,合计约 1 至 2 分钟。
(4) 再进一步(可选):材料测量并发(只读),追加仍串行;在固定开销去掉之后才值得做。
(5) steps:现成的 `--max-steps N` 已支持一次多步,先用它减少调用次数,不需要改代码;单次超过 55 秒的经 devsvc x --bg。

未落地原因:涉及 s9-nightly 的收据与证据写法、执行器多名转发与对应检查,本轮剩余时间不足以改完并实跑检查;未验证的不提交。

## 4 本轮实跑清单

- git grep 分类统计(第 1 节各数)。
- 仓内 bundle `cx-dev --status`:scope=LOCAL-PARTIAL,local_members=6,stale=0,base_stale=4,54 毫秒。
- bundle 内 net-deps:24 个已改文件一次 632 毫秒,单文件 588 毫秒。
- 读 bundle 的 work/ID-wt/result 得分相表。
- 第 2.4 节两项的改前改后重建计时与 pkg.bin 逐字节比对。
- check-cx-dev 五组:第 1 项之后实跑失败 0(43 项断言);第 2 项之后再实跑失败 0(43 项断言),各组 32、11、52、6、28 秒(load1 约 12,共享缓存首次写入);把 mkq 里的裸 memcpy 换成带界帮手(cxtrunc 棘轮)后第三次实跑失败 0,各组 33、10、24、6、21 秒(load1 约 5.6,基线已在共享缓存)。ratchets 实跑 ok=15 fail=0。
- 未跑:s9 任一相位、ratchets、derive、affected、parse2 类重建、仓内真实工作树的整套重建。
