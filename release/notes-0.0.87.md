# unisacc 0.0.87 发布说明

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | a7cd95024a5641489ed0008cf38f605327fa1f2c |
| 发布 APE SHA256(unisacc.com) | f6cb014376d7d4944c9a3f7a9252d156f7f32c08a2c0d5beb22f7098ae19e30e |
| 源快照 SID | e5bda6fd61e9c64dfc7dccabdde64c9b61c6156434e4b9f141af8692741364f7 |
| pkg.bin | 87b93943f2e43f78d06bb5f657120d23a2b3096fda9114c846282e9962b6c019 |
| slice-win-arm64 | 70d15005a82b0508c7180d7b4e0702b56e7b8248204a62e23b1a04f0067ecf6a |
| SHA256SUMS | 2b63701d83072899c31a74f671d9b64483a49f59b37e056613193f09e76f05d8 |

## 严格证据

441 批:PASS 439、FAIL 0、UNKNOWN 2、INVALID 0

严格证据按窗序合并预跑 prerun087-run(首轮 97 窗)与 prerun087-run2(新版按耗时装窗 57 窗)及单批复验,候选同本版 APE(f6cb0143)。0.0.86 到期缺口 env-semantics(86/0)实测转绿 PASS,全集 439 批 PASS、0 批 FAIL。各批按末行折叠计数。

## 产品改动

- D1/D2 收尾:C99 四类声明不一致(结构体标签、const 限定、回调等级、形参等级)带位置告警及 -Werror 拒绝路径接入;跨单元 typedef 函数指针签名比较、无名非首 void 形参约束及回调形参比较修复(16 个 exec 表与对应生成器同步)。
- E1:APE 启动头对相对缓存根(XDG_CACHE_HOME、HOME 为相对路径)按启动时工作目录归一化为绝对路径,0.0.86 到期缺口 env-semantics 转绿(86/0 PASS)。
- 列车与开发服务:prerun-drive 按历史耗时装窗与串行恢复、release-chain freeze-conflicts 自动处理已退役出库表冲突、ci-early 候选六目标前置验证、devsvc search 与固定版本缓存分域。

## 已知缺口与未跑范围

- 两个 tier=release 批(cx-booter-ape 与 tools-default-regression)未覆盖(UNKNOWN,no-producer:booter-raw-elf、old-checker 材料没有生产者),与 0.0.86 一致,不记 PASS。
- 首链一次失败与原地续跑:整链 ee595061e7 在 g2-lnx-arm64-closure 因 /tmp/.git 短暂存在被 tools/chain-step.cx 的 cs_fp_repo_of() 向上遍历至 /tmp 误判为脏 git 工作树而 rc=2(非候选缺陷,已登记 0.0.88 工具项 CS1);清理后 A 类零改动原地续跑 DONE。
- 预跑切换与补跑:首轮旧版未传 --times-file 跑完 97 窗(收 117 个 PASS)后切新版按耗时装窗跑完剩余 322 批 57 窗(prerun087-run2);首轮 W0000 的 4 个非资格 warmup 批(modelcache-budget-lru、modelcache-budget-concurrent、precommit-toolsloc、precommit-toolsloc-source)单批 jobs=1 补跑 4/4 PASS。
- 检查侧对齐:tools/gen-all-check.cx 排除未入 derive.tsv 的非产品分片生成器 registry-gen.cx(a1383ace9c);tools/check-xunit-fnptr.cx 对齐 FS.audit.tag.compare 规则行匹配、tools/check-real-commit.cx 对齐 DV_PIN_DIR 与 DEVSVC_NATIVE_DIR 缓存目录继承(ac18bfe096)。均属检查侧,候选 APE 身份 f6cb0143 不变。
- 未跑范围:本机严格预跑只在 lnx/x86_64 执行;osx/x86_64、osx/arm64、win/x86_64、win/arm64 与 lnx/arm64 原生仅由 CI 六格冒烟子集覆盖(第 4 步全量覆盖为 UNKNOWN);core 库与 U3 等 0.0.88 线改动不在本版。
