# L1b′ 第一轮原型结果

2026-10-09。**实际跑过；原型尚失败，不是产品修复。**

## 范围与命令

未修改main的src/include/kernel/exec。复制include到`/tmp/cdx-l1b-proto/include`后应用`research/l1b-sigaction.patch`；各编译器由导出参考C的临时副本构建（没有分支/工作树）。头用`-fno-trim-libc -I /tmp/cdx-l1b-proto/include`显式选择，未更新内嵌kernel。

`/tmp/cc-l1b/sig_nested.c`是cc依据旧记录重构，不是旧原始源。`/tmp/cdx-l1b-proto/nested2.c`为本轮更强探针：主程序打印M，ALRM处理函数打印A、发USR2，内层打印B，返回外层打印C，返回main打印D并检查计数与局部哨兵。系统cc实跑为`MABCD\n`、rc0。

每个编译限15秒，产物执行由`tests/bound.py 5`限时。构建临时参考限40秒。执行脚本：

```
python3 tests/bound.py 25 python3 /tmp/cdx-l1b-proto/try.py private osx/arm64
python3 tests/bound.py 25 python3 /tmp/cdx-l1b-proto/try.py private osx/x86_64
python3 tests/bound.py 25 python3 /tmp/cdx-l1b-proto/try-nested2.py private osx/arm64
python3 tests/bound.py 25 python3 /tmp/cdx-l1b-proto/try-nested2.py private osx/x86_64
```

## 已测结果

| 临时变体 | 探针 | osx/arm64 | osx/x86_64 |
|---|---|---|---|
| 旧头补丁+当前参考 | sig_nested | rc138 | rc139 |
| 仅去掉arm64共享FP/SP保存恢复 | sig_nested | rc0 | 未测 |
| 仅强制trampoline host入口 | sig_nested | rc139 | 未单测 |
| host入口+去掉arm64共享FP/SP保存恢复 | sig_nested | rc0 | 未单测 |
| host入口+sys6私有帧 | sig_nested | rc0 | rc142 |
| 旧头补丁+当前参考 | nested2 | 未测 | 输出M，rc139 |
| host入口+sys6私有帧 | nested2 | 输出MAB，rc139 | 输出MAB，rc159 |

rc是看门狗返回的实际值，未用它直接断言内核错误种类。各JSON日志保存在`/tmp/cdx-l1b-proto/*result.json`、`baseline-x86-*.json`；不存在Linux现场。

## 原型内容与边界

私有帧80B：六源快照48B、FP8B、原SP8B和16B余量。先用保留scratch保存原SP，再调整SP；源为r7时使用原SP快照。编号动态和命名调用分别装载参数，gate后恢复FP并撤帧。这里仅改sys6，sys/write共享格尚未转成私有帧。

host入口试验在临时C里强制把`_unisa_sigtramp`判为host入口，目的是隔离栈入口因素。这是**诊断硬接线，不可照搬进产品**；正式方案需要明确的通用入口契约与δ同规则处理。

结果支持共享FP/SP恢复是单层故障因素，但不支持本片已经解决真实信号。

## 新阻断与下一片

- arm64源码中的`.frame`只更新x7，HENTRY只在入口把x7接到真实SP一次。双层信号时，真实SP与tape活动帧下界可能分离，内核的新信号帧可能覆盖活动tape栈。它是由源码和MAB首断推得的疑点，尚未抓到覆盖地址，不作为已确认根因。
- 需要确定真实SP、x7和每层信号上下文的边界，证明分配/撤帧每一步的安全性；不能简单给信号栈加大容量。
- x86单层rc142、双层打印B后rc159，需独立核对macOS跳板六参数、sigreturn上下文与token；不能从arm64成功外推。
- 参数准备期间的确定性嵌套注入尚未做；不能用本探针的单层成功证明共享sys/write参数安全。

暂不落产品δ。cc可并行核对头补丁的x86跳板/sigreturn契约，cdx继续核对arm64双栈与嵌套现场；验证后才决定通用入口和私有帧如何同批实现。

