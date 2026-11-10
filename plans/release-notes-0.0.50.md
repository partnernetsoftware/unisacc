# unisacc 0.0.50 -- 发布说明(草稿,未发布;占位以 {{...}} 标出,未实跑一律写「未跑」)

本版主题是**构造即图,阶段原生**:候选构造变成一条可续跑的命令,产品可从上一发布版在任意物理根逐字节复现;产品侧第一步做了 e3 阶段检查点。产品证据是同候选六目标原生 CI(见下表);s9 跑了 {{S9_TIERS}} 档。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。`sh ./unisacc.com --version` 输出 `unisacc 0.0.50`。

## 身份与证据

| 对象 | 身份 | 说明 |
|---|---|---|
| APE `unisacc.com` | SHA256 `{{APE_SHA}}`,{{APE_BYTES}} 字节 | 同候选六目标原生 CI 的受测对象 |
| 冻结源 | `{{REV}}`,源 SID `{{SID}}`(source-packages-v2) | 版本串不在源 SID 中 |
| 构造 | 由 `tools/candidate-chain.cx` 一条命令从快照到打包,端到端墙钟 {{E2E_WALL}}(loadavg {{LOAD}}) | 0.0.49 为多窗口手工接力十余小时 |
| 两代不动点 | g2 与 g3:{{FP_SUMMARY}} | |
| 可复现性 | 0.0.49 发布版 APE 2d3852a8 已在不同物理根由 candidate-chain 逐字节复现(六片 8224ff13/f7ddb573/eddda34a/e1087d14/64782d36/2f520450) | 帧依赖物理根、产品不依赖,受 graph-check root_dependent 列与 check-root-independent 机检 |
| s9 | {{S9_RESULT}} | |
| 原生 CI | **{{CI_RESULT}}** | 运行 `{{CI_RUN}}` |

## 行为变化(升级前请读)

1. **e3 阶段检查点(候选 Y-0)**:phase B 的中间产物带 `.sha256` 收据,不符即拒绝、不回退;修复了旧双相位 B 路径单元数停在 1 的缺陷;Windows 目标无转发桩时走整路由。对正常一次性编译的用户不可见,产物逐字节不变(同源六模块含 dlopen 程序 oracle 22/22)。
2. {{OTHER_BEHAVIOR_CHANGES}}

## 本版修复与新增

- **一条命令复现候选**:`tools/candidate-chain.cx`(26 相位,检查点 + `--resume`,子进程 ≤55/60s;作业波按实测自适应分块;快照跳过 freeze 前实算 SID 核验;pack 六片 + bootstrap)。
- **构造图**:`build/graph.tsv`(203 节点、833 边,列含 `root_dependent`、`split_dim`/`split_min`)与 `tools/graph-check.cx`(无环、依赖、预算、六目标与相位次序对照、第二真相源计数)。
- **可复现性不变量**:`tools/root-canon.cx`、`tools/root-independent.cx`。
- **自动再分块库**:`tools/cxlib/rechunk.cx`(目标每波用时落在预算 45-75%,超预算减半、自动续跑,不放宽预算)。
- **lint**:`tools/cx-lint.cx`(R1 第二真相源只报告;R2 子进程时限与 `--` 分隔进 precommit 拦截)。
- **0.0.49 债清零**:opguard 与 kernel-build 显式构造器、旧 rb_phase/whole-phase 路径删除、rb_pipe 子进程加上界、`check-utf8` 缺 `--` 分隔。
- {{R_S_LAND_ITEMS}}

## 已知限制与未声称的性质

- cache-manager 两个二进制仍为借用输入(来自 0.0.49 显式输入),其构造未入 candidate-chain。
- `chain-step.cx:134` 的六目标常量顺序与另两处不一致(graph-check order_mismatch=1),改动需候选重建证明字节等价,留待 {{ORDER_FIX_VERSION}}。
- Mac/Win 切片仍由工具层分相编排产出(方案 X);Y-B(e3 按单元续跑)已有精确设计未实现,X 待 Y-B 落地后删除。
- s9:{{S9_LIMITS}}。
- 决策守卫等开发工具链的改动不属本产品。

## 验证入口

1. 核哈希:`sha256sum unisacc.com` 应为 `{{APE_SHA}}`。
2. `sh ./unisacc.com --version` 输出 `unisacc 0.0.50`。
3. 冒烟:`printf 'int main(void){return 7;}\n' > t.c && sh ./unisacc.com t.c; echo $?` 应为 7。
4. 复现:`sh ./unisacc.com tools/candidate-chain.cx -- --rev {{REV}} --out NEW_DIR`,产物 APE 应与上表相同。
5. 原生 CI 证据:见上表运行号。
