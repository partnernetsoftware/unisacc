# unisacc 0.0.81 发布说明

状态:同候选原生 CI run 44490199376(六目标 success,一次通过无重派,234 秒)。整体裁决 NOT_QUALIFIED:全套 s9 本版未跑,产品资格域未整域实跑,严格证据只覆盖下列批次,不得称全批资格。

## 真实身份

| 对象 | 身份 |
|---|---|
| 候选点 T(整链输入) | 63ba58f9f782503c9b1a2a2a26e01adc7c07d006(第五次起链整链 499/499 rc 0,约 25 分钟) |
| 冻结提交(发布标签目标) | 8b46bf21ee02f56f263bd3ad3dd126e0e35e8dc2(相对 T 产品输入零改动) |
| 发布 APE SHA256(unisacc.com) | 635a417234ed5642b27752d233fffca22e2693a09dc10c685369c9e4354124b5(7390117 字节,比 0.0.80 多 1169 字节;--version 打印 unisacc 0.0.81;打包后与链产物同字节,PACK_APE SAME) |
| 源快照 SID | 1d375835616e863cfa891779e8e8cd83b0d8d9cd2ed7ca3e68128720098c3f8a(VERIFIED,T 的链快照 63ba58f9f7-20966f22) |
| pkg.bin | 3c1dd33d15a1ff293bc5b718ff2773f0ddd4305025b166542dc2b1454cab530e |
| slice-win-arm64 | c8dc032ba78c482f839caa75c064e07b8ed142dec374e9dffadf4dc1779e3fdb |
| SHA256SUMS | 355f3c8954e3c90ad3039402fb4cf0b49b35cddfe2d502ee2d1a85beb431c797 |
| CI run | 44490199376 |

## 产品改动

产品输入四处,整程序 -o 路零变化:17 份 -o 产物与 0.0.80 公开件逐字节相同;对象产出(-c 与 -c -funit)按下列变字节。

(1) lnx/arm64 的 -funit 对象入口桥(AAPCS64):0.0.80 公开后补测发现 arm64 -funit 对象带 .init_array 但 __init_u 没有 AAPCS64 入口包装,链接这类对象的 arm64 程序启动即段错误 rc 139(0.0.80 说明已更正)。本版 librarymodule-manifest 新增 unit 节 arm64 变体:对外名 hentry x7 -> call __ub_NAME -> hleave,单元调 extern 用 x0..x5 存 x7 下 48 字节块加 hostcall(sp 16 字节对齐由 hostcall 门保证);__init_u 经同一座桥,libc 经 .init_array 调入不再崩溃。门控 object 且 arm64 且非 osx 非 win 且 of_unit=1,整程序网不含。check-objabi 新增 10 个 arm64 例用 qemu-aarch64-static 实跑:基线产品 failed=11,新产品 failed=0 expect_fail=4(p03 51、多对象同链 51、单元调 gcc 带参函数 82、栈对齐例 43)。提交 ffa68b8549;证据 release/evidence/v0.0.81-f2arm-2026-11-19.tsv 与 v0.0.81-f1barm.tsv。
(2) __init_u 符号类型 NOTYPE 改回 FUNC(LOCAL FUNC):objectplan d0.s23 与 d1.s24 各加 4 行,x86_64 与 arm64 同改;x86_64 的 -c -funit 仅此 1 字节差(readelf 核);tests/objectplan-pair-allow.tsv 行数 48/40 -> 51/43,pair new_diff=0 自检 8/0。
(3) stream 子命令步数预算耗尽返回 rc 3(提交 3a31e1964f):整域首测批次 stream-e4-io 的 e4.ref.step-budget 定因为产品缺陷,dv_stream 把 xs_run 的 3 一律当 NOT_IMPLEMENTED 返回 2,自 7c84ebc669 stream 子命令起长期存在,旧 run.c 合同是 rc 3。修后 3 且 why 非空仍是具名边界缺口返回 2,3 且 why 为空是预算耗尽返回 3。先红后绿:基线 pass=10 fail=1,新开发产品 pass=11 fail=0。
(4) 对象变字节清单:lnx/arm64 与 lnx/x86_64 的 -c 与 -c -funit 各 17/17 变字节;osx 与 win 不变。优化档整程序镜像:0.0.80 对本候选 30 格(五源组各一档,六目标)逐字节一致,30/30 PASS(cdx,证据 netregen-opt-compare.tsv)。

