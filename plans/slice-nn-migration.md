# 切片逻辑 NN 化迁移清单(M10a)

生成物:`sh ./unisacc.com tools/slice-migration-gen.cx -- --write`;校验 `--check`。输入 `plans/unit-text.tsv`(.text 估量,切片 lnx/x86_64)+ `plans/slice-units.tsv`(源文件、use 依赖、堆/递归事实、显式覆盖)。手改本文件会被 `--check` 拒绝;改事实表再生成。

方向(项目所有者 2026-11-07):六个 ISA 切片全部保留;切片内逻辑迁为 NN(事实表→生成器→网/执行器字节码);切片只留 FFI/dl/host syscall/ABI 薄层。体积下降是结果不是手段。

规则(由 `slice-migration-gen.cx` 固化):薄层 = kind host | 源在 cx/sys、cx/mem、cx/text | cx/driver/{input,output} | 显式覆盖「留切片」。其余:UNBOUNDED→迁VM字节码;FINITE/TRANSFORM→迁表;MIXED/SYS-IO→先拆分后迁。批次:迁表 1(≤10KB 且无 sys use)否则 2;先拆分后迁 2(<10KB、无 sys use、无堆)否则 3;VM 4。风险:无界结构(UNBOUNDED 或堆分配)/递归(自调用)/IO(sys use、SYS-IO 或 host)。

合计:99 单元,.text 2441257 字节。留切片 27 单元/515445 字节;迁表 6 单元/48602 字节;迁VM字节码 24 单元/1039516 字节;先拆分后迁 42 单元/837694 字节。批次 1/2/3/4 = 5/16/27/24。

对照(2026-11-07 旧基线 `88f265cc17` `plans/unit-text.tsv`):81 单元/.text 1845680 字节 -> 现 99 单元/.text 2441257 字节(净增 +18 单元/+595577 字节)。

## 1. 薄层边界清单(留切片)

| 单元 | 源文件 | 现 .text | 分类 | 说明 |
|---|---|---:|---|---|
| host.o | `src/host.cx` | 87863 | SYS-IO | - |
| m32.o | `cx/sys/posix.cx` | 42704 | SYS-IO | - |
| executor.o | `src/executor.cx` | 37298 | UNBOUNDED | 执行器本体,M10b对象 |
| m39.o | `cx/sys/runlaunch.cx` | 36523 | SYS-IO | - |
| hostout.o | `src/hostout.cx` | 31417 | SYS-IO | - |
| m49.o | `cx/sys/kernel.cx` | 29440 | SYS-IO | - |
| m12.o | `cx/sys/fs.cx` | 29273 | SYS-IO | - |
| m27.o | `cx/sys/proc.cx` | 28852 | SYS-IO | 非默认路径(仅runsplit/unitobj依赖) |
| m19.o | `cx/sys/io.cx` | 26396 | SYS-IO | - |
| m18.o | `cx/sys/time.cx` | 26211 | SYS-IO | - |
| hostres.o | `src/hostres.cx` | 16449 | SYS-IO | - |
| m38.o | `cx/sys/dl.cx` | 11696 | SYS-IO | - |
| m1.o | `cx/text/text.cx` | 11155 | TRANSFORM | 运行库原语,薄层自身依赖 |
| m4.o | `cx/sys/env.cx` | 10797 | SYS-IO | - |
| m31.o | `cx/sys/sysinfo.cx` | 10429 | SYS-IO | - |
| m34.o | `cx/mem/grow.cx` | 9133 | UNBOUNDED | 非默认路径(仅stream.item/pipe依赖) |
| m8.o | `cx/sys/hostid.cx` | 8856 | MIXED | - |
| m2.o | `cx/mem/heap.cx` | 8840 | UNBOUNDED | 运行库原语,薄层自身依赖 |
| m35.o | `cx/mem/slice.cx` | 8628 | UNBOUNDED | 非默认路径(仅stream/routepipe依赖) |
| m30.o | `cx/sys/sig.cx` | 8089 | SYS-IO | - |
| input.o | `cx/driver/input.cx` | 7520 | MIXED | - |
| transition.o | `cx/core/transition.cx` | 6345 | UNBOUNDED | 执行器本体,M10b对象 |
| m10.o | `cx/mem/bytes.cx` | 6071 | TRANSFORM | 运行库原语,薄层自身依赖 |
| m37.o | `cx/sys/mem.cx` | 5194 | SYS-IO | - |
| m43.o | `cx/sys/sysnums.cx` | 3422 | SYS-IO | - |
| m44.o | `cx/mem/buftype.cx` | 3422 | UNBOUNDED | 非默认路径(仅mem.grow/slice依赖) |
| hostnative.o | `src/hostnative.cx` | 3422 | SYS-IO | 非默认路径(空占位单元无符号) |

