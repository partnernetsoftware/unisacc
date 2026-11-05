# unisacc 0.0.44-rc3 候选 —— 发布说明草稿

> 这是**候选**说明,不是正式发布。不 push、不上传、不签名;是否发布由主人决定。

## 是什么

unisacc 是一个 C99 编译器,自己写出可执行文件(ELF、带 ad-hoc 签名的 Mach-O、PE),能编译自身,整个工具链打成一个文件 `unisacc.com`(APE:一个文件内含五个平台片和一份模型包)。默认模式是"编译并在内存中直接运行"(`unisacc.com hello.c`),不是 cc 的 `a.out`。

本候选:
- APE `unisacc.com`:sha256 `54bb97a2672f5010c8815fb0f6ecfb3ee7bb7ca91af18723007430375cc3bbb8`,5,608,646 字节
- 源码:`b1a219cf7d`
- 自举:lnx/x86_64 候选连续三代逐字节相同(`45bdf636df1e`)

## 支持哪些平台

在 GitHub 原生 runner 上实测(CI-RUN-6,运行 42092234905):

| 目标 | 状态 |
|---|---|
| Linux x86-64 | 可用,7 项全过 |
| Linux arm64 | 可用,7 项全过 |
| macOS x86-64 | 可用;stdio errno 用例需要 Linux 的 `/dev/full`,未跑 |
| macOS arm64 | 可用;同上 |
| Windows x86-64 | **不可用**:`--version` 输出乱串,编译即崩溃(0xC0000005),原因未定位 |
| Windows arm64 | 没有这个平台的片 |

7 项是:身份核对、`--version`、编译并运行 hello、默认 `-run`、跨单元 stdio、stream 管道、smoke。

## 已知限制

- **Windows 不可用**(见上)。
- **部分非法 C 会被静默接受,产物不可信**:22 种 C99 约束违例不报错,其中 12 种(重复 case、case 在 switch 外、结构重定义、冲突的 typedef 等)可能产出错误代码。需要约束检查请另用 `gcc/clang -std=c99 -pedantic-errors`。
- **`-c` 分开编译再链接**:运行库状态不跨对象共享;两个及以上调用 stdio 的对象一起链接时只打印一次告警,不报错。要共享 FILE 状态,请一次编译所有源。
- **手工拼 `unisacc stream` 管道**:tape 没有结束标记,上游截断时下游可能接受不完整的输入;请用 `set -o pipefail` 判断上游失败。正常的 `-o`/默认运行不受影响。
- 预处理器接受两种非法输入(宏名不是标识符、`#include` 头名未闭合)。
- `-c -b osx/*|win/*` 文档写的是拒绝,实际会编出对象。
- 运行库在每个对象里各带一份(约 264KB),未去重。

## 怎么验证

1. 核哈希:`sha256sum unisacc.com` 应为 `54bb97a2672f5010c8815fb0f6ecfb3ee7bb7ca91af18723007430375cc3bbb8`。
2. `sh ./unisacc.com --version` 输出 `unisacc 0.0.44`,stderr 为空。
3. 冒烟:`printf 'int main(void){return 7;}\n' > t.c && sh ./unisacc.com t.c; echo $?` 应为 7。
4. 各平台的原生证据:仓库工作流 `candidate-native.yml` 的运行 42092234905,每格证据表在其产物 `evidence-<os>-<arch>` 里。
