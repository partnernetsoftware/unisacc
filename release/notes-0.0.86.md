# unisacc 0.0.86 发布说明

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | cdc49bfe7fb1a3ff344807ad93f4e3396b7cd340 |
| 发布 APE SHA256(unisacc.com) | ad7bc06bfde456eb4a89f7b035ac7b9bd8f86b258ba824ff85308f55169b442a |
| 源快照 SID | 65203330f7e878f563ae650a9734d7c135412ebcc000e0c5ef4df21f2637b737 |
| pkg.bin | a3297a953dd8ef268558275d99862ff1d24ff3f402289463861004f12459ad26 |
| slice-win-arm64 | e0c20b6afbbc90a5473ac8061a37a3d3323121ebdf8ef69ab543d76b6588a649 |
| SHA256SUMS | 2c71ad4faecde65965c617c2acd3b206922f850f151e641941e630b39ae48325 |

## 严格证据

441 批:PASS 438、FAIL 1、UNKNOWN 2、INVALID 0

严格证据按窗号拼接预跑 prerun086b 已完成的 106 个窗(dvqualify 单提交布局 2c9dbac43e),另接 W0074-r2(opt-frame4 四批实跑 PASS)、xunit-fnptr r2(布局 156c68f5a8,检查正则修复后实跑 PASS)与 env-semantics r3(布局 F,count=84/2)。各批按末行折叠计数;被后续 PASS 取代的 FAIL/UNKNOWN 行保留原样,由 release-exec G1 判 SUPERSEDED。

## 产品改动

- W2 单元级分别编译接线(UNISACC_RUN_SPLIT,默认关,opt-in 为 1;跨单元 ABI 形状具名回落整程序路)。
- D2 回调导入 r2:parse2 control 与 enumtypes 结果表更新,新增 d2-callback-import 用例。
- 冻结提交 F 在整链输入 2c9dbac43e 之上只有非产品改动和 SID 不变的改动(ext/crypto/p256.cx 加 2 行与 exec/k2-design.md,F 归档 SID 与整链相同,pack SOURCES_SID VERIFIED);另含 tools/check-env-semantics.cx 新增计数行(断言不变),该检查修复同步到 0.0.87。

## 已知缺口

- env-semantics 2/86 FAIL:APE 启动头对相对缓存根(XDG_CACHE_HOME、HOME 为相对路径)处理有缺陷,属产品缺陷,断言保留继续 FAIL,按已知缺口放行,0.0.87 修复(expires=0.0.87)。
- cx-booter-ape 与 tools-default-regression 未覆盖(UNKNOWN,no-producer:booter-raw-elf、old-checker 材料没有生产者),上一版同样未覆盖,不记 PASS。
- D2 新拒绝类有已知的假接受和假拒绝:build/pkg-gaps.tsv 登记的 typedef 假拒绝,以及 5 例非首位无名 void 参数假拒绝;D2 单元修复在 0.0.87 线。
- 第 4 步(全量覆盖)仍为 UNKNOWN:严格证据只覆盖所列批次。CI 与公开读回(第 6 到 8 步)发布前为 PENDING。
