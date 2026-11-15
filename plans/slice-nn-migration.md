# 切片逻辑 NN 化迁移清单(M10a)

生成物:`sh ./unisacc.com tools/slice-migration-gen.cx -- --write`;校验 `--check`。输入 `plans/unit-text.tsv`(.text 估量,切片 lnx/x86_64)+ `plans/slice-units.tsv`(源文件、use 依赖、堆/递归事实、显式覆盖)。手改本文件会被 `--check` 拒绝;改事实表再生成。

方向(项目所有者 2026-11-07):六个 ISA 切片全部保留;切片内逻辑迁为 NN(事实表→生成器→网/执行器字节码);切片只留 FFI/dl/host syscall/ABI 薄层。体积下降是结果不是手段。

规则(由 `slice-migration-gen.cx` 固化):薄层 = kind host | 源在 cx/sys、cx/mem、cx/text | cx/driver/{input,output} | 显式覆盖「留切片」。其余:UNBOUNDED→迁VM字节码;FINITE/TRANSFORM→迁表;MIXED/SYS-IO→先拆分后迁。批次:迁表 1(≤10KB 且无 sys use)否则 2;先拆分后迁 2(<10KB、无 sys use、无堆)否则 3;VM 4。风险:无界结构(UNBOUNDED 或堆分配)/递归(自调用)/IO(sys use、SYS-IO 或 host)。

合计:81 单元,.text 1845680 字节。留切片 20 单元/395710 字节;迁表 6 单元/48146 字节;迁VM字节码 13 单元/706889 字节;先拆分后迁 42 单元/694935 字节。批次 1/2/3/4 = 5/14/29/13。

## 1. 薄层边界清单(留切片)

| 单元 | 源文件 | 现 .text | 分类 | 说明 |
|---|---|---:|---|---|
| host.o | `src/host.cx` | 92561 | SYS-IO | - |
| executor.o | `src/executor.cx` | 33978 | UNBOUNDED | 执行器本体,M10b对象 |
| m20.o | `cx/sys/runlaunch.cx` | 33329 | SYS-IO | - |
| hostout.o | `src/hostout.cx` | 30076 | SYS-IO | - |
| m32.o | `cx/core/execute.cx` | 26441 | UNBOUNDED | 执行器本体,M10b对象 |
| m15.o | `cx/sys/io.cx` | 26427 | SYS-IO | - |
| m19.o | `cx/sys/time.cx` | 24906 | SYS-IO | - |
| m10.o | `cx/sys/fs.cx` | 20625 | SYS-IO | - |
| m21.o | `cx/sys/posix.cx` | 14932 | SYS-IO | - |
| hostres.o | `src/hostres.cx` | 14545 | SYS-IO | - |
| m18.o | `cx/sys/dl.cx` | 11719 | SYS-IO | - |
| m1.o | `cx/text/text.cx` | 11158 | TRANSFORM | 运行库原语,薄层自身依赖 |
| m3.o | `cx/sys/env.cx` | 10210 | SYS-IO | - |
| m6.o | `cx/sys/hostid.cx` | 8883 | MIXED | - |
| input.o | `cx/driver/input.cx` | 7180 | MIXED | - |
| m7.o | `cx/mem/heap.cx` | 7099 | UNBOUNDED | 运行库原语,薄层自身依赖 |
| transition.o | `cx/core/transition.cx` | 6581 | UNBOUNDED | 执行器本体,M10b对象 |
| m8.o | `cx/mem/bytes.cx` | 6224 | TRANSFORM | 运行库原语,薄层自身依赖 |
| m17.o | `cx/sys/mem.cx` | 5278 | SYS-IO | - |
| hostnative.o | `src/hostnative.cx` | 3558 | SYS-IO | - |

薄层终态合同:sy_*/mm_*/tx_* 原语签名表 + 六目标实现状态,见 `cx/sys/README.md`;冻结后新增原语只走 L0。`cx/driver/sys`、`cx/driver/mem` 为指向 `cx/sys`、`cx/mem` 的目录软链,没有物理副本;L1 边界收敛(trace/net/reaper 移出 L0)是本清单的前置清理。

## 2. 迁移清单(非薄层)

