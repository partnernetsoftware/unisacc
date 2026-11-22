# 0.0.86 判定门:改一行再跑接线

结论:go,但分两段。第一段只做 W2d(cx 单元对象的运行库状态强定义改为 RT_UNIT/RT_EXTERN 守卫,闭包补头),约 2 单位;第二段接线加等价门约 5 单位。600 ms 目标第二段单靠分别编译达不到(实测主单元 -c 约 930 ms),还要 W3b(变参进闭包)或主单元 elf 瘦身,600 ms 降为第三段的目标,不作 W2 验收的硬门。

## (1) 现状图

sh ./unisacc.com FILE.cx -> src/drive.cx 主流程:
- 模块展开:cx/compiler/cxmod.cx 按最近的 cxroot.tsv 解析 use,得到 nsrc 个源 -> dv_expand -> dv_srcv(整程序源向量)
- 进程级缓存:src/runcache.cx rc_lookup_elf,键=候选身份加路由加全部源 sha(USE33 tape、USE34 .host ELF);命中 23 ms
- 单元前端:cx/runtime/unitcache.cx uc_span,键=UNIUC3 加产品身份加单元路由加 cli 状态加单元源;只缓存 e2 与 e1 的输出,单元级
- 分界点:drive.cx 744 行 dv_run_lower_n(route,...,nsrc,dv_srcv,...),从这里开始 units/e3/e4/prune/lower/elf 都按整程序跑,键是整程序
- 执行:hs_forward_exec / cx/runtime/runexec.cx(只做门与状态行)
- 未接线:cx/driver/unitobj.cx(USE35 单元对象键与 .uo 存储)、cx/driver/objlink.cx(ol_link)在 src/ 和 cx/ 里零调用;只有 tools/check-objlink.cx 用到

## (2) W2d/W3/F5 状态

- 已有:W2 闭包(tools/closure-obj.cx,只覆盖 C 头 stdio stdlib string ctype errno)、W2c COMMON 合并与 __init_u、W3a objlink(批次 objlink IMPLEMENTED)、USE35 键与存储
- 缺:W2d 设计单(v0.0.81 起顺延至今,未做);W3b 变参进闭包(补丁归档在 archive/plans/v0.0.75-r2-w3b*.patch,未入);W3c run 分支接线;F5(drivelink 与 objlink 两路对拍检查)未做,无批次
- 本次实测把 W2d 的实质定位了:cx 单元对象(-DRT_EXTERN 下)里 environ 与 _unisa_sig、_unisa_sigblocked、_unisa_sigflags、_unisa_sigmask、_unisa_sigpending 是强定义 B,每个单元一份,objlink 原样拒绝:
  `REFUSED 4 objlink: REFUSE duplicate symbol environ in x/m.o`
  environ 在 include/cx-flat.h 有三份定义(3141、3416、4773 行),_unisa_sig* 在 6134 到 6138 行,均无 RT_UNIT/RT_EXTERN 守卫

## (3) 接口体积

| 项 | 文件 | 改动 | 估 |
|---|---|---|---|
| W2d | include/cx-flat.h 的生成源(environ 三处、signal 段)加守卫;tools/closure-obj.cx HDRS 加 signal 与 cx 隐式运行库所需头 | 守卫只在定义 RT_* 时生效,默认分支不变,-o 字节不变(需字节等价回归证实) | 约 40 行,1 单位 |
| F5 | 新 tools/check-unitlink.cx 加批次 | 同一 cx 脚本整程序与分别编译两路 stdout、rc、stderr 对拍,含拒绝(重复强定义、未定义)文案 | 约 200 行,1 单位 |
| 接线 | src/drive.cx run 分支(744 行前)或新 src/driverun 段 | 对 dv_srcv 中非主单元:uo 键查 -> 未中则 -c -funit -DRT_EXTERN 出对象并 uo_store;主单元出对象;ol_link 闭包加全部对象;任何 objlink 拒绝回落整程序路 | 约 250 到 350 行,3 单位 |
| 等价门与批次 | tests/batches.tsv 两行,tests/run.cx 对拍 | | 1 单位 |