## 第二轮：入口与gate捕获、双层转绿

实际跑过，2026-10-09；产品文件仍未修改。

x86临时编码器在host入口第一条参数移动前保存寄存器，直接write二进制现场，再恢复全部寄存器。在sigreturn的TO_GATE前再次保存。现场文件`/tmp/cdx-l1b-proto/gatecapture-osx-x86_64.result.json`：前80B为入口，后80B为gate，末48B为返回现场。

- 入口r8/r9与跳板uctx/token逐位相等，排除第5/6参数桥接换址。
- 入口rsp在静态64KiB备用栈内，uctx却在另一地址区；不能用uctx不邻近rsp断言桥接错误。
- gate的rax=0x20000b8，rdi=uctx、rsi=1、rdx=token；实际执行syscall，返回+1。
- cc独立系统clang x86_64对照也得到同样失败，`infostyle=30`后能返回main。其源与结果在`/tmp/cc-l1b/`。这里区分cc对照与cdx复验，不把源代码阅读当实跑。
- cdx临时头把x86第二参数固定30：sys6私有帧原型单层rc0、双层MABCD/rc0；Rosetta双层重复5/5通过。

arm64临时变体`/tmp/cdx-l1b-proto/spgate.c`：在TO_GATE保存真实SP到x7下方16B，用真实SP=x7-512执行系统调用，之后恢复真实SP（不改变x7、原错误返回归一化保留）。同一双层探针由MAB/139变MABCD/0，重复5/5通过；单层也rc0。这支持“gate时真实SP高于tape活动帧，嵌套信号覆盖活动栈”的方向，但尚未直接捕获被覆盖字节。

重复结果：`/tmp/cdx-l1b-proto/nested-repeat.json`。构建每步40秒，执行每次2秒，重复实验整步15秒上限。

### 仍需解决的生产边界

1. 512B只为诊断预留，不能当作生产栈契约；还需明确对齐、入口保存区、所有异步可中断指令处的真实SP与tapeSP关系。
2. 仅在gate期间同步真实SP，不能保护gate外任意时间来的信号；正式方案须覆盖分配/撤帧及真实ABI调用边界。
3. `.sys`/`.write`共享暂存格尚未迁移，参数准备中重入注入尚未做。
4. `_unisa_sigtramp`按名字硬编码host入口仅是诊断；产品需通用入口契约。
5. x86的30只在Rosetta实跑，未声称原生Intel通过；[XNU x86 sigreturn源码](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/dev/i386/unix_signal.c)提供UC_FLAVOR与上下文恢复判据，不能代替目标机器验收。
6. 未生成产品δ、未构建私有.com、未完成四个非Windows目标镜像逐字节对拍。L1b′仍未收口。

## 第三轮：gate之外的异步嵌套反例

实际跑过，2026-10-09。诊断源[ l1b-signal-async.c ](l1b-signal-async.c)，结果[l1b-signal-async-results.json](l1b-signal-async-results.json)。诊断编码器差异保存在[l1b-stack-prototype.patch](l1b-stack-prototype.patch)：对临时完整导出C应用，不是对main源应用，更不是待直接落地的生产补丁；其中仍含强制跳板名字的诊断接线。

主程序与ALRM外层处理函数都反复递归6层、创建33个long的volatile局部数组并检查内容。宿主每轮送ALRM，100微秒后送USR2，再等1毫秒；外层末尾还自行触发USR2。内层计数、外层计数、所有局部数组和返回求和均检查。父测试限7秒、执行进程超时必杀。新强探针不依赖内层只在kill系统调用期间投递。

| 原型 | 三次外部异步嵌套 | ALRM发送数 |
|---|---|---|
| 逐帧/调用保护真实SP（shadow） | 3/3输出R、OK，rc0 | 141、143、143 |
| 仅gate下移真实SP（spgate） | 3/3 SIGSEGV，rc=-11 | 每次1 |

