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

- D1/D2:C99 声明不一致带位置告警及 -Werror 拒绝路径接入;跨单元 typedef 函数指针签名比较与无名非首 void 形参约束修复(16 个 exec 表与对应生成器同步)。
- E1:APE 启动头对相对缓存根(XDG_CACHE_HOME、HOME 为相对路径)按启动时工作目录归一化为绝对路径,0.0.86 到期缺口 env-semantics 转绿(86/0 PASS)。
- 列车与开发服务:prerun-drive 按历史耗时装窗与串行恢复、release-chain freeze-conflicts 自动处理已退役出库表冲突、ci-early 候选六目标前置验证、devsvc search 与固定版本缓存分域。

## 已知缺口

- cx-booter-ape 与 tools-default-regression 未覆盖(UNKNOWN,no-producer:booter-raw-elf、old-checker 材料没有生产者),上一版同样未覆盖,不记 PASS。
- 本机严格预跑只在 lnx/x86_64;其余五目标只由 CI 冒烟子集覆盖。第 4 步(全量覆盖)为 UNKNOWN。
