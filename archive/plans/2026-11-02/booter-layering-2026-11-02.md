# unisacc 分层开发模式:booter + 外部基座 + 外部流权重(2026-11-02)

## 现状(已核实)
- `unisacc.com`(现为 624B shell 头)→ `unisacc.com.dev/entry.elf`(加载器)+ `p3-package.bin`(P3/P4 容器,含模型/阶段目录 D/模型 M/资源 F,资源里有各架构 kernel 与头文件);头通过 `UNISA_CONTAINER` 指向容器(src/host.cx:108)。
- 发布形态是单文件 APE(装载器 + 五片 + 一份包 + UNIPKG1 footer)。
- 容器已经是"可分离的部分的集合",只是目前打成一个文件。

## 目标分层
```
L0 booter(APE com,极小)  只做:识别 OS/架构 → 按 manifest 找到并校验 基座+阶段权重 → 交给基座执行
L1 基座(per-arch)         通用执行器("权重换逻辑"的机器,即 kernel);外部文件,按内容哈希寻址
L2 阶段权重(7 流/18 网)   e2/e1/e3/e4/prune/lower/elf 各自的模型文件;外部文件,按内容哈希寻址
L3 资源                   头文件、事实表等
```
**关键:一切按内容哈希寻址;pack 只是 resolver 的另一个来源**。

## Resolver 协议(内置 pack 与外部目录共用,行为完全一致)
- `manifest.tsv`:`name  kind(base|stage|resource)  arch  sha256  size`。
- 来源按序:`UNISA_HOME` 指定目录 → com 旁目录 → 用户缓存 → 内嵌 pack(UNIPKG1 footer)。命中即校验 sha256,不符即拒绝并报具名错误,不回退到下一来源静默用别的。
- **产品身份 = manifest 的 sha256(Merkle 根)**:这与 `plans/identity-unify` 一致,一个原语取代 pp_product_digest/ape-receipt/build.json artifact 段(今天 pack/rebuildcontext 的口径漏同步就出在这里)。

## 为什么这样能"快速固化与分开开发测试"
- 每个阶段权重是独立文件+独立哈希:单独改、单独测。阶段测试 = 输入流字节 → 输出流字节的 golden(接口"相邻成帧"已有测试 interfaces-lnx-x86_64),不需要整包重建。
- 基座与权重解耦:换基座(新架构)不动权重;换权重不动基座。慢车道纪元缩成"只重算被改文件及其下游哈希"。
- 完整 `-o` 构造时间可按阶段归因(正好服务 C44-SPEED-01)。

## 分步(每步有闸门,低风险先行)
| 步 | 内容 | 闸门 |
|---|---|---|
| B0 | `P3 explode/implode` 工具:把现 P3/P4 拆成"目录+manifest",再合回 | 往返逐字节相同(证明 pack ≡ 外部) |
| B1 | resolver 实现(tools 级原型,再下沉 host.cx):按 manifest 从目录加载,run 通过 | `-run` 同输出;外部/内置两种来源跑同一批 smoke 结果一致 |
| B2 | 阶段独立测试台:每个权重文件一个 golden 测试 | 7 流各有输入/输出 golden;改一个权重只重跑它与下游 |
| B3 | 基座外置:每架构一个文件,booter 选 | lnx x86_64/arm64 两个基座各自通过 |
| B4 | booter 成为 APE com 头(替换 shell 头);发布仍可 pack 内嵌 | 单文件发布与目录发布行为一致;启动开销 ≤ 现 -run 热 10ms +20% |
| B5 | 身份收敛到 manifest 根;退役 split1/split2 口径 | check-productpkg 以 manifest 为唯一口径 |

## 风险/待你定
1. 启动开销:多文件打开与校验会增加冷启动;B1 必须测(现热 -run ≈10ms)。校验可缓存(路径+size+mtime 命中后跳过重算,但缓存只存"已校验"标记,不替代首次校验)。
2. Windows/macOS 的 APE booter 写法不同;B4 前先只做 lnx,其余沿用现行并行 pack 形态。
3. 本纪元(步12 起)是否停止:建议**停**。已完成的步 7-11 产物保留作 B0 的真实输入(p3 `a0daec1f`、entry `40a5c98e`、kernel 字节),纪元的"安装/换标"不再做,因为 B5 会改变身份口径,现在换标是白换一次。
