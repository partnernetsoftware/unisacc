# 晚绑定 2c3 第0步｜variadic 类别设计（只是设计）

**证据边界：** 现状为读源码与读 gate 清单所得，未跑新实现、未改仓库。建议均为“只是设计”。依据：`research/c43-late-binding-cut2c-step0.md` §2.3、W（7b3e36af，`exec/c/librarylatebind.h` 的 `UsLateClass` 表）、2c2 裁定（prd：typed 归属由模型表决定，逻辑类别与物理 owner 分离，物理复用 function owner1，只加接线行改不了归属）。

## 1. 现状（读源码）

- **编译期不建 variadic 的 cif。** `librarycallables.h:346` 只对非 variadic 原型调 `ffi_prep_cif`；variadic 原型只声明固定前缀（`:415–416` 注释：prototype 只声明 fixed prefix，concrete 声明 promoted tail 且永不作通用签名）。
- **原生 plan 在调用期按具体签名构造。** `librarycallables.h:417–422` 检查 concrete 与原型的一致性（原型 variadic/mode1/计数，concrete 非 variadic、计数不小于原型、结果类型相等）；`:431,438,441,450` 具名报 “variadic callable requires concrete signature / stale foreign or incompatible variadic callable / invalid concrete variadic callable signature”。
- **libunisacc 调用路径。** `libunisacc.c` 在 compile 后替换 native plan 与模板（`:694–697`，`c->native_plans=plans`），调用期经 callable scope（`:854` 起）。
- **E3 已有变参模板。** `exec/parse2/libraryvariadic-*`：17 行模板、117 行结果；`hookcall` 输出 `vcall/plan1/tape2`，含 `SITEFN/SITEBIND/COUNT/DEPTH/BASE/SHAPE/NAMES/REQUESTS` 字段（`libraryvariadic-manifest.tsv:3–4`）。它是 K2 翻译的 E3 片段，不是 W 类别。
- **lower 没有变参调用模型。** lower 中唯一的 variadic 命中是 `librarydataspan-result.tsv:247,265–267` 的 `ldn_variadic`，那是数据名记录的标志位，不是调用点模型。
- **源码前端拒绝。** `parse2/callcontrol-result.tsv:117`：`DEAD.vc` 路径报 `not covered: call to a variadic function`。它属于 parse2 调用控制；库变参调用是否经过这一行，本步未核实，不作结论。
- **现有闸。** `lib-variadic-native`（`gate.sh:524`，产品层原生调用，带 sanitize）与 `lib-variadic-resolver-host`（`gate.sh:447`，冻结模板候选）。即变参运行路径今天已有实测，但不经 W 分派。

结论：变参今天是“E3 模板 + 调用期 concrete cif”的旁路；W 的类别表里没有它。

## 2. 类别与物理 owner（能否复用 owner1）

**能。** 建议：
- 逻辑类别：新增 variadic 逻辑 kind（编号待定；W 的 kind7 仅为测试类，不占用产品编号），wire 与普通函数相同（函数 wire），物理 owner 复用 **function owner1**，partition 与函数相同。
- 类别由分区模型选择：`(kind=函数 wire, callkind=2, V3 显式启用)`，依据 2c2 typed 的办法（`exec/librarytypedpartition`，不查地址）。模型输出仍是每 ID 唯一归属，缺归属或重复归属拒绝。
- 只加接线行不能改归属：归属由分区模型表决定，W 表的新行只提供该逻辑类别的 declare/verify/缺失诊断。
- **不能**把 variadic 校验写进 owner1 的通用 `us_late_function_provider` 分支。依据 2c2 的修正：owner8 会迫使 `libraryrelocate.h` 通用校验改动，与 W 零改动冲突；变参应是独立的 verify 函数指针（类比 kind7 的 `w_kind7_verify`），通用函数路径不变。

风险：owner1 的 relocate 校验若只认函数 wire，variadic 的 concrete 检查必须放在 verify 回调里，不能依赖 relocate 的通用 extent/类型检查。

## 3. callsite 帧与 promoted-tail 类型放在哪个阶段

