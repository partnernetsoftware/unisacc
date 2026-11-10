# unisacc 0.0.50 -- 发布说明(草稿,未发布;未实跑一律写「未跑」)

本版主题是**构造即图,阶段原生**,但产品侧的第一步(候选 Y-0,e3 阶段检查点)在候选构造中暴露出产品级回归(见「未进本版的改动」),按「一个候选只含一类大改」与「未证实不宣称」的规则**不进 0.0.50**。因此 **0.0.50 的产品字节与 0.0.49 同源**:同一冻结源 979b56e4 的六片 g3 切片与 P4 包,只在打包时注入版本串 `0.0.50`。本版交付的是构造体系:候选从快照到打包成为一条可续跑的命令,产品可从上一发布版在任意物理根逐字节复现。产品证据是同候选六目标原生 CI(见下表)。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。`sh ./unisacc.com --version` 输出 `unisacc 0.0.50`。

## 身份与证据

| 对象 | 身份 | 说明 |
|---|---|---|
| APE `unisacc.com` | SHA256 `76ed4200aaaf71b2ac9dd13e6dde6ad31632016ddea8fe43893a4ffe32484967`,6501348 字节 | 同候选六目标原生 CI 的受测对象 |
| 冻结源 | `979b56e4d918894670a71080970149112499d223`(与 0.0.49 相同),源 SID `e764a8165b5aa9c4213d0367d602e2a8c322ed38c6ee147d1630d6e2257e2077`(source-packages-v2) | 版本串不在源 SID 中;候选表 commit 列记真实构造源,不记打包时的 main 头 |
| 打包输入 | P4 `56efea53…`;六片 g3:win/x86_64 `64782d36…`、lnx/x86_64 `6db86a94…`、lnx/arm64 `09f8dcb1…`、osx/x86_64 `eddda34a…`、osx/arm64 `e1087d14…`、win/arm64 `2f520450…`;cache-manager `452d9240…`/`3f6e4cbd…` | 与 0.0.49 发布收据 `ape-receipt.txt` 逐项相同,只有候选表 version 不同 |
| 与 0.0.49 APE 的差异 | 52528 字节 | 切片经 deflate 打包,版本槽的 6 字节变化在压缩流中级联;pack 已证可复现(0.0.49 APE 由同一输入在不同物理根逐字节复现),唯一变更输入是版本串 |
| 两代不动点 | 沿用 0.0.49 的 g2==g3 证据(同源同切片) | 本版未重新构造切片 |
| 可复现性 | 0.0.49 发布版 APE 2d3852a8 已在不同物理根由 `tools/candidate-chain.cx` 逐字节复现(六片全部相同) | 帧依赖物理根、产品不依赖,受 graph-check `root_dependent` 列约束 |
| s9 | 未跑(0.0.50 APE 未跑 s9 任何档;0.0.49 同源产品的提交档 96/96 见 0.0.49 说明) | 不称全批通过 |
| 原生 CI | **QUALIFIED**,运行 `42767415154`(六目标 candidate 作业 + fetch/cell-tools/ffi-lnx/report 全部 success,提交 X=114ef751be) | 版本精确匹配 `unisacc 0.0.50`,独立资产 slice-win-arm64 已注入版本 |

## 行为变化(升级前请读)

1. 产品的 C 语言行为、ABI、六目标产物与 0.0.49 相同;只有 `--version` 与记录文件的版本/摘要不同。
2. `unisacc.com.build.json` 采用规范形(`settings: {}`),`commit` 为真实构造源 979b56e4。

## 本版修复与新增(构造体系,随仓库交付,不在 APE 内)

- **一条命令复现候选**:`tools/candidate-chain.cx`(26 相位,检查点 + `--resume`,子进程 <=55/60s;作业波按实测自适应分块;快照跳过 freeze 前实算 SID 核验;pack 六片 + bootstrap;verify-snap 接受祖先快照并记 BOOTSTRAP_FROM)。
- **构造图**:`build/graph.tsv`(203 节点、833 边,列含 `root_dependent`、`split_dim`/`split_min`)与 `tools/graph-check.cx`(无环、依赖、预算、六目标与相位次序对照、第二真相源计数);`build/hardwire.tsv`、`build/preflight-rules.tsv`(9 条隐性前提机检规则,0.0.51 预检工具的输入)。
- **可复现性不变量**:`tools/root-canon.cx`、`tools/root-independent.cx`。
- **自动再分块库**:`tools/cxlib/rechunk.cx`(目标每波用时落在预算 45-75%,超预算减半、自动续跑,不放宽预算)。
- **lint**:`tools/cx-lint.cx`(R1 第二真相源只报告;R2 子进程时限与 `--` 分隔进 precommit 拦截)。
- **0.0.49 债清零**:opguard 与 kernel-build 显式构造器、旧 rb_phase/whole-phase 路径删除、rb_pipe 子进程加上界、`check-utf8` 缺 `--` 分隔;S(1) 源身份测量接受显式 native helper;D(5) chain-step pack 首参显式绝对 ROOT。
- **release-build 两处回归修复**:rb_die 丢失 `sy_exit(1)`(缺参越界读 argv 以 139 退出)、rb_pipe 丢失尾部换行修剪(每条 freeze 路径在 rev 处 die);`tools/check-release-usage.cx` 在临时共享克隆内幂等验收。
- **独立切片版本注入**:`tools/version-inject.cx`(与 ape-pack 同源 `rv_inject_slot`;检查 `check-version-inject` T1-T4)。
- **登记表合并**:`.gitattributes` 对账本与三张登记表启用 `merge=union`,并行补丁不再在表尾冲突。

## 未进本版的改动(留 0.0.51)

- **候选 Y-0**(e3 阶段检查点,main 提交 77b5e559b3):用 Y-0 构造器编出的 worker.o 出现两处 `R_X86_64_PLT32 setpgid` 外部引用,而 0.0.49 构造器编出的同源 worker.o 没有——fork 后的 `sy_setpgid` 被降成 libc 符号,六片 g1 无法链接。已登记 `Y0-SETPGID-REGRESSION`(复现 `out/c050-y0/y0-setpgid-regression.md`),修复并经完整候选链与六目标 CI 之前,Y-0 候选不得发布。
- Y-B(e3 按单元续跑)已有精确设计(`plans/y-design-050.md`)未实现;方案 X(工具层分相出片)保留。

## 已知限制与未声称的性质

- cache-manager 两个二进制仍为借用输入(来自 0.0.49 显式输入),其构造未入 candidate-chain。
- `chain-step.cx:134` 的六目标常量顺序与另两处不一致(graph-check order_mismatch=1),改动需候选重建证明字节等价,留待 0.0.51。
- s9 三档本版 APE 均未跑。
- 决策守卫等开发工具链的改动不属本产品。

## 验证入口

1. 核哈希:`sha256sum unisacc.com` 应为 `76ed4200aaaf71b2ac9dd13e6dde6ad31632016ddea8fe43893a4ffe32484967`。
2. `sh ./unisacc.com --version` 输出 `unisacc 0.0.50`。
3. 冒烟:`printf 'int main(void){return 7;}\n' > t.c && sh ./unisacc.com t.c; echo $?` 应为 7。
4. 复现切片:`sh ./unisacc.com tools/candidate-chain.cx -- --rev 979b56e4d918894670a71080970149112499d223 --out NEW_DIR`,六片应与上表相同;在 version=0.0.50 的候选表下 `tools/release-ape.cx -- pack` 应得同一 APE。
5. 原生 CI 证据:见上表运行号。
