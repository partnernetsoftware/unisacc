# R12-0：linuxbridgecheck.py 断言失败诊断

日期：2026-09-29 · 诊断者：dsh · 基线：HEAD `9b587bb`（`cd34167` 之后的树）
授权：cc-unisacc 裁定「授权根因诊断，产出本文件；若根因清楚且改动 ≤20 行、**不在 exec/ 闭包**，可直接修并入门禁；否则只报告」

## 1　现象

`exec/c/linuxbridgecheck.py` 在当前 HEAD 上退出 1：

```
File "exec/c/linuxbridgecheck.py", line 47, in main
    assert text(elf.read_bytes())==text(mac.read_bytes())==text(baseline.read_bytes())
AssertionError
```

无参数即可触发（该脚本只用 `--evidence` 可选参数），因此它是一个「本可入常设门禁」的检查——这正是它在
`research/r12-ungated-checks.tsv` 里被单列为 `red` 而不是 `gate` 的原因。

## 2　最小复现

断言比较三份 `.text` 段：`-target <arch>-linux-gnu` 编出的 ELF、`-arch <arch>` 编出的 Mach-O，
以及**从 HEAD 取同一 `.S` 重编**的 Mach-O 基线。分架构跑（`clang -c`，全部有界）：

| 架构 | ELF | Mach-O | 基线 | `ELF==Mach` | `Mach==基线` |
|---|---:|---:|---:|---|---|
| `arm64` | 276 B | 276 B | 276 B | **真** | **真** |
| `x86_64` | **155 B** | 159 B | 159 B | **假** | **真** |

复现命令（约 3 秒）：

```sh
clang -target x86_64-linux-gnu -c exec/c/librarycall_x86_64.S -o /tmp/e.o
clang -arch x86_64            -c exec/c/librarycall_x86_64.S -o /tmp/m.o
# 用脚本同款的 .text 提取逻辑比对：共有区段 27 字节不同，首个差异在偏移 123
```

## 3　根因

**不是产物漂移，也不是源文件过期。** `Mach-O == 基线` 两轮都为真，说明仓库里的
`librarycall_*.S` 就是 HEAD 版本、源没有动过；漂移假设可以直接排除。

失败只发生在 `x86_64`，差异是一处**分支编码宽度不同**：

```
ELF  : ... 48 39 ca 73 0d              49 8b 04 d2 ...
Mach : ... 48 39 ca 73 0d 0f 83 00 00 00 00  49 8b 04 d2 ...
                    └─ jae +13 (2 B)  └─ jae rel32 +0 (6 B)，纯填充
```

两处尾部 16 字节完全相同，差异只在这一处。

来源是源文件里的一段拷贝循环（`exec/c/librarycall_x86_64.S:78-85`）：

```asm
.Lus_stack_copy:
    cmpq %rcx, %rdx
    jae  .Lus_stack_ready      # ← 这一条
    movq (%r10,%rdx,8), %rax
    movq %rax, (%rsp,%rdx,8)
    incq %rdx
    jmp  .Lus_stack_copy
.Lus_stack_ready:
    callq *%r11
```

**Mac-O 目标下，汇编器在 `jae .Lus_stack_ready` 之后追加一条 `0f 83 00 00 00 00`（`jae rel32`，
相对位移 0），用作对齐填充；ELF 目标下不追加。** 6 − 2 = 4，正好是 155 与 159 之差。

也就是说：两个目标编出的**指令语义完全相同**，但 Mach-O 多了一条等价空跳作填充。断言要求
「三份字节相等」，这个要求在 `x86_64` 上**不可能成立**，与代码正确性无关。

`.p2align 4` 出现在 `librarycall_x86_64.S:12` 与 `:58`，说明该文件本身使用对齐指令；填充是汇编器
为对齐序列而插入的，属预期行为而非缺陷。

## 4　为什么这个断言值得保留

断言想证明的东西仍然成立且重要：**同一份 `.S` 在 ELF 与 Mach-O 上产出相同指令**——这是
`librarycall_*.S` 作为「跨格式桥接载体」的核心声明（文件头注释：「Cross-assemble both ELF
bridges and compare instruction bytes with Mach-O」）。

问题只在于判定方式：拿**原始字节**比较会被对齐填充污染，而该污染与语义无关。

## 5　建议（三种，按改动面从小到大）

| | 做法 | 代价 | 副作用 |
|---|---|---|---|
| **A** | 断言改为比较**指令序列**（`llvm-objdump -d` 的助记符+操作数文本，忽略填充字节） | 约 10–15 行，且脚本已经依赖 `llvm-objdump`（第 38 行） | 弱化到「反汇编文本相等」；填充被忽略，语义差异仍会被抓到 |
| **B** | 保留字节比较，但**先剥离已知填充模式**（`0f 83 00 00 00 00` 等相对位移为 0 的跳转） | 约 8–10 行 | 精确但针对性较强，新填充形式出现时会再次失败 |
| **C** | 断言只在 `arm64` 要求字节相等，`x86_64` 降级为「符号与指令计数相同」 | 约 5 行 | 最省事，但也弱化最多；`x86_64` 的编码差异将不再被检查 |

**dsh 倾向 A**：它保留断言的**意图**（跨格式指令一致），只是换成不会被对齐填充干扰的判定；
且脚本已在用 `llvm-objdump`，不引入新依赖。B 是次选（保留字节级严格性，但把填充名单写进代码）。

**不建议**为了让它变绿而改 `librarycall_x86_64.S`（例如去掉 `.p2align`）：那会改动跨格式载体的
真实布局，把一个「检查方式」问题变成「产物」问题。

## 6　改动归属（需你裁定）

修 A/B/C 任一种都改 `exec/c/linuxbridgecheck.py`——**位于产品闭包内**。按你定的规则，
dsh 未改任何文件，先报清单等你决定：

- 待改文件：`exec/c/linuxbridgecheck.py`（1 个）
- 改动量：A ≈ 10–15 行，B ≈ 8–10 行，C ≈ 5 行（均在你给的 ≤20 行内）
- 改完后可入门禁：实测无参即可跑，且 `clang` 编译约 3 秒，是**唯一**一个无外部参数却当前失败的检查
- 若选 A 或 B，dsh 会同时在 `research/r12-ungated-checks.tsv` 把它的处置从 `red` 改为
  `gate`，并加入 `tests/gate.sh`

## 7　未做与边界

- 未修改任何文件（本报告为唯一新增物）。
- 未验证 `x86_64` 填充是否随 clang 版本变化：本机 clang 版本见下；换工具链后填充可能出现在别处，
  这也是 A 优于 B 的一条理由。
- 未检查 `.p2align` 之外的其它填充来源（如函数间对齐）：差异只有 1 处，未见其它。

```
$ clang --version | head -1
Apple clang version 21.0.0 (clang-2100.1.1.101)
```
