# WARN-PROBE-USED 方案(进 rc2 候选)

## 现象(实测)

- `src/drivelink.cx:54-72`:`dv_has_probe` 在单元 tape 里找 `.bss g_<probe> ` 行(probe 取自 `exec/facts/rt-shared.tsv`,stdio 组为 `_u_st_fd`),有就计数;第 2 个这样的对象触发告警。
- 每个 `.cx` 对象都带全份运行库(RT-DEDUP 之前每对象一份),所以 `.bss g__u_st_fd` 行恒在:`/tmp/objm` 的 main-x86_64.o、value-x86_64.o、value-arm64.o 里 `g__u_st_fd` 各出现 5 次,其中 `main.cx` 只有 `extern int object_value; int main(void){return object_value;}`,完全不用 stdio。
- 结果:两个 `.cx` 对象一起链接就恒告警 1 次,与源码是否用 stdio 无关;纯 C 对象不受影响(bdy 实测 stderr 0 字节)。objects-* 九批因此红。

## 判据应该是什么

告警要表达的事实是"两个以上单元**会各自操作**一份私有流表,跨单元的 FILE 状态会分裂"。所以判据应是"该单元的代码实际引用了流表",而不是"该单元带了流表"。

可选做法(按改动面从小到大):

1. **引用判据**:一个单元算"用了流表",当且仅当它的 tape 里,除运行库自身的定义之外,还有对 stdio 组任一状态符号(`state_symbols` 列)或其入口函数(`exempt_symbols` 列里的 `_u_st_*` 入口)的引用,且引用出现在用户函数体内。实现要点:tape 里运行库段与用户段要可区分。若当前 tape 没有边界标记,需要先在 lower 阶段给运行库段加一个可识别的界(属产品输入,进 rc2)。
2. **可达判据**:从用户入口(`main` 与非运行库的外部可见函数)出发,在 tape 的调用边上做可达性分析,能到达 stdio 组状态的单元才计数。比 1 准,但要在 drivelink 里建调用图,代码量更大。
3. 不推荐:改成"stdio 头被 include"作判据。头文件是否 include 与是否真用无关,会漏报也会误报。

建议先做 1;如果 tape 里运行库段不可区分,就按 2 做,不要退回"带了就算"。

## 判据来源

- 符号集合继续只来自 `exec/facts/rt-shared.tsv`(经 `tools/rt-shared-gen.cx` 生成的 `cx/compiler/rtsharedname.cx`),drivelink 里不写符号字面量(与现状一致)。
- 若要区分"定义"与"引用",在事实表里加一列(例如 `entry_symbols`)并由生成器派生,不在 C 里手写名单。

## 测例(新增或改造,断言不放宽)

| 用例 | 期望 |
|---|---|
| 两个 `.cx` 对象都不用 stdio(`main.cx` + `value.cx`,即 /tmp/objm 那对) | 链接 rc0,stderr 0 字节,运行 rc 7 |
| 一个用 stdio(`printf`)、一个不用 | 不告警(只有一份会被操作) |
| 两个都用 stdio(各自 `printf`/`fopen`) | 恰好告警 1 次,rc 不变 |
| 三个都用 | 仍恰好 1 次 |
| 纯 C 对象两个 | 不告警(回归) |
| `-c -funit` 与 `-O0/-O1/-O2` 三级 | 结论同上,不随优化级变 |

现有 `tests/objects*.cx` 的断言(`product-link-accept` 要求 stderr 为空、`refuse()` 逐字节比 stderr)**不改**;修好后它们应自然回绿。`tests/stdio-crossunit.cx` 要加"两个都用 stdio 时恰好一次告警"的正例,保证判据收窄后告警没有被整体关掉。

## 范围与验收

- 改动:`src/drivelink.cx`(判据);若走做法 1 可能还要动 lower 的运行库段界、`exec/facts/rt-shared.tsv` 与生成器。都是产品输入,只进 rc2 候选,在仓外克隆里出 patch(RC2-CANDIDATE)。
- 验收:objects-* 九批 PASS;formats-objects PASS(回归);stdio-crossunit PASS 且含新正例;单源产物字节不变(`tests/stream-pipe`、`kernel-base` 不变)。
- 回滚面:判据改错的后果是漏报(真分裂时不告警),所以"两个都用"的正例必须先写、先红,再改判据。
