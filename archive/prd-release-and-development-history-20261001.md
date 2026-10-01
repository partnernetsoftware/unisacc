# prd 历史身份与实施记录（截至 2026-10-01）

此文件保存从 prd.md 移出的历史原文；当前状态、架构与路线仍以 [prd.md](../prd.md) 为准。

## 逐版发布身份

### 已发布版本身份（README 只放用户入口；本表按版本一行，数字以各版验收记录为准）

| 版本 | 身份 / 证据 |
|---|---|
| v0.0.17 | **已发布 2026-10-01**（主人预先批准）。Version 0.0.17；源 `ae3f989`；候选 `5c4802c2…` 1,452,870 B；公开签名 `unisacc.com` `cdbd714c…` 1,468,648 B；公证 dmg `986a5fe7…`；回执 research/r17-release-acceptance.json |
| v0.0.16 | **已发布 2026-10-01**（主人确认）。公开资产只有签名 `unisacc.com` 与公证 dmg。Version 0.0.16；结项提交 `313610b`；候选 `unisacc.com` 1,295,312 B，SHA-256 `6fc720265f9a9573…`（与自举 stage2/stage3 字节相同，GHCR 重封 `10e9ca04…`）；已做而未公开：Azure 公司签名（run 36813357447，签后 `c1c72e2f90fd04b7…` 1,311,088 B）、Apple 公证 dmg `eb433bffa309c762…`；回执 research/r16-release-acceptance.json |
| v0.0.15 | **未发布**（整理版）。结项 `d6a5046`；产品字节与 0.0.14 候选相同 |
| v0.0.14 | Version 0.0.14; release source `75ec4de`; candidate `3d86639072f5d562…` 1,288,064 B; published `unisacc.com` Authenticode-signed 1,303,840 B, SHA-256 `c229cebf6cd58ca0…`; dmg `9f13acc4b87d6100…`; receipt research/r14-release-acceptance.json |
| v0.0.13 | Version 0.0.13; release source `3af8b36`; candidate `94ea9c76f9cc62f1…` 1,190,297 B; published `unisacc.com` Authenticode-signed 1,206,080 B, SHA-256 `75c698ecb71f287c…`; receipt archive/research/r13/r13-release-acceptance.json |
| v0.0.12 | Version 0.0.12; product closure sealed at `c4d667e` (candidate GHCR digest in `release/candidate.json`); published `unisacc.com` is Authenticode-signed: 1,170,368 B, SHA-256 `fb607af59388aa20cbd9f536d6b781f3cf83a6a0294c0e4e69e844faba2736cb` (unsigned gate candidate 1,154,589 B, SHA-256 `df8cc9b4a3d997ea9e33bbdbaefeecb1a11d10dd98f529a4fb8d87f2e21e7f7a`); macOS app/dmg Developer ID signed and notarized; the same candidate bytes ran the demo suite on six hosted runners (lnx/osx/win × x86_64/arm64) and full suites on four native architectures; receipt [archive/research/r12/r12-release-acceptance.json](research/r12/r12-release-acceptance.json) |
| v0.0.11 | Version 0.0.11; product closure sealed at `8b5abc9` (candidate GHCR digest in `release/candidate.json`); published `unisacc.com` is Authenticode-signed: 1,170,384 B, SHA-256 `e86cc61c2d9ee8abd511f5d6b5c0f114a01a34dbe6146bb411e3f204c65d792a` (unsigned gate candidate 1,154,605 B, SHA-256 `6a3dfce28aa05ca474442ebe9e6fc4d07f4da7c15d1d3b5a6c21e91290f920c8`); macOS app/dmg Developer ID signed and notarized; library bodies on demand by default (`-fno-trim-libc` opts out); receipt [archive/research/r11/r11-release-acceptance.json](research/r11/r11-release-acceptance.json) |
| v0.0.10 | Version 0.0.10; product closure sealed at `fdff9c5`, release source `ae6d512`; published `unisacc.com` is Authenticode-signed: 1,168,488 B, SHA-256 `f6e8e090a5288583389bdbbb5674f7e0fbb717baf13fa600f8074c77d1acdb2e` (unsigned gate candidate 1,152,711 B, SHA-256 `4ba24140a1307a34216efd0f2e7c892a92990ee2729a8774a58e0a4fe2315002`; model package identical); 24 deployed networks each `network = table` over the whole domain; in-process `libunisacc` (contexts, symbol injection, typed V2/V3 signatures with model-certified carriers, callbacks, USLCALL3 source-origin aliases); local release gate 332 suites rc 0, Linux arm64 guest and Windows/x86_64 guest smoke recorded in the [release receipt](research/r10/r10-release-acceptance.json) |
| v0.0.9 与更早 | 见 [早期版本沿革](prd-history-20260929.md) |

## R19 开发决定