薄层终态合同:sy_*/mm_*/tx_* 原语签名表 + 六目标实现状态,见 `cx/sys/README.md`;冻结后新增原语只走 L0。`cx/driver/sys`、`cx/driver/mem` 为指向 `cx/sys`、`cx/mem` 的目录软链,没有物理副本;L1 边界收敛(trace/net/reaper 移出 L0)是本清单的前置清理。

## 2. 迁移清单(非薄层)

| 单元 | 现 .text | 分类 | 建议去向 | 依赖(前三) | 风险 | 批次 | 备注 |
|---|---:|---|---|---|---|---:|---|
| m45.o `cx/compiler/asmtext.cx` | 214910 | UNBOUNDED | 迁VM字节码 | 调用:- 被调:driveface.o | 无界结构 | 4 | 非默认路径,不计-o收益 |
| cxmod.o `cx/compiler/cxmod.cx` | 191885 | UNBOUNDED | 迁VM字节码 | 调用:- 被调:m0.o,m3.o,driveunits.o(+4) | 无界结构/递归 | 4 | src/cxmod.c为C镜像 |
| m40.o `cx/runtime/source_frame.cx` | 95075 | UNBOUNDED | 迁VM字节码 | 调用:bytes 被调:runexec.o | 无界结构 | 4 | - |
| tapegate.o `cx/compiler/tapegate.cx` | 75425 | UNBOUNDED | 迁VM字节码 | 调用:tapeshape,tape_frame 被调:m6.o,drivestream.o,drivetapebin.o | 无界结构 | 4 | 非默认(仅.tapebin/--tapebin/stream) |
| driveroute.o `src/driveroute.cx` | 67824 | SYS-IO | 先拆分后迁 | 调用:fwdsig,fwdmerge,fwdincrement 被调:- | 无界结构/IO | 3 | - |
| m0.o `src/drive.cx` | 65213 | MIXED | 先拆分后迁 | 调用:text,heap,runcache 被调:- | IO | 3 | 同源src/drive.cx两单元 |
| runexec.o `src/runtime/runexec.cx` | 59280 | SYS-IO | 先拆分后迁 | 调用:io,env,posix 被调:m16.o,executor.o,driveplan.o | 无界结构/IO | 3 | - |
| driveface.o `src/driveface.cx` | 58564 | MIXED | 先拆分后迁 | 调用:asmtext,archive 被调:- | 无界结构 | 3 | 非默认路径,不计-o收益 |
| tapelink.o `cx/compiler/tapelink.cx` | 49928 | UNBOUNDED | 迁VM字节码 | 调用:- 被调:- | 无界结构 | 4 | 非默认路径(仅-c多单元链接) |
| m14.o `cx/runtime/rcident.cx` | 49555 | MIXED | 先拆分后迁 | 调用:fs,env,sha 被调:m3.o,m16.o | 无界结构/IO | 3 | - |
| m3.o `src/runcache.cx` | 47379 | UNBOUNDED | 迁VM字节码 | 调用:fs,env,sha 被调:m0.o,driverun.o,drive.o | 无界结构/IO | 4 | - |
| driveunits.o `src/driveunits.cx` | 47193 | MIXED | 先拆分后迁 | 调用:input,entrybind,stages 被调:m0.o,drive.o | 低 | 3 | - |
| m16.o `cx/runtime/unitcache.cx` | 46590 | UNBOUNDED | 迁VM字节码 | 调用:env,fs,sha 被调:m3.o,driveunits.o,stages.o(+2) | 无界结构/IO | 4 | - |
| m29.o `cx/driver/objlink.cx` | 45943 | UNBOUNDED | 迁VM字节码 | 调用:fs,heap 被调:m9.o | 无界结构/IO | 4 | 非默认路径(仅UNISACC_RUN_SPLIT) |
| stages.o `src/stages.cx` | 43011 | MIXED | 先拆分后迁 | 调用:routespan,rtsharedname,unitcache 被调:driveroute.o,m0.o,driveunits.o(+3) | 无界结构 | 3 | - |
| drivelink.o `src/drivelink.cx` | 38487 | MIXED | 先拆分后迁 | 调用:rtsharedname,io 被调:m0.o,drive.o | 无界结构/IO | 3 | 非默认路径(仅.o/.a链接与.tape) |
| m28.o `cx/driver/unitobj.cx` | 36846 | UNBOUNDED | 迁VM字节码 | 调用:fs,sha,proc 被调:m9.o | 无界结构/IO | 4 | 非默认路径(仅UNISACC_RUN_SPLIT) |
| m15.o `cx/runtime/modelcache.cx` | 35030 | MIXED | 先拆分后迁 | 调用:env,fs,sha 被调:- | 无界结构/IO | 3 | - |
| m36.o `cx/compiler/fwdsig.cx` | 30616 | MIXED | 先拆分后迁 | 调用:- 被调:driveroute.o,drivestream.o,m24.o | 无界结构 | 3 | - |
| m6.o `cx/compiler/tapebin.cx` | 30188 | UNBOUNDED | 迁VM字节码 | 调用:tapeshape,tapegate,tape_frame 被调:m0.o,drive.o | 无界结构 | 4 | 非默认路径(仅--tapebin -S) |
| m9.o `cx/driver/runsplit.cx` | 28658 | UNBOUNDED | 迁VM字节码 | 调用:fs,env,proc 被调:m0.o,drive.o | 无界结构/IO | 4 | 非默认路径(仅UNISACC_RUN_SPLIT) |
| m47.o `cx/compiler/fwdmerge.cx` | 26560 | MIXED | 先拆分后迁 | 调用:- 被调:driveroute.o,m48.o | 低 | 3 | - |
| m5.o `cx/runtime/holdframe.cx` | 26195 | MIXED | 先拆分后迁 | 调用:heap,env,fs 被调:driveroute.o,m0.o,driveunits.o(+2) | 无界结构/IO | 3 | - |
| modeldecode.o `cx/runtime/model_decode.cx` | 25362 | UNBOUNDED | 迁VM字节码 | 调用:model_lifetime,modelcache 被调:- | 无界结构 | 4 | - |
| drivestream.o `src/drivestream.cx` | 25229 | MIXED | 先拆分后迁 | 调用:input,stages,attribute_frame 被调:- | 无界结构/IO | 3 | 非默认路径(仅stream子命令) |
| cli.o `cx/driver/cli.cx` | 22688 | MIXED | 先拆分后迁 | 调用:- 被调:- | 低 | 3 | - |
| m23.o `cx/stream/pipe.cx` | 21248 | UNBOUNDED | 迁VM字节码 | 调用:item,grow,slice 被调:m7.o | 无界结构 | 4 | 非默认路径(仅routepipe依赖) |
| deps.o `cx/driver/deps.cx` | 20231 | MIXED | 先拆分后迁 | 调用:- 被调:- | 低 | 3 | 非默认路径(仅-c无-o且结果丢弃) |
| codec.o `cx/runtime/codec.cx` | 19780 | UNBOUNDED | 迁VM字节码 | 调用:- 被调:rescache.o | 无界结构 | 4 | - |
| m52.o `cx/runtime/package_decode.cx` | 18391 | UNBOUNDED | 迁VM字节码 | 调用:package_lifetime,bytes 被调:packmodel.o | 无界结构 | 4 | - |
| m24.o `cx/compiler/fwdmark.cx` | 15730 | UNBOUNDED | 迁VM字节码 | 调用:fwdsig 被调:m7.o | 无界结构 | 4 | 非默认路径(仅routepipe依赖) |
| m33.o `cx/runtime/frameparts.cx` | 15401 | UNBOUNDED | 迁VM字节码 | 调用:heap,bytes 被调:m17.o | 无界结构 | 4 | - |
| args.o `cx/driver/args.cx` | 15359 | MIXED | 先拆分后迁 | 调用:- 被调:- | 无界结构 | 3 | - |
| m13.o `cx/sys/sha.cx` | 15334 | TRANSFORM | 迁VM字节码 | 调用:- 被调:m14.o,m3.o,m16.o(+3) | 低 | 4 | SHA-256轮函数,差分oracle |
| drivetapebin.o `src/drivetapebin.cx` | 14392 | MIXED | 先拆分后迁 | 调用:tapegate,path 被调:m0.o,drive.o | 低 | 3 | 非默认路径(仅.tapebin输入) |
| entrybind.o `src/entrybind.cx` | 14273 | MIXED | 先拆分后迁 | 调用:- 被调:driveunits.o,driveargs.o | 低 | 3 | - |
| m46.o `cx/driver/archive.cx` | 13059 | MIXED | 先拆分后迁 | 调用:- 被调:driveface.o | 低 | 3 | 非默认路径(仅ar子命令与.a链接) |
| route.o `cx/driver/route.cx` | 12979 | FINITE | 迁表 | 调用:- 被调:- | 低 | 2 | - |
| packmodel.o `src/packmodel.cx` | 12690 | UNBOUNDED | 迁VM字节码 | 调用:packagefooter,package_decode,package_lifetime 被调:- | 无界结构 | 4 | - |
| fwdstub.o `cx/compiler/fwdstub.cx` | 12354 | MIXED | 先拆分后迁 | 调用:libc99bind 被调:driveunits.o | 低 | 3 | - |
| driverun.o `src/driverun.cx` | 11703 | MIXED | 先拆分后迁 | 调用:host,runcache,stages 被调:m0.o,drive.o | 无界结构/IO | 3 | - |
| m7.o `cx/compiler/routepipe.cx` | 11186 | UNBOUNDED | 迁VM字节码 | 调用:item,pipe,fwdmark 被调:m0.o,drive.o | 无界结构 | 4 | 非默认(仅-c/-funit与.tape/runsplit) |
| m51.o `cx/runtime/packagefooter.cx` | 11165 | MIXED | 先拆分后迁 | 调用:- 被调:packmodel.o | 低 | 3 | - |
| m42.o `cx/compiler/libc99bind.cx` | 10547 | MIXED | 先拆分后迁 | 调用:- 被调:fwdstub.o | 低 | 3 | - |
| driveplan.o `src/driveplan.cx` | 9660 | MIXED | 先拆分后迁 | 调用:cxmod,host,runexec 被调:m0.o,drive.o | 低 | 2 | - |
| m48.o `cx/compiler/fwdincrement.cx` | 9599 | MIXED | 先拆分后迁 | 调用:fwdmerge 被调:driveroute.o | 低 | 2 | - |
| attributeframe.o `src/runtime/attribute_frame.cx` | 9505 | MIXED | 先拆分后迁 | 调用:- 被调:driveunits.o,m16.o,drivestream.o | 低 | 2 | - |
| m25.o `cx/compiler/rtsharedname.cx` | 9405 | FINITE | 迁表 | 调用:- 被调:stages.o,drivelink.o | 低 | 1 | - |
| m21.o `cx/runtime/tape_frame.cx` | 8966 | UNBOUNDED | 迁VM字节码 | 调用:- 被调:tapegate.o,m6.o,drivestream.o | 无界结构 | 4 | 非默认(仅tapegate/tapebin/stream) |
| m20.o `cx/compiler/tapeshape.cx` | 8435 | FINITE | 迁表 | 调用:- 被调:tapegate.o,m6.o | 低 | 1 | 非默认路径(仅tapegate/tapebin依赖) |
| bindstage.o `src/compiler/bindstage.cx` | 8235 | MIXED | 先拆分后迁 | 调用:- 被调:- | 低 | 2 | - |
| rescache.o `cx/runtime/rescache.cx` | 8234 | MIXED | 先拆分后迁 | 调用:heap,bytes,codec 被调:- | 无界结构 | 3 | - |
| m50.o `cx/runtime/model_lifetime.cx` | 7983 | MIXED | 先拆分后迁 | 调用:- 被调:modeldecode.o | 低 | 2 | - |
| m17.o `cx/runtime/frames.cx` | 7254 | MIXED | 先拆分后迁 | 调用:heap,bytes,frameparts 被调:driveunits.o,m5.o,drivestream.o | 无界结构 | 3 | - |
| modelnumber.o `cx/runtime/model_number.cx` | 7106 | FINITE | 迁表 | 调用:- 被调:- | 低 | 1 | - |
| driveargs.o `src/driveargs.cx` | 6701 | FINITE | 迁表 | 调用:entrybind,cxmod 被调:m0.o,drive.o | 低 | 1 | - |
| routestep.o `src/runtime/routestep.cx` | 6381 | MIXED | 先拆分后迁 | 调用:modelcache 被调:- | 低 | 2 | - |
| compat.o `cx/driver/compat.cx` | 5940 | MIXED | 先拆分后迁 | 调用:- 被调:- | 低 | 2 | - |
| m11.o `cx/runtime/package_lifetime.cx` | 5503 | MIXED | 先拆分后迁 | 调用:bytes 被调:m52.o,packmodel.o,rescache.o | 低 | 2 | - |
| memlayout.o `cx/runtime/memlayout.cx` | 5212 | MIXED | 先拆分后迁 | 调用:- 被调:driverun.o | 低 | 2 | - |
| drive.o `src/drive.cx` | 5040 | MIXED | 先拆分后迁 | 调用:text,heap,runcache 被调:- | IO | 3 | 同源src/drive.cx两单元 |
| path.o `cx/driver/path.cx` | 5037 | SYS-IO | 先拆分后迁 | 调用:- 被调:drivetapebin.o | IO | 2 | - |
| stagehandoff.o `src/runtime/stagehandoff.cx` | 4809 | MIXED | 先拆分后迁 | 调用:- 被调:- | 低 | 2 | - |
| m22.o `cx/stream/item.cx` | 4558 | UNBOUNDED | 迁VM字节码 | 调用:grow,slice 被调:m23.o,m7.o | 无界结构 | 4 | 非默认路径(仅routepipe/pipe依赖) |
| m26.o `cx/runtime/routespan.cx` | 4543 | MIXED | 先拆分后迁 | 调用:- 被调:stages.o | 低 | 2 | - |
| m41.o `cx/runtime/profile.cx` | 4538 | MIXED | 先拆分后迁 | 调用:time,env,io 被调:runexec.o | IO | 3 | 非默认路径,不计-o收益 |
| driveutil.o `src/driveutil.cx` | 4444 | MIXED | 先拆分后迁 | 调用:text,holdframe 被调:m0.o,driveunits.o,drivestream.o(+2) | 低 | 2 | - |
| modelload.o `cx/runtime/modelload.cx` | 4243 | UNBOUNDED | 迁VM字节码 | 调用:- 被调:- | 无界结构 | 4 | - |
| m54.o `cx/compiler/cellname.cx` | 3976 | FINITE | 迁表 | 调用:- 被调:drivestream.o | 低 | 1 | 非默认路径(仅stream子命令) |
| m53.o `cx/runtime/stream_attributes.cx` | 3800 | UNBOUNDED | 迁VM字节码 | 调用:- 被调:drivestream.o | 无界结构 | 4 | 非默认路径(仅stream子命令) |
| bind.o `src/bind.cx` | 3647 | MIXED | 先拆分后迁 | 调用:- 被调:- | 低 | 2 | - |
| entryexec.o `src/entryexec.cx` | 3602 | MIXED | 先拆分后迁 | 调用:- 被调:- | 低 | 2 | 非默认(仅链接保活无运行期调用) |

