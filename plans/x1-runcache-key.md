# X1 运行缓存键归一(0.0.71,待合入)

状态:补丁与 17 项检查已写好并实跑通过,尚未合入主线,原因是 srcshape 棘轮。

## 做了什么
- src/runcache.cx 的键由 `当前目录 + 原样参数 + 原样源路径 + 源字节` 改为"归一"形式:当前目录前缀下的绝对路径改写为相对路径进键,收据同样记相对路径,命中时在新目录下重新解析再核 sha;当前目录串本身不再进键。
- 单元或被读到的头文件含 `__FILE__` 时,条目"钉住":改存在旧形式键(含当前目录、原样参数与路径)下,收据记绝对路径;查找先查归一键,没有再查钉住键。这样 `__FILE__` 展开出本根路径的程序在另一个根下不会错误命中。
- 键版本 12 改为 13。

## 实测(候选 = entry-build 构造,2026-11-16)
- tests/runcache-roots.cx 17 项全过:两个根之间命中;改源一字节、改被 use 的模块、换编译器身份(候选副本新 inode)、改 -D 参数各自不命中且新值可见;`__FILE__` 在两个根下各打各的路径、互不命中;每个场景另跑 UNISACC_RUN_CACHE=0 臂,输出相同。已装产品跑同一检查,b-first-hit 失败,说明检查能区分新旧行为。
- 真实工具(tools/status.cx,两个 git archive 完整根):根 A 冷 7947 ms,根 B 首次 138 ms(命中),A 再次 134 ms,B 再次 140 ms。旧键下换根必不命中,每工具新根首跑约 5 至 8 秒。

## 阻塞
- tests/srcshape(src/ 只许变瘦):runcache.cx 670 行 -> 756 行,maxfn 118 -> 123,ifs 124 -> 139,src 总行数 5854 -> 5927。要合入须在 src/ 别处净删至少 73 行,或把归一逻辑改成模型步。R1(prune 判定移出驱动)可能腾出行数;未腾出前不提高基线。

## 合入办法
    git apply plans/patches/x1-runcache-key.patch      # 含 src/runcache.cx 与 tests/runcache-roots.cx
    sh ./unisacc.com tools/entry-build.cx -- $PWD $(readlink -f unisacc.com) /tmp/x1/eb
    rm -rf /tmp/x1/rr && sh ./unisacc.com tests/runcache-roots.cx -- /tmp/x1/rr /tmp/x1/eb/candidate
再登记 runcache-roots 批次(tests/manifests),并在冻结前走慢车道。

## 未验证
- bootstrap-0.0.59 多单元编译、两代不动点、六目标;旧检查 tests/runcache.cx(24 项)与 runcache-units.cx(6 项)在新代码下的复跑结果是上一轮子代理所报(24/24、6/6),本轮未复跑。

## v2:键材料迁入 cx/runtime/rcident.cx,过 srcshape(2026-11-16,基 4126cd07b0)
补丁:plans/patches/x1-runcache-key-v2.patch(真实路径头,三个文件:src/runcache.cx、cx/runtime/rcident.cx、tests/runcache-roots.cx;在 4126cd07b0 的归档树与共享检出上 git apply --check 均通过)。v1 补丁保留作对照,不再合入。

### 设计与行数账
- 采用方案 A(迁出):rc_hex_file、rc_hex_sources 两个纯键材料函数连同 X1 新增的归一、`__FILE__` 扫描、钉住判定,迁入既有模块 cx/runtime/rcident.cx(runcache 与 unitcache 早已 use,不新增模块,不动单元清单),导出 rk_open、rk_norm、rk_file、rk_pinned、rk_sources;当前目录串成为 rcident 的模块私有状态。src/runcache.cx 只留调用,查找处两段探测并成一个 rc_probe(先归一键,缺席才查钉住键)。
- 行数:src/runcache.cx 670 -> 608(基线 670/118/124,三项都不升);src/ 合计 5841 -> 5779(基线 5854);cx/runtime/rcident.cx 28 -> 217。归档树内实跑 tests/srcshape.cx --check:ok,0 项超基线。判定与拒绝条件逐条照搬(文件上限 64 MiB、大小核对、64 KiB 起走 sy_sha256_hex、收据逐项复核 sha 与"仍不存在")。
- 相对 v1 的一处收紧:rk_norm 对 `cwd//x` 形路径不归一(v1 会得到以 / 开头的"相对"路径),并先核长度再比前缀。
- 方案 B(在 src/ 别处净删 60 行重复)未采用:要动别的产品文件的判定路径,波及面大于 A,且与 R1 抢同一批行数;本轮未跑 dup-census。
- 风险:(1) rcident 分配由 hs_alloc 改为 mm_alloc/mm_grow/mm_free(与 unitcache 同惯例),cx_read 的缓冲在 lnx/x86_64 候选上实跑无异常,其余目标未跑;(2) rcident 新增 use sys.env、sys.sha、mem.heap、mem.bytes、text.text,unitcache 与 tests/cases/lower_prefix 的闭包随之变化;(3) build/layer-api/cx.runtime.tsv 与 plans/slice-units.tsv 的 rcident 行需由集成者重算;(4) 键版本 12 -> 13,旧条目全部失效一次。

