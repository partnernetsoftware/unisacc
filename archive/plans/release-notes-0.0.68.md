# unisacc 0.0.68 发布说明(草稿)

状态:草稿,未冻结、未整链、未公开。凡标"待填"处须以实跑证据填写,不得预填数字。起草者 relprep,2026-11-16,依据 git log refs/tags/v0.0.67..v0.0.68 与 origin/main..v0.0.68(00:19 实测 v0.0.68 = 42caaf6115,尚未合入 main)。

## 版本主题

驱动与运行缓存二期(build/version-seq.tsv 第 5 行)。本版产品源相对 0.0.67 只改 3 个文件(cx/runtime/model_decode.cx、cx/runtime/modelcache.cx、src/executor.cx,共 26 增 4 删),exec/ 只改 exec/facts/derive.tsv 一行,kernel/ 未变。大部分提交是发布门、s9 harness 与开发服务工具。

## 主要变更(已核实提交)

(1) 运行缓存与启动:L4 模型解压校验布局独立计时(25caf2d994);L4 只读映像收益不足,产品实现撤回、负控资产保留(6d0ec21a59);500 ms 启动门在 0.0.68 不可达的实测结论,门改判到后续版本(d564eda04a)。本版不宣称启动时间达标。
(2) s9 发布门整改:材料映射修复与 s9map 库(e7e77059cd、4eba850364);F4c 共约 128 批由 daily 档移到 release 档,只改 tier 列(198ba2e64f、fa77aa97d9、a52b57f67e);F7 冻结根一条命令 tools/s9-root.cx(54eaf4b064);单片窗口改读片声明预算(42bb504397、5a071a43df);平台不属本机的批在计划阶段记 NOT_APPLICABLE(1c52ddf1aa);降并发与冻结根补带 unisacc.com.build.json(21f95007be)。
(3) 发布门漂移修复:tools-compile 已审清单补登 133 个新 .cx 源并补 17 个分片批次(e43b97ecbf);pins、chain-argv-check、graph-check 等四项(61e730f9d8);lab-loader 与 release 的模块改经仓根 cxroot.tsv 解析、ape-pack 壳头在低虚存时补回具名文案(6929ca4d91);watch-bundle、stream-prune-cases、source-bom、random-expr 四项(66505c70d7)。
(4) 发布打包:release-run --pack 核验真实整链固定点与续跑身份(42ebecc749),即方案 A 第 4 步公开硬门(tools/release-run.cx:143-190)。
(5) 构造链:candidate-chain 主流程收成静态步骤表(3a158d2b1c);chain-phases 入派生总闸(bc6df42c29)。
(6) 开发服务 devsvc(写窗口锁、原生执行体缓存、flow 引擎、文档算子)与提交门降本(9f3571972e 等)。属开发基础设施,不进产品。
(7) 编译覆盖缺口:本版只含第 0 期,即普查工具 tools/notcovered-census.cx、底表 tests/notcovered.tsv 与 38 个复现(5d2b4f9717、ade1284a06、ff988e122a)。修复(第一期起)在分支 v0.0.69,不在本版;按决定 0.0.68 不为此打补丁。

## 资格与证据(待填)

| 步 | 内容 | 结果 | 证据 |
|---|---|---|---|
| 1 | 冻结门复跑 | 待填 | 待填 |
| 2 | 整链与固定点(四类 same,g2/g3 六切片 SHA) | 待填 | 待填 |
| 3 | 打包 | 待填 | 待填 |
| 4 | s9 commit 档(不挡公开) | 待填 | 待填 |
| 4 | s9 daily 档 | 待填:第一次真跑 NOT_QUALIFIED(冻结根 5a071a43df),第二次真跑在 42caaf6115 根上进行中 | 待填 |
| 4 | s9 release 档 | 待填 | 待填 |
| 5 至 8 | 布局、草稿资产、同候选 CI、读回 | 待填 | 待填 |
| 裁决 | QUALIFIED 或 NOT_QUALIFIED | 待填 | release/decision.txt |

## 已知缺口(如实)

(1) release 档域闭合:full-* 17 项按设计报 OPEN(含 NEEDS_FOREIGN_HOST 与 BLOCKED 条目);release 档单独聚合分支(候选池 S 的 F2)未实现。
(2) 无候选材料类批次:old-product、release-ctor-location 等约 60 批在首次真跑中为 UNKNOWN(材料未备);F4b 生产者类约 39 批同样 UNKNOWN。未手工改标。
(3) run-reserve 与 compile-as-limit 的 low-named-refusal:修复在 ape-pack 壳头(6929ca4d91),需新壳头产品(本版整链产物)上复测;复测前不记通过。
(4) random-expr:本机无宿主 tcc,批现报 HOST_TOOL_MISSING rc 3,不冒充通过。
(5) daily 真跑:第二次真跑结论待填;第一次的 UNKNOWN、FAIL 分类见 plans/release-gates-0.0.68.md。
(6) slice-migration-gen 输入表引用已删的 cx/compiler/frames.cx,待复核;streams-material 批往根内写目录不清。
(7) 启动 500 ms 门未达(见变更 1)。
(8) 未跑范围:跨平台仅由同候选 CI 覆盖(待填);osx cache-manager 借用副本来源未证实;独立参考与原生 watchdog 仍有宿主 C 引导依赖,构造协调器仍有 Python 债。

## 资产

待填:unisacc.com、pkg.bin、slice-win-arm64、unisacc.com.build.json、build.json、SHA256SUMS 及各 SHA256(以 release/candidate.tsv 为准)。

## 回滚

待填:release/rollback.txt 按 0.0.68 重写后引用(v0.0.68 回草稿、v0.0.67 恢复 Latest、前向回滚 X/Y1/Y2、本机装机回到 0.0.67 APE)。