| 单元 | 现 .text | 分类 | 建议去向 | 依赖(前三) | 风险 | 批次 | 备注 |
|---|---:|---|---|---|---|---:|---|
| m27.o `cx/compiler/asmtext.cx` | 223602 | UNBOUNDED | 迁VM字节码 | 调用:- 被调:driveface.o | 无界结构 | 4 | 非默认路径,不计-o收益 |
| cxmod.o `cx/compiler/cxmod.cx` | 120061 | UNBOUNDED | 迁VM字节码 | 调用:- 被调:m0.o,driveunits.o,drivestream.o(+3) | 无界结构/递归 | 4 | src/cxmod.c为C镜像 |
| m22.o `cx/runtime/source_frame.cx` | 93049 | UNBOUNDED | 迁VM字节码 | 调用:bytes 被调:runexec.o | 无界结构 | 4 | - |
| tapegate.o `cx/compiler/tapegate.cx` | 78680 | UNBOUNDED | 迁VM字节码 | 调用:tapeshape 被调:drivetapebin.o | 无界结构 | 4 | - |
| driveroute.o `src/driveroute.cx` | 69288 | SYS-IO | 先拆分后迁 | 调用:fwdsig,fwdmerge,fwdincrement 被调:- | 无界结构/IO | 3 | - |
| driveface.o `src/driveface.cx` | 61641 | MIXED | 先拆分后迁 | 调用:asmtext,archive 被调:- | 无界结构 | 3 | 非默认路径,不计-o收益 |
| m0.o `src/drive.cx` | 51361 | MIXED | 先拆分后迁 | 调用:text,runcache,env 被调:- | IO | 3 | 同源src/drive.cx两单元 |
| runexec.o `src/runtime/runexec.cx` | 48286 | SYS-IO | 先拆分后迁 | 调用:io,env,posix 被调:executor.o,driveplan.o | 无界结构/IO | 3 | - |
| m2.o `src/runcache.cx` | 43791 | UNBOUNDED | 迁VM字节码 | 调用:fs,env,sha 被调:m0.o,driverun.o,drive.o | 无界结构/IO | 4 | - |
| stages.o `src/stages.cx` | 39347 | MIXED | 先拆分后迁 | 调用:routespan,rtsharedname 被调:driveunits.o,drivestream.o,driverun.o | 无界结构 | 3 | - |
| driveunits.o `src/driveunits.cx` | 39305 | MIXED | 先拆分后迁 | 调用:input,entrybind,stages 被调:m0.o,drive.o | 无界结构 | 3 | - |
| drivelink.o `src/drivelink.cx` | 35019 | MIXED | 先拆分后迁 | 调用:rtsharedname,io 被调:m0.o,drive.o | 无界结构/IO | 3 | - |
| m5.o `cx/compiler/tapebin.cx` | 30660 | UNBOUNDED | 迁VM字节码 | 调用:tapeshape 被调:m0.o,drive.o | 无界结构 | 4 | - |
| m29.o `cx/compiler/fwdsig.cx` | 30040 | MIXED | 先拆分后迁 | 调用:- 被调:driveroute.o,drivestream.o | 无界结构 | 3 | - |
| tapelink.o `cx/compiler/tapelink.cx` | 29890 | UNBOUNDED | 迁VM字节码 | 调用:- 被调:- | 无界结构 | 4 | - |
| modeldecode.o `cx/runtime/model_decode.cx` | 25496 | UNBOUNDED | 迁VM字节码 | 调用:model_lifetime 被调:- | 无界结构 | 4 | - |
| codec.o `cx/runtime/codec.cx` | 21634 | UNBOUNDED | 迁VM字节码 | 调用:- 被调:rescache.o | 无界结构 | 4 | - |
| m30.o `cx/compiler/fwdmerge.cx` | 20805 | MIXED | 先拆分后迁 | 调用:- 被调:driveroute.o | 低 | 3 | - |
| cli.o `cx/driver/cli.cx` | 20203 | MIXED | 先拆分后迁 | 调用:- 被调:- | 低 | 3 | - |
| deps.o `cx/driver/deps.cx` | 19109 | MIXED | 先拆分后迁 | 调用:- 被调:- | 低 | 3 | - |
| m35.o `cx/runtime/package_decode.cx` | 18075 | UNBOUNDED | 迁VM字节码 | 调用:package_lifetime,bytes 被调:packmodel.o | 无界结构 | 4 | - |
| drivestream.o `src/drivestream.cx` | 17788 | MIXED | 先拆分后迁 | 调用:input,stages,attribute_frame 被调:- | 无界结构/IO | 3 | - |
| entrybind.o `src/entrybind.cx` | 14188 | MIXED | 先拆分后迁 | 调用:- 被调:driveunits.o,driveargs.o | 低 | 3 | - |
| args.o `cx/driver/args.cx` | 13796 | MIXED | 先拆分后迁 | 调用:- 被调:- | 无界结构 | 3 | - |
| m28.o `cx/driver/archive.cx` | 13750 | MIXED | 先拆分后迁 | 调用:- 被调:driveface.o | 低 | 3 | - |
| drivetapebin.o `src/drivetapebin.cx` | 13582 | MIXED | 先拆分后迁 | 调用:tapegate,path 被调:m0.o,drive.o | 低 | 3 | - |
| route.o `cx/driver/route.cx` | 13399 | FINITE | 迁表 | 调用:- 被调:- | 低 | 2 | - |
| fwdstub.o `cx/compiler/fwdstub.cx` | 12208 | MIXED | 先拆分后迁 | 调用:libc99bind 被调:driveunits.o | 低 | 3 | - |
| m34.o `cx/runtime/packagefooter.cx` | 11620 | MIXED | 先拆分后迁 | 调用:- 被调:packmodel.o | 低 | 3 | - |
| m12.o `cx/runtime/rcident.cx` | 11385 | MIXED | 先拆分后迁 | 调用:fs 被调:m2.o | IO | 3 | - |
| m25.o `cx/compiler/libc99bind.cx` | 10717 | MIXED | 先拆分后迁 | 调用:- 被调:fwdstub.o | 低 | 3 | - |
| packmodel.o `src/packmodel.cx` | 10613 | UNBOUNDED | 迁VM字节码 | 调用:packagefooter,package_decode,package_lifetime 被调:- | 无界结构 | 4 | - |
| attributeframe.o `src/runtime/attribute_frame.cx` | 10060 | MIXED | 先拆分后迁 | 调用:- 被调:driveunits.o,drivestream.o | 无界结构 | 3 | - |
| m31.o `cx/compiler/fwdincrement.cx` | 10002 | MIXED | 先拆分后迁 | 调用:- 被调:driveroute.o | 低 | 3 | - |
| driverun.o `src/driverun.cx` | 9868 | MIXED | 先拆分后迁 | 调用:host,runcache,stages 被调:m0.o,drive.o | 无界结构/IO | 3 | - |
| m26.o `cx/runtime/modelcache.cx` | 9069 | MIXED | 先拆分后迁 | 调用:env,fs,sha 被调:- | IO | 3 | - |
| m13.o `cx/compiler/tapeshape.cx` | 8543 | FINITE | 迁表 | 调用:- 被调:tapegate.o,m5.o | 低 | 1 | - |
| bindstage.o `src/compiler/bindstage.cx` | 8438 | MIXED | 先拆分后迁 | 调用:- 被调:- | 低 | 2 | - |
| m14.o `cx/compiler/rtsharedname.cx` | 8420 | FINITE | 迁表 | 调用:- 被调:stages.o,drivelink.o | 低 | 1 | - |
| m33.o `cx/runtime/model_lifetime.cx` | 8404 | MIXED | 先拆分后迁 | 调用:- 被调:modeldecode.o | 低 | 2 | - |
| driveplan.o `src/driveplan.cx` | 7548 | MIXED | 先拆分后迁 | 调用:cxmod,host,runexec 被调:m0.o,drive.o | 低 | 2 | - |
| modelnumber.o `cx/runtime/model_number.cx` | 7338 | FINITE | 迁表 | 调用:- 被调:- | 低 | 1 | - |
| rescache.o `cx/runtime/rescache.cx` | 7228 | MIXED | 先拆分后迁 | 调用:heap,bytes,codec 被调:- | 无界结构 | 3 | - |
| m11.o `cx/sys/sha.cx` | 6979 | TRANSFORM | 迁VM字节码 | 调用:- 被调:m2.o,m26.o | 低 | 4 | SHA-256轮函数,差分oracle |
| m4.o `cx/runtime/holdframe.cx` | 6920 | MIXED | 先拆分后迁 | 调用:heap,env,fs 被调:driveroute.o,m0.o,driveunits.o(+2) | 无界结构/IO | 3 | - |
| driveargs.o `src/driveargs.cx` | 6374 | FINITE | 迁表 | 调用:entrybind,cxmod 被调:m0.o,drive.o | 低 | 1 | - |
| compat.o `cx/driver/compat.cx` | 6186 | MIXED | 先拆分后迁 | 调用:- 被调:- | 低 | 2 | - |
| routestep.o `src/runtime/routestep.cx` | 5776 | MIXED | 先拆分后迁 | 调用:modelcache 被调:- | 低 | 2 | - |
| m9.o `cx/runtime/package_lifetime.cx` | 5553 | MIXED | 先拆分后迁 | 调用:bytes 被调:m35.o,packmodel.o,rescache.o | 低 | 2 | - |
| memlayout.o `cx/runtime/memlayout.cx` | 5396 | MIXED | 先拆分后迁 | 调用:- 被调:driverun.o | 低 | 2 | - |
| m16.o `cx/compiler/frames.cx` | 5166 | MIXED | 先拆分后迁 | 调用:heap,bytes 被调:driveunits.o,drivestream.o | 无界结构 | 3 | - |
| path.o `cx/driver/path.cx` | 5112 | SYS-IO | 先拆分后迁 | 调用:- 被调:drivetapebin.o | IO | 2 | - |
| drive.o `src/drive.cx` | 5031 | MIXED | 先拆分后迁 | 调用:text,runcache,env 被调:- | IO | 3 | 同源src/drive.cx两单元 |
| stagehandoff.o `src/runtime/stagehandoff.cx` | 4925 | MIXED | 先拆分后迁 | 调用:- 被调:- | 低 | 2 | - |
| m24.o `cx/runtime/routespan.cx` | 4664 | MIXED | 先拆分后迁 | 调用:- 被调:stages.o | 低 | 2 | - |
| m23.o `cx/runtime/profile.cx` | 4407 | MIXED | 先拆分后迁 | 调用:time,env,io 被调:runexec.o | IO | 3 | 非默认路径,不计-o收益 |
| driveutil.o `src/driveutil.cx` | 4371 | MIXED | 先拆分后迁 | 调用:text,holdframe 被调:m0.o,driveunits.o,drivestream.o(+2) | 低 | 2 | - |
| modelload.o `cx/runtime/modelload.cx` | 4359 | UNBOUNDED | 迁VM字节码 | 调用:- 被调:- | 无界结构 | 4 | - |
| m36.o `cx/compiler/cellname.cx` | 4072 | FINITE | 迁表 | 调用:- 被调:drivestream.o | 低 | 1 | - |
| bind.o `src/bind.cx` | 3763 | MIXED | 先拆分后迁 | 调用:- 被调:- | 低 | 2 | - |
| entryexec.o `src/entryexec.cx` | 3610 | MIXED | 先拆分后迁 | 调用:- 被调:- | 低 | 2 | - |