阴性对照有效。最初较弱版本没有让外层处理函数递归工作，同一送信号节奏下两个原型都绿；不能把那轮当作gate之外反例。

shadow诊断变更：真实SP取候选x7向下16B对齐后再留16B保护区；正向.frame先保护新帧再更新x7，负向.frame先更新x7再释放；call在保存返回地址前保护，ret取回返回地址后释放；HLEAVE按入口x29锚点恢复宿主SP/FP/LR。该轮没有gate的512B特殊处理。

边界：C产出的路径实测通过，不等于任意输入tape已证明。特别是mov到r7当前统一先更新真实SP，尚需区分向上/向下更新顺序；其他算术写r7未接保护。HOSTCALL、全系统调用私有化和通用回调入口仍未完成。先修这些边界，再移入参考和δ，不把这份诊断补丁直接应用到产品。

## r7显式赋值与HOSTCALL续验

实际跑过。诊断mov到r7增加方向分支：候选低于旧x7时先保护候选，再提交；否则先提交，再更新真实SP。低级`tape`直接把SP下移128B、检查结果，再恢复并检查，真实arm64镜像rc0。双层嵌套仍MABCD/0，异步递归仍R/OK/0（154次ALRM）。任意算术/加载/立即数写r7尚未处理，不能将mov一项记成全域完成。

HOSTCALL源码审计：BK_HOST_ARM和签名桥在真栈私有区域保存x1–x7、原SP、LR，进入宿主后按宿主栈运行，回来再恢复。实际探针在原异步递归主循环里每5轮转发一次libSystem sched_yield（共20000次），R/OK/0，153次ALRM；裸-S tape确认含sched_yield及.hostcall。这只验一个真实宿主签名，不是所有FFI契约穷举。

临时补丁已更新到本轮mov方向分支。正式通用入口、其他SP写入、sys/write私有化、四目标逐字节和私有.com仍待完成，头补丁继续停放。

## 私有三参数、通用入口及其他SP写入

实际跑过。临时POSIX`.sys`/`.write`把三个源值和FP保存到80B私有栈帧，再按ABI形状装载参数。Windows维持原路径。动态编号sys6与命名sys6继续各自语义。`.sys`的atfd_1/atfd_1_zero/atfd_2_zero5/zero4按原lower规则转换；尚未穷举这些形状的实际OS效果。

新增低级tape专门用r7直接作为三种写调用的缓冲，arm64与Rosetta各输出ABCABCABC、rc0，证明取得调整前SP。原型原先用x16暂存原SP，新.frame也会用x16计算候选SP，存在冲突；已改用x12，不能沿用旧暂存选择。

删除backend对`_unisa_sigtramp`的单名判断；临时头中的跳板命名为`__ccw_unisa_sigtramp`，通过既有通用host入口前缀，两架构双层均MABCD/0，arm64 HOSTCALL+外部异步递归R/OK/0（185次ALRM）。include仍未修改，停放头补丁仍需cc把这一命名与autonames守卫一并对齐。

ARM其他写r7的原型先在x14计算结果，再按向下先保护、向上先提交的顺序写x7（x15作SP对齐暂存）；覆盖有限低级目录中以首操作数为目的的计算/加载及SETREG/HOSTADDR/ARGVGET，不覆盖内存写等仅把r7作为基址的指令。sub64/add64/load64写r7低级镜像rc0，双层MABCD/0，HOSTCALL异步递归R/OK/0（194次ALRM）。x14/x15保留用途与所有内部低级序列仍需逐项审计，不能仅凭范围编号认为目录契约永久稳定。

本轮更新的诊断补丁仍针对临时完整导出C；它不再含按信号函数名单名硬接线。还未改产品参考、lower/enc δ，未做每个快照边界注入；`.exit/.print`仍共享格，Linux直接handler入口尚未接桥。不得因本轮Mac原型接受而提前落停放头。