- 只接 run 路,-o 字节线不受影响;exec/pipeline/image-stages.tsv 不需改(仍走 e2 到 elf,只是按单元)
- 不需三源同改:cxmod 解析不变(cx/cx0/plan.cx、src/cxmod.c、cx/compiler/cxmod.cx 不动)
- 不需新网或新权重;但 include 头属产品输入,改后须重建候选并核身份
- 合计约 7 单位(第一段 2,第二段 5),与计划 W1 3 加 W2 5 相当

## (4) 小实验(私有树 g86.dmd1T9,load1 在 11 到 15,时间偏高)

脚本 use sys.env sys.fs mem.heap(闭包 mem.bytes),整程序 -run:
`h=none argc=3 rc=3 ms=2187`
手工步骤(冷,UNISACC_RUN_CACHE=0 UNISACC_UNIT_CACHE=0):
- closure-obj build:6241 ms(一次性,按内容寻址)
- 各模块 -c -b lnx/x86_64 -funit -DRT_EXTERN:sys/env 1094、mem/heap 1108、mem/bytes 853 ms;主单元 982 ms(对象 192 KB,含每单元一份的 printf 族)
- 直接 objlink:被 environ 重复强定义拒绝(见上)
- 用 objcopy -W 把上述六个符号削弱(代 W2d 的守卫)后链接成功 73920 字节,运行与整程序逐字节一致:
  `h=none argc=3 rc=3`;`G86X=yes` 时 `h=yes argc=2 rc=3`,整程序同
- 改主文件一行两次:`cmain=970 weaken=5 link=8 exec=2 total=987`、`cmain=914 weaken=4 link=7 exec=1 total=928`;同时整程序 `whole ms=2166`
- 主单元 -c 分相(RR_TIME,ms):e2 156、e1 6、e3 167、tapecheck 27、e4 68、prune 11、lower 114、elf 255。elf 与 lower 大头来自单元里自带的运行库静态函数(W3b 未做)
结论:手工路径走通,重编范围只剩主单元,2166 -> 约 930 ms(约 2.3 倍);600 ms 还差约 330 ms,主要可从 elf 255 与 lower 114 里取(W3b 或主单元只出用户函数)

## (5) 等价门设计

- 定义:同一脚本同一参数与环境,分别编译路与整程序路 stdout 字节相同、rc 相同、stderr 字节相同(含编译期拒绝文案);ELF 字节不比,run 路产物不入 -o 字节线,新合同写为"run 路分别编译只承诺可观察行为等价,产物身份不入候选键"
- 回落:objlink 任一拒绝(rc 2 到 7)与闭包未覆盖的头,一律回落整程序路,回落本身要有批次且 stderr 不多出字节
- 对拍:tests 里现有 run 与 cx-host-run 批次加 UNISACC_RUN_SPLIT=1 与 =0 各跑一遍,逐字节比;再加 F5 的拒绝用例;已知不覆盖:回溯位置(objlink 头注释标 UNMEASURED)、跨单元字符串合并与 TLS

## (6) 结论与切口

go。
- 第一段(约 2 单位,先做):W2d 守卫加闭包补头加 F5 检查。验收:sh ./unisacc.com tools/check-objlink.cx 仍 OBJLINK_OK;新 check-unitlink 对本实验脚本(不用 objcopy)两路 stdout/rc/stderr 一致;-o 字节等价回归(未定义 RT_* 时 sha 不变)
- 第二段(约 5 单位):drive run 分支接线(uo 缓存加 objlink 加回落)加等价门批次。验收:tests/run.cx --smoke 全绿;run 类批次两路逐字节一致;改一行 RR_TIME 只见主单元 e3 到 elf;墙钟 2086 -> 1000 ms 以下
- 第三段(储备):W3b 变参进闭包或主单元对象瘦身,目标 600 ms 以下
开放问题:
(1) cx 隐式运行库还有哪些强定义状态(本次只见 environ 与 _unisa_sig*,errno 等由闭包给),需全模块集扫一遍 nm
(2) 单元对象键要含 -DRT_EXTERN 与闭包 sha,模块的传递 use 头集合进 H 字段是否足够
(3) 模块私有 static 状态与 __init_u 顺序在多模块(初始化有依赖)下是否与整程序一致
(4) cx-flat.h 是生成物还是手写,守卫要改在哪一层
(5) 进程级缓存 USE34 与分别编译并存时的优先级与失效
