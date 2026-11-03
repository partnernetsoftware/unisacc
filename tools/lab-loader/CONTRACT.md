# loader 合同（实验线;以后 ujs 车道复用同一页）

本页是 boot / manifest / kind 的接口说明。它只描述**合同**,不描述 unisacc 专有内容:
没有 entry.elf、没有 e1..elf、没有 P3 这些名字。任何上层(NN 集、经典阶段、其他项目)
按本页填 manifest,就能被同一份 boot 加载。

## 1 boot 做什么（四步，顺序固定）

1. **识别宿主**：只认自己编译目标支持的 (os, arch)。
2. **读 manifest.tsv**：同目录下的文本表，按行的 kind/arch 选成员。
3. **校验 sha256**：读到成员的字节后,在 boot 自己的原生代码里算摘要并与该行 sha 比对。
   不接受、不信任宿主外部工具(sha256sum/shasum/certutil 等),因为六格里有四格没有它们。
   不符 → **具名拒绝**(信息里含 `sha256` 与成员名),退出非零,**不 exec**。
4. **交出控制**：exec 选中的成员,argv 原样透传。

boot **不做**的事：不解析成员内部格式、不选阶段、不做路由、不做组合。
路由与组合属于被执行成员(由它读同一份 manifest 决定)。

## 2 manifest 字段

```
name    kind    arch    sha256    size    order
```

- TAB 分隔,一行一个成员,LF 结尾;`#` 开头为注释。
- `name`：相对 manifest 所在目录的路径(可含 `/`)。
- `kind`：见 §3。
- `arch`：宿主的 `os/arch`（如 `lnx/x86_64`），或 `any`（与宿主无关）。
- `sha256`：该成员文件字节的 SHA-256,64 位小写十六进制。
- `size`：字节数。
- `order`：成员序号,从 0 起连续,用于表达**执行顺序**;不是字节数。

## 3 kind 列表

| kind | 含义 | arch | 说明 |
|---|---|---|---|
| `base` | 通用执行器(基座) | 随宿主 | 换权重不动它;每宿主一份 |
| `stage` | NN 阶段权重 | `any` | 与架构无关;按 order 组成执行顺序 |
| `stage-classic` | 经典阶段可执行 | `any` 或随宿主 | 逻辑与底座同体,不能共用 base |
| `resource` | 具名资源(头文件、事实表等) | `any` | 由被执行成员按名字取用 |

规则：

- `base` 行 **必须恰好一行**匹配当前宿主(arch 等于宿主,或 `any`)。多于一行或零行 → 拒绝。
- `stage` 可以有 **N 行**;它们的 `order` 决定执行顺序,由 base 读取,不写进 boot。
- `stage` 与 `stage-classic` 可以并存;区分靠 kind,不靠"有没有模型负载"。
- `stage-classic` 行可以指向与 `base` 行**同一个文件**(L2 发现)：经典阶段把逻辑与底座
  编在同一个可执行里,因此它不能被别的执行器驱动。
- 同一 `name` 出现在多行 → 拒绝。

## 4 必填的不变量（boot 与生成器共同保证）

1. `order` 在同一份 manifest 内从 0 连续、不重复。
2. 每个成员的 `size` 与 `sha256` 与磁盘字节一致。
3. `base` 与所有 `stage-classic` 的 `arch` 要么等于宿主,要么 `any`;`stage`/`resource` 一律 `any`。
4. 命中的成员 sha 不符时**具名拒绝**,不静默回退到别的来源。

## 5 拒绝口径（一律非零退出 + stderr 一行具名信息）

| 情形 | 信息必须含 |
|---|---|
| 宿主不受支持 | `unsupported os` / `unsupported arch` |
| manifest 缺失或不可读 | `manifest` |
| 没有匹配本宿主的 base 行 | `no base row` 与宿主 `os/arch` |
| base 行多于一行 | `duplicate base row` |
| 成员文件缺失 | `missing` 与成员名 |
| sha256 不符 | `sha256` 、成员名、期望值与实得值 |
| order 不连续或重复 | `order` |

## 6 各宿主怎么实现（本轮状态）

| 宿主 | 状态 |
|---|---|
| `lnx/x86_64` | 实跑(L3 之后每轮实测) |
| `lnx/arm64` | 只出字节,CI-ONLY |
| `osx/x86_64`、`osx/arm64` | 只出字节,CI-ONLY |
| `win/x86_64` | 只出字节(MZ/PE 前缀),CI-ONLY |
| `win/arm64` | 无原生切片,照实记缺口 |

shell 段在 Unix 上只做一件事:识别 (os, arch) 后 exec 对应原生段;不做 hash、不做选择。

## 7 已知约束（实测，2026-11-03）

本节记录模板与包格式的真实边界，供 ujs 车道参考；都不是设计意图，是当前实现的实测事实。

1. **P2 与 P3 互斥，且各有硬性形态要求。**
   - `P 2` 包的 `M` 行只有 2 个数（长度），负载必须是**明文网**且首字节为 `N`。
   - `P 3` 包的 `M` 行有 4 个数（压缩长度、原始长度、codec=1、CRC32），负载是
     该原始正文的 raw deflate；解压后**必须以 `UNINETB1` 开头**（二进制网）。
   - 因此「文本网」只能进 P2，「UNINETB1 二进制网」只能进 P3。选错版本会得到
     具名拒绝 `package requires networks`（P2 首字节不是 N）或
     `package requires binary networks`（P3 解压后不是 UNINETB1）。
2. **EOF 必须写成终止动作，不能回到读键状态。** 表里 `START 256 -> START []
   `（空动作）会让执行器在 EOF 键上不前进，无限自环直到 step budget exhausted。
   正确写法是指向一个 `[[ACCEPT]]` 的终态行。
3. **状态名不进 `tbl`/`net` 的 `S` 表。** `S` 表只存被动作序列引用的字符串（拒绝原因等）；
   要按名字找状态，必须从原始表 DSL 或状态序号取，不能读编译后的网。
4. **执行器切片与宿主绑定。** `base.bin` 按宿主一份；换模型不换基座成立，
   换宿主必须换 base。