## 开发工具链(不进产品)

(1) cx-dev 重建 lnx/arm64 配方(38a86c4799):原 native=0 来自验证范围而非宿主不能构造;开放 Linux 的 lower 与 enc/arm 四个配方,osx、win、unitcontainer 仍 NOT_CLAIMED;arm 对象先变后恢复,编码重建约 8 秒;独立复核有条件放行,build.json 记录问题见缺口。
(2) 整域首测基线:0.0.80 公开件对 609 批整域,首轮 PASS 489 FAIL 7 UNKNOWN 113,二轮 PASS 493 FAIL 7 UNKNOWN 109(outer-timeout 0,load-wait-timeout 0);109 个 UNKNOWN 定因:83 no-producer、15 平台不适用、9 非 IMPLEMENTED SOURCE、1 kernels 多相备料、1 缺主机 tcc。
(3) 6 个新 FAIL 批全部是批次或材料缺陷或期望过时,不是 0.0.80 回归,均已修:tsvx-table(标记后空格)、run-tmpfile(运行目录移到 /tmp)、stream-prune-io(夹具缺 .end)、interfaces-lnx-x86_64(垫片三缺口与过时锚点,覆盖由 241 增到 279 条断言)、stages(.frame 回填槽 7 -> 10 宽后期望未重生成,新 tools/stages-regen.cx 带 --check 与 43 个负控 sha256 守卫,elf 锚点逐字节一致)、stream-e4-io(产品缺陷,见产品改动 (3))。
(4) old-product 材料生产者(tools/old-product.cx 固定公开资产下载与完整性缓存,29 项离线检查;tools/old-product-material.cx 备料与 gate,10 项):30 个 netregen 批经 dvqualify 30/30 PASS(旧件 0.0.79 对 0.0.80)。affected-oldproduct 按产品版本推导前一版(806faf002f、e2961f8076)。
(5) ref 类冻结经典参考(提交 500d6cf3d8 复建 ua_ref,独立复建与 cdx 收据逐字节一致);单提交资格布局的冻结参考持久缓存(661f967ca3)。stream-e4-o2 PASS;其余 20 个 ref 批旧判据失败,见严格证据与已知缺口。
(6) realprog 接回(tools/realprog-fetch.cx 加 check-realprog.cx,kilo、jsmn-simple、jsmn-dump、cJSON 四项与 cc 对拍 pass=4);Lua 与 SQLite 已知被产品拒绝不在本批。
(7) 仓根升级到 0.0.80(install-ape);plans/ 归档 58 个旧文件;table-lint、handwritten-census 检查。

## 严格证据

(1) 预跑(T3 布局 35be2fcca4,候选 APE 635a4172,dvqualify jobs 2):262 批。
(2) 补跑(T4 布局 f329ffb2ad,ref 缓存与 affected-oldproduct 修复之后,同一候选 APE):28 批,覆盖 ref 类批与 affected-oldproduct。
(3) 拼接:整合文件 release/evidence/v0.0.81-strict-2026-11-19.tsv 为预跑段(strict2.tsv)与补跑段(strict3.tsv)原样拼接,各段保留自己的 source/layout/seed 注释头,不删不改任何行;全部 BATCH_RUN 行 candidate=635a417234ed...,5 处 version unisacc 0.0.81。同一批在两段都有行时以后段为准,affected-oldproduct 的 T3 段 FAIL count=5/4 由 T4 段 PASS 取代(G1 记 SUPERSEDED fail_row=5 pass_row=671)。
(4) 最终 263 批:PASS 238、FAIL 21、UNKNOWN 4、INVALID 0。
  - FAIL c-negative 53/9:与 0.0.76、0.0.79、0.0.80 同批同数,产品接受 9 个应拒的 C99 负例,非本候选回归。
  - FAIL 20 个 ref 旧判据批(optimization-closure 10/30、optimization-corpus-0..9 各 6/36、properties-cases-layout-len12 0/42、properties-cases-layout-len3-0..5 各 0/36、properties-cases-lower 1/12、properties-cases-random 17/26):判据为冻结参考 500d6cf3d8 的字节输出,X3 起产品指令选择有意变化致输出不同,与 cdx 标准布局收据一致;均登记 expected-fail.tsv,到期 0.0.82 换判据。
  - UNKNOWN cx-booter-ape、tools-default-regression、stream-e4-io-cc、stream-prune-io-cc:材料无生产者(no-producer),记 UNMEASURED 不记 PASS,expected-unmeasured.tsv 引用本次真实 DEFERRED 行。