### 已实跑(候选 = 归档树加补丁后 entry-build,candidate sha 43670d7f,构造 69.6 秒)
- tests/runcache-roots.cx 17 项:failed=0。已装产品跑同一检查:b-first-hit 失败(对照有效)。
- tests/srcshape.cx --check:ok。

### 未验证(不记 PASS)
- 真实工具换根首跑耗时:本轮用 check-pins.cx 与 status.cx 在两个归档根里直接起候选与已装产品,两臂的私有缓存目录都没有落任何条目(热跑也不命中),属计时脚本或归档根环境问题,未定因;因此"改前改后换路径首跑"的毫秒数本轮没有可信数据,沿用上文 v1 的 status.cx 数据仅作参考。
- tests/runcache.cx(24 项)、tests/runcache-units.cx(6 项)、unitcache 各批次、ratchets、cx-layers、module-units、layer-api:未跑。
- bootstrap-0.0.59 多单元编译、两代不动点、六目标、慢车道:未跑。

### 集成者验收清单
(1) git apply plans/patches/x1-runcache-key-v2.patch;(2) sh ./unisacc.com tests/srcshape.cx -- --check,预期 ok 且 runcache.cx 608 行,随后 --write 收紧基线;(3) sh ./unisacc.com tools/ratchets.cx,预期全过(重点 cxlibc、cxapis、cxinclude、coreapi);(4) cx-layers 与 module-units 批次,预期 PASS;(5) tools/derive.cx --check,按提示 --write 重算 layer-api 与 slice-units;(6) entry-build 出候选后跑 tests/runcache-roots.cx(17/0)、tests/runcache.cx(24/0)、tests/runcache-units.cx(6/0)与 unitcache 批次;(7) 用 bootstrap-0.0.59 做多单元编译,确认旧产品能编新的 rcident.cx 与 runcache.cx;(8) 登记 runcache-roots 批次(tools/register.cx);(9) 真实工具两根计时(仓根与冻结根,UNISACC_RR_TIME=1,看 lookup_hit=2);(10) 慢车道:两代 GEN 不动点、六目标、全批。回滚:git revert 合入提交,键版本回到 12。

### v2 实测:换路径首跑(2026-11-16,同一小程序两个路径,各产品私有 UNISACC_RUN_CACHE)

| 产品 | 冷启动 | 同路径热 | 换路径首跑 | 换路径第二次 |
|---|---|---|---|---|
| 已装 0.0.67 | 635 毫秒 | 9 | 632(键含路径,失效重编) | 10 |
| 候选 8d48e018(含 X1 v2) | 644 毫秒 | 6 | 7(命中) | 6 |

换路径首跑约 90 倍。此前 check-pins 的 515 对 4132 毫秒(仓根第二次对冻结根首次)同向。每个新冻结根里数百个不同工具的首次编译因此可免。口径:单个小程序;大工具的绝对节省更大,未逐个测。

## 命中路固定开销:分段实测与第一步(2026-11-16,基 a254011d0a)

口径:归档树 /tmp 下,空 main 加十条 use(time、fs、posix、proc、env、sha、os.osx、sysx.process、bytes、heap,闭包 13 个源文件共约 135 KB,缓存映像 345683 字节,条目文件约 52 万字节),候选直接执行(不经 sh 包装),各候选独立 UNISACC_E3CACHE,每项 15 次,毫秒,load1 约 3 至 7。

