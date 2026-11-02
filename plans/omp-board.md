# omp 状态板(omp-cc 维护,2026-11-02 21:05)

编排者 omp-cc。每个窗口永远有一个"当前任务+下一个任务";空闲即派活,不等回信。

| 窗口 | 当前任务 | 下一个 | 可写路径 |
|---|---|---|---|
| omp-bdy | kernel 收窄:adopt/mixed/pack 三个校验方同步 kernelinputs,一步一发 | 纪元合并跑准备 | tools/cxlib/{kernelinputs,kernelcontext,rebuildcontext,sourcepackage}.cx、tools/source-package-{adopt,mixed,pack}.cx、tools/kernel-build.cx、tools/asm-material.cx |
| omp-go | tools/check-kernel-context.cx 四项负例+登记批次(基于已提交 kernelinputs) | W4b netname/streamname 合并探针 | tools/check-kernel-context.cx、tests/batches.tsv 自己的行 |
| omp-ds | census 复跑(`tests/run.cx --all` 并行只读)并逐项归因 UNKNOWN | check-host-provider 登记批次(如实标红) | tests/census*、tools/check-host-provider.cx、自己的批次行 |
| omp-go2 | 重建 pins.tsv(check-pins failed=0) | W6 source-job-wave 迁移计划(只计划) | tools/pins.tsv |
| omp-go3 | tmux 状态栏(moltbaby)已收口 | 待命 | moltbaby 仓 |

冻结:无。长跑会话:无。已装产品:快车道候选(HEAD 416bc6d0c2 + 旧 p3),putc/磁带 R1-R4 红待纪元。
纪元合并清单:W5 补丁 01-04、cwd 头修复(host.cx 已提交)、FFI 字节表+include 对照、kernel 收窄。