0.0.19 R19-7 实测（2026-10-01）：E3 的结构体返回局部标识符专用入口遇到 `return parse(...)` 会落入普通“identifier is not a local”拒绝。直接改走通用 `EXPR` 的私有试验虽接受输入，却多写调用前栈槽；含 `char[16]` 的结构体还把 24 字节误算成 72 字节，不能作为产品修复。本版先按构造具名拒绝，保留该组合的功能缺口；生成器与更完整的组合矩阵由 R19-7 继续跟踪。

0.0.19 R19-8 产品侧决定：E3 在 `GV.record` 以符号 ID、单元序号和存储类记录带初值对象定义；同单元第二次报 `redefinition of this object (it already has an initialiser)`，一步多文件的跨单元外部同名定义报 `multiple definitions of this object across units (each has an initialiser)`。`FN.def1` 同样记录函数定义，重复的 `main` 等外部函数报 `multiple definitions of this function`；不同单元的同名 `static` 各自独立。暂定定义不设置初值标志；对象输入链接仍由 tape 链接器检查。

## 结构测量与纠错

两个 e3 分别是错误与错误+告警变体；十二个 e2 是六目标的普通/位置变体。共享物理模型数与阶段行数以本节的生成账本为准（见上方 `model-bytes` 区：24 个共享网络体、1,088 条阶段行），本段不重述；此处曾写「33 个共享物理模型被 1,082 条阶段行引用」，与同一文件上方的生成区不符。引用次数不是物理份数。压缩前 Q/C 是动作与共享声明，H 是阈值/选择参数；三个数字不与压缩模型体相加。21 份头/库源码与两 ISA 核资源另计；混合平台驱动缺独立 link-map，不虚构其 libc/启动/OS 子项比例。当前产物大小相对已发布 v0.0.8 的 5,388,402 B 小约 77.11%，签名后字节变化另记；产物与包的当前字节同样以生成账本为准（本段曾写 1,233,236 B 与 985,172 B，那是 v0.0.9 的值）。

---

**逐阶段结构计数口径**：论文 §1.3 的结构快照见 [pipeline-structure-20260928.json](research/20260928/pipeline-structure-20260928.json)，绑定旧01e5c1d9测量基线；当前d4f7d302静态结构见[r9-pipeline-structure-prune.json](research/r9/r9-pipeline-structure-prune.json)，旧calc动态计数不得外推。静态动作数按每个序列展开 Q/C 前缀后的动作次数求和；另记包中实际保存的后缀动作数。它们不等于一次程序运行的动态动作数，也不是动作操作码种类数。声明返回按银行数与键成员数另记；O1/O2 不能共用一组结构数字。结构审计另附 calc 动态计数：cc-unisacc 提供插桩/原始输出，cdx 用同一二进制独立复跑，五阶段计数一致；输入、模型与运行时 SHA、O0/run 与 memory 路由及日志归档到 research/pipeline-counts-20260928/。它不是出货汇编内核的性能测量，结构表中的 O2 网络未参与该次 O0 运行。

## R18/R19 实施回执

### R18-10 产品侧参数形状（2026-10-01）

E3 从 INTRINSIC/INTRINSIC6 读取新内建，不另造名单；P3 头资源挂载递归覆盖 include/sys/。lower 的 zero4 以 code-abi-sources.tsv 声明三参加零，继续由通用动作与 ABI 表生成。Linux ARM poll 无号，使用 ppoll；不把拒绝算通过。首片 Linux/macOS ARM 的 typed lowering 在 C 与 Python 执行器上与参考逐字段同；完整产品与六目标验收待后续。

### R19-10 产品侧转发描述符（2026-10-01）

仅 macOS `-run` 设置资源 `\0cli/run-forward`。E3 保留通常的原型事实和未定义调用检查；有资源时，首次遇到可达且有原型的未定义调用，才把名称与既有 `USLSIG3` 签名包进 `USLFW1` 长度帧侧车。无资源时原 tape 与拒绝契约不变。驱动只解帧、检验签名并调用共享桩文本生成器；不会从调用指令猜参数类型。变参、结构体按值等未支持形状按名拒绝。侧车与产品驱动尚需联测，不将此片记为 R19-10 完成。

### 0.0.19 自源码门禁限时拆分（2026-10-01）

`selfcheck` 的逐阶段成像和打包成像分别对同源参考镜像比较；macOS 的 N1=N2=N3 另用参考 N1 独立验证，逐阶段套件已证明它与网络 N1 同字节。产品驱动的自身网络构建也从 27 个模式组合中拆出。完整门禁保留两种成像、自举与模式组合的全部断言；每个新 job 独立准备可验证的输入，不依赖另一 job 的临时目录或执行顺序。起因是源码增长后旧单 job 超过队列 48 秒额度；全量时间与结果待候选重测。

驱动自编译另触及 `run.c` 默认 2 亿步燃料限制（rc 3 `timeout`），这与墙钟看门狗不同。该项采用与完整源码成像相同的 4000 亿步燃料上限，仍由 60 秒墙钟看门狗约束；在本机暖缓存下，拆出的 `core-build` 与 `core-modes` 分别通过，后者 27 个模式比较全部保留。E3 宿主转发模式缺原型时恢复既有的按名 `undefined function` 诊断，与参考一致。
