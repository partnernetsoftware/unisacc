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