(5) 预热与干跑:N4 读回预热 WARM READY(5 个目标);release-exec --dry DRY_GATES_OK,G1 STRICT PASS(expected_fail=21 superseded=1),CANDIDATE_SHA、APE_VERSION、RATCHETS 15/0、MAIN_ANCESTOR 均 PASS。

## 整链

第五次起链 499/499 rc 0,约 25 分钟,链目录 /tmp/unisacc-session-v081-chain3/63ba58f9f782503c9b1a2a2a26e01adc7c07d006。前四次起链失败共约 33 分钟,均为环境原因:版本工作树根的 unisacc.com 与 build.json 为非常规文件,以及整链快照缓存复用;登记缺口 WORKTREE-ROOT-FILES-MUST-BE-REGULAR 与 INSTALL-APE-RECORD-STALE-COMMIT。

## 已知缺口

(1) arm64 -funit 对象:x7 内部栈在 sp 之下,单元执行期间异步信号(带处理函数且未设 sigaltstack)可能覆盖;hentry 不保存 x19-x28 与 d8-d15,当前单元代码只用 x0-x17 但无守卫;只支持前 6 个整型参数,浮点与栈参数与单元内取址外传回调未做(同 x86_64);缺口 OBJABI-ARM64-X7-STACK-BELOW-SP-SIGNAL,0.0.82 起修。
(2) stream 预算码靠 why 是否为空区分属隐式约定;src/drive.cx:780 整程序路径有同样写法未改;-cc 变体批次未跑(stream-e4-io-cc、stream-prune-io-cc 为 no-producer)。
(3) 20 个 ref 批旧判据失败登记 expected-fail,到期 0.0.82 判据替换(设计已批准)。
(4) 83 个 no-producer 中 30 个(old-product)本版转绿,余 53 个归 0.0.82 与 0.0.83;本版严格证据内 4 个 UNMEASURED。
(5) check-affected.cx 主检查在 58 秒窗内跑不完(结果未知);derive --check 的 iface-hash 等 DRIFT 在 main 上已有。
(6) 整链第 1 到 4 次起链失败(WORKTREE-ROOT-FILES-MUST-BE-REGULAR);INSTALL-APE-RECORD-STALE-COMMIT(install-ape 不刷新 build.json 的 commit 与 sources_sha256)。
(7) c-negative 53/9 同批同数,expected-fail。
(8) F4 检查(-c 与 -c -funit 在 lnx/x86_64 现状固定)本版未做,顺延 0.0.82。
(9) F5 链接合同检查(drivelink 与 objlink 两路一致)本版未做,顺延 0.0.82;W3b、W3c 去留随之待定。
(10) unit-frontcache-equiv0 与 equiv3 的 deps=DIFF 定因本版未做,顺延 0.0.82(本版严格证据中两批 PASS,不代表定因完成)。
(11) 投影读集推广本版未做,顺延 0.0.82。
(12) 全套 s9 本版未跑;产品资格域本版仍非整域,严格证据所列批次之外为 UNMEASURED,不得称全批资格。

## 未解决项(沿用)

(1) NN-VERIFY-WEAK。
(2) -S -funit 返回 NOT_IMPLEMENTED。
(3) 单元内 printf 无输出;const 仍落在非只读节;weak 未做(归 0.0.85);真实 apps 的 -c -funit 对象与 gcc crt 链接时 _start 重复定义。
(4) 其余开放项见 build/pkg-gaps.tsv。

## 证据入口

release/evidence/0.0.81/INDEX.tsv;release/evidence/v0.0.81-strict-2026-11-19.tsv;release/evidence/0.0.81/expected-fail.tsv;release/evidence/0.0.81/expected-unmeasured.tsv;release/evidence/0.0.81/*-deferred.tsv;release/evidence/0.0.81/pack-run.out;release/evidence/0.0.81/cache-manager.tsv;release/evidence/0.0.81/warm-*.tsv。撤回方案:release/rollback.txt(回退 Latest 到 v0.0.80,不删标签与 release)。