## 3. 前 10 个最适合先迁的单元(收益/风险)

评分 = .text ÷ (1 + 2×风险项数),仅计「迁表」「先拆分后迁」且批次 ≤2;m14 为 M10c 试点钉首位。

| # | 单元 | .text | 去向 | 风险 | 最小验收 |
|---|---|---:|---|---|---|
| 1 | m25.o `cx/compiler/rtsharedname.cx` | 9405 | 迁表 | 低 | 金标全枚举:键域由事实表定义,网输出与 C 实现逐字段 cmp 全同 |
| 2 | route.o `cx/driver/route.cx` | 12979 | 迁表 | 低 | 金标全枚举:键域由事实表定义,网输出与 C 实现逐字段 cmp 全同 |
| 3 | driveplan.o `src/driveplan.cx` | 9660 | 先拆分后迁 | 低 | 先拆分:IO 壳留切片、纯逻辑子程序差分(语料+边界),壳/逻辑接口以表固定 |
| 4 | m48.o `cx/compiler/fwdincrement.cx` | 9599 | 先拆分后迁 | 低 | 先拆分:IO 壳留切片、纯逻辑子程序差分(语料+边界),壳/逻辑接口以表固定 |
| 5 | attributeframe.o `src/runtime/attribute_frame.cx` | 9505 | 先拆分后迁 | 低 | 先拆分:IO 壳留切片、纯逻辑子程序差分(语料+边界),壳/逻辑接口以表固定 |
| 6 | m20.o `cx/compiler/tapeshape.cx` | 8435 | 迁表 | 低 | 金标全枚举:键域由事实表定义,网输出与 C 实现逐字段 cmp 全同 |
| 7 | bindstage.o `src/compiler/bindstage.cx` | 8235 | 先拆分后迁 | 低 | 先拆分:IO 壳留切片、纯逻辑子程序差分(语料+边界),壳/逻辑接口以表固定 |
| 8 | m50.o `cx/runtime/model_lifetime.cx` | 7983 | 先拆分后迁 | 低 | 先拆分:IO 壳留切片、纯逻辑子程序差分(语料+边界),壳/逻辑接口以表固定 |
| 9 | modelnumber.o `cx/runtime/model_number.cx` | 7106 | 迁表 | 低 | 金标全枚举:键域由事实表定义,网输出与 C 实现逐字段 cmp 全同 |
| 10 | driveargs.o `src/driveargs.cx` | 6701 | 迁表 | 低 | 金标全枚举:键域由事实表定义,网输出与 C 实现逐字段 cmp 全同 |

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
- 批次 4(迁VM字节码)硬依赖 M10b:`archive/plans/v0.0.47-vm-resource-contract.md` 决策门 1-3 未过前不把大状态编译逻辑搬进 VM(OOM 具名可恢复、活跃分配归零、逐分配点失败注入全 PASS、输入长度边界拒绝、步数/取消合同)。
- 留切片的 executor/execute/transition 本身是 M10b 的修改对象,不进迁移清单。

## 6. 登记建议(共享表由集成者改)

`tests/batches.tsv` 建议行:

```
slice-migration-gen	IMPLEMENTED	interfaces	tools/check-slice-migration-gen.cx	SOURCE	lnx/x86_64	55	-	tools/check-slice-migration-gen.cx	-	interfaces.slice-migration-gen	rc0:CHECK_SLICE_MIGRATION_GEN	product
```

`tests/coverage.tsv` 建议行:`interfaces.slice-migration-gen	interfaces	IMPLEMENTED_SEPARATE_BATCH	M10a migration list generator --check against unit-text/slice-units	plans/unit-text.tsv;plans/slice-units.tsv;tools/slice-migration-gen.cx;plans/slice-nn-migration.md`。`tests/toolsloc.tsv` TOTAL 需 +264(gen 227 + check 37),由 cc 授权。