定因(strace -f -tt 加 UNISACC_RR_TIME,改前):
(1) 命中路的大头不在 runcache 里,而在它之前的 cxm_plan(cx/compiler/cxmod.cx,生成物,源在 cx/cx0):每次运行都对整个 use 闭包做全量前端规划,strace 下约 100 毫秒、实际约 70 毫秒。它把每个模块源读两遍并各做一次扫描(第一遍 cxm_scan 模式 0 取 use 闭包,其间每个文件先被 realpath 回调以 O_PATH 打开加 readlink 约 17 次,合计 186 次打开;第二遍 cxm_srcsha 加 cxm_scan 模式 1 加 cxm_project 导出接口,再做 usechk、collisions、checkcalls 全程序检查),然后把 16 个 CXIF 逐个整读比对(cxm_put,内容相同不重写)。cxmod.o 属 -O1 六个驱动对象之一。
(2) "闭包源读三遍":cxm_plan 两遍,第三遍是 rk_sources 把全部单元拼成一个 135 KB 的块算键(软件 SHA-256 约 8.5 毫秒)。
(3) "CXIF 读两遍":cxm_put 一遍,第二遍是 rc_lookup_kind 的收据核对(收据即各单元的强制包含,也就是这 16 个 CXIF,逐个 rk_file 重算 sha)。
(4) 52 万字节不是懒装载问题:条目是整份转发 ELF(USE34),整读后对 345683 字节的映像做一次软件 SHA-256(cx_sha256_hex,约 16 MB 每秒,22 毫秒),这是 lookup 29 毫秒里的主项;读文件本身不到 2 毫秒,memfd 写入 0.2 毫秒。mmap 省不了哈希,故不选。

已做(本节提交):cx/runtime/rcident.cx 新增 rk_hex(4 KiB 及以上走 sy_sha256_hex 即内核 AF_ALG,失败回落 cx_sha256_hex),rk_file、rk_sources 的总摘要、src/runcache.cx 存与取两处映像摘要都改走它。两条路径摘要值相同,键版本(ver=13)、条目格式、收据内容都不变,正确性合同不变(仍是内容摘要,没有引入 stat 信任)。src/runcache.cx 行数不变(608)。

| 项 | 改前候选 8d48e018 中位/最小 | 改后候选 ff64a2c8 中位/最小 |
|---|---|---|
| 十条 use 空 main 命中 | 116.4 / 112.3(第二轮 116.9 / 113.2) | 85.5 / 81.2(第二轮 83.9 / 79.9) |
| 空 main 无 use 命中 | 3.3 / 2.9 | 2.2 / 2.1 |
| RR_TIME lookup(十条 use) | 28.8 | 5.8 |

验证(候选 ff64a2c8 实跑):runcache-roots assertions=17 failed=0;runcache assertions=24 failed=0;runcache-units assertions=6 failed=0;freeze-gate boot(bootstrap-0.0.59 多单元)PASS units=3;ratchets 见提交说明。

未做、收益更大的下一步(约 70 毫秒,需动 cx/cx0 生成源与 src/driveplan.cx、src/host.cx,不在本次写域):
(a) 规划备忘:键为(产品身份、cwd、"--" 之前的参数、入口源的 size、dev、ino、mtime、ctime),值为闭包每个源与每个 CXIF 的同一组 stat 身份、规划期所有探测为 ENOENT 的路径(同目录优先的模块候选、cxroot.tsv、cxpack.tsv)、以及最终条目键。全部 stat 相符且缺席路径仍缺席时跳过 cxm_plan 与 rk_sources,直接取条目;任何一项不符、stat 失败、或 mtime 落在备忘写入时刻的同一秒内(可疑)一律回落现行全读路径。前提是 cxm_plan 现在不记录缺席探测,须先给 cxm_rd 的 -1 返回接上 rc_on_file 同类的记录钩子,否则新出现的同名模块不会使备忘失效,这是不能省的正确性条件。
(b) hs_cxm_sha(src/host.cx)改走 rk_hex:cxm_srcsha 与 cxm_outsha 每次对约 135 KB 加 60 KB 做软件摘要,估计省约 10 毫秒,未测。
(c) realpath 回调按路径备忘,去掉 186 次 O_PATH 打开中的重复(估计 1 至 2 毫秒,未测)。
未验证:osx、win、arm64 上 sy_sha256_hex 恒回落软件路径,行为与改前相同但未在这些目标实跑;AF_ALG 不可用的 Linux 内核同理走回落,未构造该环境实测。