## 3. 前 10 个最适合先迁的单元(收益/风险)

评分 = .text ÷ (1 + 2×风险项数),仅计「迁表」「先拆分后迁」且批次 ≤2;m14 为 M10c 试点钉首位。

| # | 单元 | .text | 去向 | 风险 | 最小验收 |
|---|---|---:|---|---|---|
| 1 | m14.o `cx/compiler/rtsharedname.cx` | 8420 | 迁表 | 低 | 金标全枚举:键域由事实表定义,网输出与 C 实现逐字段 cmp 全同 |
| 2 | route.o `cx/driver/route.cx` | 13399 | 迁表 | 低 | 金标全枚举:键域由事实表定义,网输出与 C 实现逐字段 cmp 全同 |
| 3 | m13.o `cx/compiler/tapeshape.cx` | 8543 | 迁表 | 低 | 金标全枚举:键域由事实表定义,网输出与 C 实现逐字段 cmp 全同 |
| 4 | bindstage.o `src/compiler/bindstage.cx` | 8438 | 先拆分后迁 | 低 | 先拆分:IO 壳留切片、纯逻辑子程序差分(语料+边界),壳/逻辑接口以表固定 |
| 5 | m33.o `cx/runtime/model_lifetime.cx` | 8404 | 先拆分后迁 | 低 | 先拆分:IO 壳留切片、纯逻辑子程序差分(语料+边界),壳/逻辑接口以表固定 |
| 6 | driveplan.o `src/driveplan.cx` | 7548 | 先拆分后迁 | 低 | 先拆分:IO 壳留切片、纯逻辑子程序差分(语料+边界),壳/逻辑接口以表固定 |
| 7 | modelnumber.o `cx/runtime/model_number.cx` | 7338 | 迁表 | 低 | 金标全枚举:键域由事实表定义,网输出与 C 实现逐字段 cmp 全同 |
| 8 | driveargs.o `src/driveargs.cx` | 6374 | 迁表 | 低 | 金标全枚举:键域由事实表定义,网输出与 C 实现逐字段 cmp 全同 |
| 9 | compat.o `cx/driver/compat.cx` | 6186 | 先拆分后迁 | 低 | 先拆分:IO 壳留切片、纯逻辑子程序差分(语料+边界),壳/逻辑接口以表固定 |
| 10 | routestep.o `src/runtime/routestep.cx` | 5776 | 先拆分后迁 | 低 | 先拆分:IO 壳留切片、纯逻辑子程序差分(语料+边界),壳/逻辑接口以表固定 |

