# CX-M-2 库模块 main 条款：现状与迁移提案（2026-10-31）

状态：提案，不改 spec/code；不并 W-7/身份预检。

## 现状
CX-M-2：库模块不得定义 main。当前三源（src/cxmod.c:652、cx/compiler/cxmod.cx:689、cx/cx0/plan.cx:707）只在 `cxm_lib[u]` 为真时检查，即“被 use 的模块”才拒绝；命令行根或未被使用的 module 文件定义 main 仍被接受。

三入口实测（同一 root 形状 `#pragma unisa module modulemain` + main）：
- 已装 `unisacc -cx-plan` rc0，输出单元；
- 隔离候选 gcc_cx/clang_cx 编译 rc0；
- 与 CX-M-2 字面合同冲突。
（身份只能绑定各自实际执行产物；不跨替换假定。）

## 真实根盘点（working tree，排除 out/archive/ujs）
- apps/csih 24（其他车道，需其主人迁移）
- tools 35（本仓工具，多数是生成/构造脚本）
- tests 16（夹具/测试程序）
- archive 2（不迁移）
- src/release/examples/bench 0
合计非 archive 75 个 tracked 文件同时含 module pragma 与 main。

## 修复方案（待核准）
1. 三源在扫描/投影后，对**所有**声明了 `module` 的 .cx 单元检查顶层 main 定义，不再受 `cxm_lib` 限制；诊断统一为 `module NAME defines main (a library module)`。
2. 已被 use 的模块仍拒绝；位置/条件/未知 pragma 既有检查不变。
3. 不考虑把 module 文件“允许 main”作为例外；不为通过测试放宽条款。
4. 单一权威解析：三源同诊断、同位置；已装产品与候选分别取实际执行身份。

## 迁移
- tools 35：独立入口删除 `#pragma unisa module`（保留 main），或用单独的库模块承载函数；逐文件用已装产品构建验证。
- tests 16：夹具改为库模块无 main + 消费者，或删除 module 行；保留 CX-M-2 负例。
- apps/csih 24：其他车道，发信封由主人迁移；不具备前置条件前不启用全局拒绝。
- archive 2 不迁移；src/release 0。
- 产品构建 entry-build/库清单中若有 module+main，按同规则先清点。

## 验收
- 三入口 module+main 均 rc 非0、诊断含 library module、不产出目标；module-only 库仍可 `-c -o` 并链消费者；被 use 的 module+main 仍拒绝。
- 迁移后 tools/tests 受影响批次全绿；csih 由主人提供证据；完整仓扫描无未迁移 module+main。
- 不把本提案当作已修复；不改 CX-M-2 text/spec。
