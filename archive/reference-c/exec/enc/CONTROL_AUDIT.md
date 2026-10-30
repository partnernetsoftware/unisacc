# 编码生成器控制与输入来源审计

审计快照：`main f360e6d` 加 `076a796` 的补丁（隔离树重放为 `764da96`）。
此文记录迁移边界，不替代功能支持清单；早期 [ARM64.md](../../archive/exec/enc/ARM64.md) 的“手写 generator / 非 net”描述不代表本快照。

## 范围与结论

六个目标共享两个入口：`gen.py`（x86_64）、`arm.py`（arm64）。Linux 选择 ELF，macOS 选择 Mach-O，Windows 选择 PE；两个 raw 模式另作底层核对。
运行期输入判定、数值扫描、指令选择/打包、分支松弛、地址布局、重定位、签名算法与输出循环均由现有 `finite_rules` 声明装配。
**不能据此声称所有生成的边和动作都直接来自 TSV，或 Python 只读取常量。** 下列模板装配与初始化仍是 Python；它们也属于生成物的输入。

| 生成模块 | 控制声明 | 保留的真实输入/装配 |
| --- | --- | --- |
| `gen.py` | `x86-procs/line/operand/emit/shell-*` | ENCSPEC、NUM、动态分类和 metadata；共享 byte 序列 |
| `address.py` | `address-*` | DATA_BASE、ELF/Mach-O/PE 布局与 IMPORTS；参数化 header/RIP 实例 |
| `fp.py`、`x86win.py` | `fp-*`、`x86win-*` | FARITH/FCMP/FP_OPS、REGMAP/NUM、IMPORTS、OPS/RCS、FIELDS 与 WINARGS_BODY |
| `arm.py` | `armbase-*`、`armcontract-*` | ENCSPEC、组合后的动态 specs、共享 word 序列 |
| `armint/armmem/armfp/armbranch.py` | 同名 `*-*.tsv` | SPECS、LDS/STS/LDU/STU/IP0、浮点表、RELFIELD |
| `arminput/armlayout/armwin.py` | 同名 `*-*.tsv` | TIns META、格式布局、STD_FIRST、IMPORTS、WINARGS_BODY |
| `x86itoa/armitoa.py` | 同名 `*-*.tsv` | 本模块机器码模板；地址与模板输出次序用声明安装 |
| `memorylayout.py` | `memorylayout-*` | DATA_BASE、DLL/IMPORTS；公共 `exec/modelinput.py` 的 u64 资源读取声明 |
| `elfimage/pedelta/machodelta.py` | 同名 `*-*.tsv` | 格式字段、MACHINE/CPU、HDRS、PAGE、IMPORTS/DLL、IDENT 等 |
| `sha256delta.py` | `sha256-*` | 从 `src/back_image.c` 提取轮常量和初始摘要；算法在声明中 |

共享构造器来自 `exec/parse/gen.py`；有限规则装载器来自 `exec/finite_rules.py`。`tins.py` 的 META 是生成期 schema 输入，TIns Python 解析器不是网络运行时。`unisa.emit_x86` / `unisa.emit_arm` 的 opcode、寄存器和 ABI 模板，以及 `unisa.image` 格式数据仍是依赖；迁移没有复制它们的当前答案。

## Python 剩余项：逐类限定

- `gen.py:222`、`arm.py:85` 的 `g.on` 只装入 `load_rules` 展开的类别行，保留动态类别数量；不是另写的运行期选择梯。
- `elfimage.py:51,65`、`pedelta.py:20`、`machodelta.py:26`：IMPORTS/格式字段枚举用 `P.call` 发出 `EI.bytes` 或大小端字段调用边。宽度、字面量/寄存器值、字段顺序仍由 Python schema 决定。这是有意保留的字段模板装配，不能记作“每条边直接来自 TSV”。
- `armlayout.py:12` 的默认 `target_os=1`，`armwin.py:17` / `x86win.py:25` 的字段清零，以及入口 intern/table 初始化，仍含 Python 动作模板。清零枚举随 metadata 变化；没有独立状态分支。严格要求“固定动作也全为 TSV”时，这些仍是剩余项。
- `x86itoa.py:7-18` 的 rr/imm 模板构造、`armitoa.py:6-34` 的机器字表达式和外部 WINARGS_BODY 含最终机器码中的循环/分支。网络负责调度和地址绑定；不能宣称这些机器码算法已重新表达为 TSV。
- Python 仍计算格式字段长度、IMPORTS 布局常量、fresh 名称及条件安装。固定 header/ABI 字段 schema 仍在模块中；运行期长度、分支距离、页布局、重定位与摘要不由参考编码器计算。

阅读检查：生产模块没有残留 `P.branch` / `P.goto`；上述字段 `P.call` 和两处通用 `g.on` 是完整例外清单。`shacheck.py` 的调用链是测试 harness，不计入生产生成器。此结论是源码审计，不是完整语言正确性证明。

## 精确行数与字节账

计数为 UTF-8 文件实际字节和 `splitlines()` 行数，包含注释/空行；不含生成 JSON/TBL/NET、测试、本文或其它文档。19 个生成模块就是上表所列模块（两个入口、17 个辅助模块）。

| 快照类别 | 文件数 | 行数 | 字节 |
| --- | ---: | ---: | ---: |
| 生产生成器 Python | 19 | 1168 | 66515 |
| `exec/enc/*.tsv` 全部声明及装配数据 | 71 | 2649 | 165958 |
| 上两项合计 | 90 | 3817 | 232473 |
| 另列：TIns schema/参考 parser `tins.py` | 1 | 150 | 6691 |

对 `f360e6d` 的唯一编码实现增量是 Windows 收口补丁：

| 类别 | 原行/字节 | 新行/字节 | 净行/字节 |
| --- | ---: | ---: | ---: |
| `x86win.py` | 125 / 6795 | 91 / 4283 | -34 / -2512 |
| 新增 5 个 `x86win-*.tsv` | 0 / 0 | 269 / 16270 | +269 / +16270 |
| 合计 | 125 / 6795 | 360 / 20553 | +235 / +13758 |

这不是整个迁移历史的净量，也不是压缩后模型大小。

## 实测与未测

本次隔离树实际生成并记录调用模块：两个架构各 raw/ELF/Mach-O/PE，共 8 图，均成功；状态数依次为 x86 `1309/1462/1687/1670`、ARM `1442/1594/1819/1802`。未在本次文档审计重跑语义测试。

此前独立提交实跑：完整图比较含每状态/观察/后继/展开动作/元数据及序列多重集合；FP 的 NET 生成代码双架构各 550 主机用例通过；Windows NET+SIM setup 为 359 字节，全部 11 API 的真实 lower 输入为 132 指令/2381 字节，寄存器/target/arity 和 6 项 gate 拒绝检查通过。动态扰动覆盖寄存器、操作/谓词表、IMPORTS、布局、metadata 基址、返回转换顺序和模板数据。

这些是编码阶段与主机 harness 证据，不是全六阶段或全部 C 语义证明。本轮没有 Linux/Windows 客机执行，没有启动 VM，也没有重新包装出货产物。完整模型冻结后的六目标验收由父任务统一执行。临时证据分别保存在 `/tmp/unisacc-e5-encaudit-928`、`/tmp/unisacc-e5-fp-928`、`/tmp/unisacc-e5-xwin-928`，临时目录不是持久门禁。
