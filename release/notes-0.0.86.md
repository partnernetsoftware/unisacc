# unisacc 0.0.86 发布说明

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | 7e0db28a72f66fc96cbc9058c3ec5449024491b8 |
| 发布 APE SHA256(unisacc.com) | e762e82d953fbc39076fbb5736cbfa7760759c50b8ac4a59e53447b437013669 |
| 源快照 SID | 1a4b7bf79eb5a8e3ece0a02666ab36545a845c2d5bae7523c6d2b598ba4cd35e |
| pkg.bin | 2753205de0157d1be5d967e860a9950e52038771145d7befa5b7a2f2d9763505 |
| slice-win-arm64 | e0c20b6afbbc90a5473ac8061a37a3d3323121ebdf8ef69ab543d76b6588a649 |
| SHA256SUMS | ebfb21a722f8a42804603ac3fcac03857ff356441e77c2ba5fd17455750f1205 |

## 严格证据

441 批:PASS 438、FAIL 1、UNKNOWN 2、INVALID 0

严格证据按窗号拼接预跑 prerun086c 已完成的 170 个窗(dvqualify 单提交布局 38feea2ad3,候选同本版 APE),末段接 env-semantics r3(布局 F2,count=84/2)。各批按末行折叠计数。

## 产品改动

- W2 单元级分别编译接线(UNISACC_RUN_SPLIT,默认关,opt-in 为 1;跨单元 ABI 形状具名回落整程序路)。
- D2:C99 约束新拒绝类(隐式函数调用、冲突函数类型等)与回调形参比较 r2。
- Windows 头修复:include/unistd.h 的 unlinkat 在 _WIN32 分支调用 rmdir 前补前置声明(上一次候选 CI 44772879607 的 win/x86_64、win/arm64 smoke 因 D2 隐式函数拒绝失败,该候选作废未公开,失败日志保留为 release/evidence/0.0.86/ci-failed-44772879607.*)。
- 工具侧:tools/check-env-semantics.cx 新增计数行(断言不变),同步到 0.0.87。

## 已知缺口

- env-semantics 2/86 FAIL:APE 启动头对相对缓存根(XDG_CACHE_HOME、HOME 为相对路径)处理有缺陷,属产品缺陷,断言保留继续 FAIL,按已知缺口放行,0.0.87 修复(expires=0.0.87)。
- cx-booter-ape 与 tools-default-regression 未覆盖(UNKNOWN,no-producer:booter-raw-elf、old-checker 材料没有生产者),上一版同样未覆盖,不记 PASS。
- D2 新拒绝类有已知的假接受和假拒绝:函数类型 typedef 形参假拒绝、5 例非首位无名 void 参数假拒绝,以及回调形参内 3 例既有假接受;D2 单元修复在 0.0.87 线。
- 本机严格预跑只在 lnx/x86_64;其余五目标只由 CI 冒烟子集覆盖。第 4 步(全量覆盖)为 UNKNOWN。