## 4. 需要新建的生成器/执行器能力(缺口)

- 表→网生成器通用化:现有 `*-gen.cx` 每张表一个生成器;需要一个「键域枚举 + 输出列」的通用一热阈值网生成器(带 `--check`),FINITE 单元(route/tapeshape/rtsharedname/cellname/model_number/driveargs)共用。
- 差分 oracle 驱动:对 TRANSFORM/MIXED 单元,批量把语料喂给 C 实现与网/VM 程序并逐字节 cmp,输出逐例收据(现无,评审②要求独立 oracle)。
- CX→执行器字节码编译路线(VM):UNBOUNDED 单元(asmtext/cxmod/source_frame/tapegate/tapelink/runcache 等)需要把 CX 子集编成执行器程序;缺:稀疏内存上的结构体/数组寻址、调用约定、字符串原语映射到 tx_*。M10e 只出备忘与一个有界子程序实测。
- 执行器资源合同(见 §5):具名可恢复 OOM、输入长度/属性乘加溢出拒绝、动作/字节预算与取消点。
- 壳/逻辑拆分接口表:MIXED 单元拆成 IO 壳(留切片)与纯子程序,接口(参数/返回/所有权)以事实表固定并由生成器出两侧桩。
- 度量:每单元迁移收据需 6 片 .text 减量、pkg 增量、APE 总字节、E2·stream 墙时冷/热各 ≥3 样本;缺项标 UNMEASURED(现 tools 无逐单元收据生成器)。