- **E3（模型阶段）：** 为每个变参调用点输出一条**无地址 callsite 帧**：稳定 callsite ID、固定前缀个数、promoted-tail 类型列表（按 C 默认提升后的类型）、结果类型、ABI 证书需求。它是 USLCON1 的兄弟记录，**不**在 USLCON1 里塞实例 handle（与 2c2 typed 的“计划需求，绑定后实例化”一致）。
- **lower：** 只消费 callsite ID 与需求的 span（与普通函数同样的“未绑定 span”形态），不生成 cif，不做 promotion。
- **运行期（绑定后）：** 按 concrete 签名构造 cif，复用现有 `librarycallables.h` 的 concrete 校验（`:417–422`），不改它的规则。

理由：callsite 是调用点属性，不是符号属性；同一符号的不同调用点可以有不同 tail，把它放在符号合同里会把一个 callsite 的 tail 错误地绑定到整个符号。

## 4. 需要的类别专属表与挂点（只加新文件，共享表先报）

1. 分区模型：`exec/librarycallsitepartition`（新目录，仓外起草），输入无地址声明与 target，输出 callsite 归属（USLSEL1 同格式，owner1）。
2. E3 callsite 需求表：`exec/parse2/librarycallsite-*`（新文件），复用 `libraryvariadic` 的模板字段（`SITEFN/COUNT/DEPTH/BASE/SHAPE` 已有），新增 tail 类型列表与 callsite ID。
3. lower 的 span 表：`exec/lower/librarycallsite-*`（新文件），只声明 ID 与 span 行，不改 `librarydataspan`。
4. W 表：一行新的 `X(...)`（逻辑类别 → 函数 wire、owner1、partition 同普通函数，verify 指向新的 `us_late_callsite_provider`）。这是 W 表唯一的共享改动，需先报 cc。

挂点：`libunisacc.c` 只在 relocate 后把 callsite 框交给 callable 层（不改 compile 期的 plan 构造，`:694–697` 保持）。

## 5. 与 typed 共享多少（fixed 部分）

- 共享：固定参数的类型描述符、USLSIG3 签名编码、函数 wire、owner1 的 relocate 路径、dispatcher/plan-handle 角色、函数 provider 的查找。估计约一半的编码与分派。
- 变参专属：固定前缀计数、promoted-tail 列表、callsite ID、concrete 一致性检查、每调用点的 cif、以及 promotion 规则（整数提升、float→double，ABI 相关）。
- 因此 variadic 应排在 typed 之后，依赖 typed 的 fixed 原型表；typed 先落地前不做 variadic 的 E3 片段。

## 6. 验收探针组（只是设计）

A/B（同源，不同实际 tail 或不同目标）：
- A：tail 类型不同（int vs long vs double）→ E3 callsite 帧 **不同**且与 concrete 对应；未绑定 lower 的 span 与普通函数形态一致；provider 缺失具名拒绝。
- B：同一调用点不同实际 provider 地址 → 帧、ID、span 不变（地址不入模型）。

负例（逐条具名）：missing-callsite（帧缺失）、missing-provider（provider 缺失，不降级旧路径）、unsupported-promotion（未覆盖的提升）、stale-concrete（concrete 与原型不一致，沿用 `librarycallables.h:438` 的文本）。

变参运行：固定前缀 + tail 的返回值正确；tail 为 double 时的 ABI 结果（仅 x86_64 lnx，其它目标报未覆盖）。

## 7. P6 复用

- P6 基线（26 源，130 行 manifest）**不含变参**。它的作用是：每一步后，26 个原有案例的 manifest 必须逐行不变（证明非变参路径未被扰动）。
- 新增变参源（建议 6–8 个：固定前缀 + 不同 tail）作为 P6 的新组，单独冻结，不与原 26 行混排。
- P6 的行为侧需要 variadic 的调用规范：`calls.py` 的调用行可扩展为 `sym ret args...`，tail 以 long 传入不足以覆盖 double，需要在 p6run 增加 double 的调用签名（记录为 2c3 的前置）。

## 8. 不做的事（本步范围外）

- variadic 回调（callback 图）留 2c4。
- Windows 变参 ABI：`lnx` 之外只报未覆盖，不冒充。
- 不改 `libraryrelocate.h`、不改 owner 表的已有行、不开第三物理通道。

## 9. 待 cc 定的点

1. 逻辑 kind 的编号（建议占下一个未用编号，不占 kind7）。
2. callsite 帧放 E3 的新文件，还是并入 `libraryvariadic` 模板（建议新文件，避免改已有模板）。
3. `callcontrol-result.tsv:117` 的覆盖边界是否与库变参一致，需先核实再决定是否要改该拒绝行。
