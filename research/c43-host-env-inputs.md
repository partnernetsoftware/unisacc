# 宿主环境变量对产品结果的影响：现状记录（2026-10-14）

性质：现状记录与设计建议；不改产品。对应 `research/c43-plan-proposal.md` 第 1 步（核心阶段不得读取宿主状态）。

## 实测（实际跑过）

`tests/hostenvcheck.cx`，候选 `0556ea9f…`（`/tmp/c43-forward-paths/final/unisacc-next.com`）。对产品源码里每个 `getenv` 读取的变量，在“全部清空”的基线之外逐个设值，比较退出码、stdout、stderr 和写出的文件。程序两个：`int main(void){return 3;}`（`-S`、`-o`、`-run`）和调用 `sysconf` 的转发程序（`-run`）。

| 变量 | 取值 | 结果 |
|---|---|---|
| `UNISA_MAXSTEPS` | 1000、0 | **四种运行全部改变**：编译以 `timeout` 失败，rc3 |
| `UNISA_MAXSTEPS` | 100000000 | 无变化（大于这些程序所需） |
| `UNISA_ATTRIBUTES`、`UNISA_CONTAINER`、`UNISA_JOBS`、`UNISA_FWD_UNITCACHE`、`UNISA_FWD_REPLAY`、`UNISA_MEMORY_TWOPASS`、`TMPDIR` | 见检查 | 这几个程序上无变化 |

“无变化”只对这两个小程序成立，不是全域证明。

## 读源码（未执行）

- `exec/c/run.c:647`：`maxsteps = getenv("UNISA_MAXSTEPS") ? strtol(...) : UNISA_DEFAULT_MAXSTEPS`（默认 200000000，`run.c:22–23`），传给 `core_run`。步数上限决定每个阶段是接受还是以 `timeout` 拒绝，所以宿主环境可以直接改变核心阶段的结果。
- `exec/c/run.c:610–640` `core_host_fetch`：资源键在进程资源和随包资源里都找不到时，**非头文件键被当作文件系统路径直接读盘**（`snprintf(path,"%.*s",n,p)` 后 `readfile`）；头文件键在 `INCDIR` 下读。这是第 1 步要关掉的磁盘回退。
- `UNISA_ATTRIBUTES`（`run.c:763、775`）既读又写一个宿主文件，影响 E3 读入的属性资源。
- `UNISA_CONTAINER`、`UNISA_JOBS`、`UNISA_FWD_*`、`TMPDIR`/`TEMP` 属于适配器或调度，不直接进核心阶段；仍应在身份表里记为观测层输入。

## 建议（只是设计，需主人裁定）

1. 步数上限是执行预算，属于合同的一部分，不应是宿主状态。两种做法：(a) 出货产品固定为常量，环境变量只在测试构建里生效；(b) 改为显式命令行参数，并在拒绝诊断里写明“预算耗尽”与所用预算，使结果可复现。建议 (a)，并让超预算的拒绝与语言拒绝用不同的诊断词。
2. 磁盘回退：严格模式下缺键一律报“资源缺失”，不读盘；默认产品路径是否也改，由主人定（影响用户的 `#include` 查找行为）。
3. `UNISA_ATTRIBUTES` 写宿主文件的副作用应移出出货产品，或限定为测试接口。

登记：`tests/hostenvcheck.cx` 不入闸，记录完成返回 77。