## 5. 与 M10b(执行器资源合同门)的依赖

- 批次 1(迁表,FINITE/小 TRANSFORM)不依赖 M10b:网为有界一热查表,不触发执行器动态分配增长;但验收仍要跑在现执行器上,OOM 退出为 panic 时失败不可区分,收据须记「执行器合同未门禁」。
- 批次 2/3(先拆分后迁)部分依赖:子程序若引入输入相关增长(输出/属性),须等 M10b 的输出/属性预算与扩容失败合同。
- 批次 4(迁VM字节码)硬依赖 M10b:`plans/v0.0.47-vm-resource-contract.md` 决策门 1-3 未过前不把大状态编译逻辑搬进 VM(OOM 具名可恢复、活跃分配归零、逐分配点失败注入全 PASS、输入长度边界拒绝、步数/取消合同)。
- 留切片的 executor/execute/transition 本身是 M10b 的修改对象,不进迁移清单。

## 6. 登记建议(共享表由集成者改)

`tests/batches.tsv` 建议行:

```
slice-migration-gen	IMPLEMENTED	interfaces	tools/check-slice-migration-gen.cx	SOURCE	lnx/x86_64	55	-	tools/check-slice-migration-gen.cx	-	interfaces.slice-migration-gen	rc0:CHECK_SLICE_MIGRATION_GEN	product
```

`tests/coverage.tsv` 建议行:`interfaces.slice-migration-gen	interfaces	IMPLEMENTED_SEPARATE_BATCH	M10a migration list generator --check against unit-text/slice-units	plans/unit-text.tsv;plans/slice-units.tsv;tools/slice-migration-gen.cx;plans/slice-nn-migration.md`。`tests/toolsloc.tsv` TOTAL 需 +264(gen 227 + check 37),由 cc 授权。
