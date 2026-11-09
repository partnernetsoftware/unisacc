# unisacc 0.0.49 —— 发布说明

本版主题是**构造提速与可复现发布**,产品的 C 语言行为基本不变。产品证据是同候选六目标原生 CI(见下表);**全批会话 s9 只跑了提交档**,daily 与 release 两档未跑,不称全批通过。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`(APE 加载头、六个 ISA 切片和模型包)。`sh ./unisacc.com --version` 输出 `unisacc 0.0.49`。

## 身份与证据

| 对象 | 身份 | 说明 |
|---|---|---|
| APE `unisacc.com` | SHA256 `2d3852a8404e1ba372de0fd28e69d6da7d9412d7b8b259d0d2e36593dac8e44d`,6501348 字节 | 同候选六目标原生 CI 的受测对象 |
| 冻结源 | `979b56e4d918894670a71080970149112499d223`,源 SID `e764a816…`(source-packages-v2,十二包摘要组合) | 版本串不在源 SID 中 |
| 构造器 | `8224ff13…`(Linux x86_64 片) | 各代内核、切片收据的构造器均为此值 |
| 模型包 `pkg.bin` | SHA256 `56efea531e104b4114b314d5bdb4e5efc2a6debb96f974fa5d6e32675a637db8`,带 `UNIPKG1` 16 字节尾 | |
| `slice-win-arm64` | SHA256 `d99940a474afe2b9387e01102ee7ff54ed7ad6e83459b46453ca2c95ff612ed9`(已注入 0.0.49 版本槽) | |
| 两代不动点 | g2 与 g3:85 个目标文件、2 个内核、6 个切片、产品均逐字节相同 | |
| 最终 APE 内容 | 内嵌六片与 P4 两核经 11 项内容核对,与 g3 及规范内核一致 | `out/c49-ape-content-results.tsv` |
| s9 提交档 | 同候选 96/96 PASS(两 step 255.13s + 61.94s,child 工作树扫描 0 次) | daily、release 档未跑 |
| 原生 CI | **QUALIFIED 42/0**(六目标各 7 格全 PASS,0 失败/0 未跑/0 无效) | 运行 `42661106290`(测试提交 e5b26c5c);首轮运行 42659892210 因资格工具未迁移版本源,四个 POSIX 格 rc2(Windows 两格通过),修复后重跑 |

## 行为变化(升级前请读)

1. **版本串移出源 SID。** 版本只来自 `release/candidate.tsv`,在打包时注入;源 SID 改为十二包摘要组合。发版不再因为改版本号而重建整条构造链。依赖旧 `exec/facts/limits.tsv` 里 `PRODUCT_VERSION` 行的外部脚本需改读候选表。旧 `nninputs`/`shared_gen` 缓存缺 v2 字段会被拒绝并重建。
2. **单元数超过 16 的程序现在也进运行缓存**(上限改为 128,等于 `DV_MAXU`)。大工具第二次运行不再全量重编;候选上 20 单元程序第二次运行命中(`lookup_hit=2`)。

## 本版修复与新增

- **构造链:** `chain-step fixedpoint` 改为完整六片合同并支持续跑;内核相位显式传递构造器(此前内部硬写已装产品);子进程调用补 `--` 分隔。
- **Mac/Win whole-o 分相:** 单次超过 60 秒上限,改为在产品公开阶段边界上分相续跑(Mac:前端分波 + 首遍 e3 + 镜像;Win:`st_span` 三相),每相 ≤45 秒。
- **S9:** child 改用会话内预编译执行器;身份测量继承父会话;工作树扫描改为 wave 级共享;`wt_diff` 平方级修复;有界回填。
- **发布与冻结工具:** `tools/freeze-plan.cx`(冻结前缀由真实计入文件导出,1638 文件/41 别名/52 前缀,未覆盖为 0)、`tools/release-decide.cx`(8 步裁决,缺证据即 UNKNOWN)、`tools/integrate-patch.cx`(补丁集成门)。
- **测试修正:** `runcache-units` 两处缺陷(退出码断言、指纹比较当前对当前);`host-cell` 与 `candidate-qualify` 改从候选表读版本;`pp-autoinc-gen` 表头基线红。

## 已知限制与未声称的性质

- **s9 只跑了提交档**;daily 与 release 档没有运行。提交档仍需两个 step,未达成单 step ≤300 秒。
- **Mac 与 Win 切片由分相编排产出。** 本地证据是:同源默认路径的中间产物(前端帧、转发桩)逐字节相等,以及同 ISA 中等程序「默认一次性 vs 分相」镜像逐字节相等;最终资格以原生 CI 为准。旧双相位 B 路径(会把单元数置为 1)产出的镜像已撤回,不用于本版。
- **贴线:** Mac Intel 的 B 相构造本体 43.138 秒,距 45 秒子相上限约 2 秒,负载升高可能超限。根治方案(e3 产品原生阶段化)排在 0.0.50。
- **守卫用旧产品:** P3 opguard 与 kernel-build helper 仍用已装产品做守卫比较(不注入产物字节),0.0.50 修复。
- **未做:** 产品消费共享传输帧(SPEED-049-B 第二步)、`seed/gen.c` 拆分(H)。
- **与 0.0.48 镜像的结构对照**只作信息记录(源码不同,28 处差异),不作资格证据。
- 冻结到发布的墙钟目标(≤44 分钟)**本版未达成**:实际十余小时,主要是相位间的人工/代理间隙与临时设计 Mac/Win 分相方案;端到端计算之和约 31 分钟。

## 验证入口

1. 核哈希:`sha256sum unisacc.com` 应为 `2d3852a8404e1ba372de0fd28e69d6da7d9412d7b8b259d0d2e36593dac8e44d`。
2. `sh ./unisacc.com --version` 输出 `unisacc 0.0.49`。
3. 冒烟:`printf 'int main(void){return 7;}\n' > t.c && sh ./unisacc.com t.c; echo $?` 应为 7。
4. 资产校验:下载全部资产后 `sha256sum --check SHA256SUMS`。
5. 原生 CI 证据:见上表运行号;证据表在其 `evidence-<os>-<arch>` 产物里。
