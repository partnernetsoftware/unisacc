# 0.0.38 一次裁红请求（full038c 出口表，2026-10-10）

依据：full038c 真实出口 649/649、final rc=1（538 PASS）。混合 segment 完整调度基线，非绿链。出口表由 `release/tools/exittable.py` 按 `release/preauth.tsv` / `release/rulings.tsv` 归类；原始证据包在仓外 `~/.unisacc/evidence/full038c-final`（含 SHA256SUMS、needs-ruling.txt）。不重跑、不进 Draft、不开第四轮；所有原失败保留，不改验收措辞、不抬限时、不删测。

## 已有处置（无需再裁，列明以免误读）

| 类 | 数 | 处置 |
|---|---|---|
| PASS | 538 | — |
| HOST_TIMEOUT（H1 清单内、solo 重试后仍 142） | 15 | 按预授权表记 H1 云机基线：closure-c1..4、stages-1..3、exec-e5、exec-f1-attributes、exec-macself(+package)、exec-tableself-2、lib-callable-variadic-model、shared-e2-plain、tools-2 |
| RESOURCE_UNKNOWN（内存证据缺测，拒启 rc2） | 6 | 义务未执行，按准入协议记缺证：com-seedgen、seedgen-2/3/4、seedparse2-1/2 |
| UNVERIFIED_HOST（宿主不适用 rc77） | 6 | 覆盖义务保留：exec-bootstrap-osxarm/osxx86、lib-stack-arm/x86/x86-hostabi、lib-windows-gp-native |
| BLOCKED（前驱失败、未执行） | 27 | 挂三个根：rowcov-parse2-build（25：24 分片 + union）、com-luatests-build（1）、rowcov-enc 系（union 1）；随根处置，不单列 |

## 请裁：NEEDS_RULING 57 项（按根因分组）

| 组 | 数 | 套件 | 事实 | 建议处置（供裁） |
|---|---|---|---|---|
| A 宿主跳项（skip 即失败） | 7 | ccinterop（skip 6）、hosthdr（skip 12）、elfobj / com-elfobj（skip 2）、forward / com-forward（skip 1）、syscall6（skip 1） | 判定部分 wrong 0；跳过的是本机缺宿主条件的子项 | 记 H2 宿主基线；0.0.38 H2 切口改为套件声明宿主需求、不满足子项 rc77（P7 已批做法），不放过 skip |
| B 自举/跨目标 self 超时 | 8 | exec-armself、exec-e4self、exec-macxself(+package)、exec-winself(+package)、exec-winx86self(+package) | 均 solo 重试后 142；同族 exec-macself 已在 H1 | 追加入 H1 云机基线 |
| C rowcov / lib-*-source 超时 | 20 | rowcov-enc-1..8、rowcov-pp、rowcov-pp-locations、rowcov-pp-shared、rowcov-parse2-build（BLOCKED 25 的根）、lib-bitfield/carrier-factory/fp-rank/fp-value-rank/layout/sig3-source、lib-source-provenance(+located) | 单 rowcov 进程树实测 1.97 GB；本机从未在 46 s 内完成 | 记 H1；另开切口审 rowcov 超时是否丢弃已完成 probe（只在开发预验复用，正式不拼接） |
| D 其他超时 | 10 | com-fb12-multi、com-luatests-build（BLOCKED 1 的根）、com-tapebin-1/2/3、exec-arm、exec-asm、exec-driver-core-contracts、seed-matrix-features-1、shared-e2-located | 均 142 | 记 H1 云机基线 |
| E 已裁套件的新失败形态 | 2 | lib-lifecycle（运行时分配断言，异于已裁 40 s 超时）、exec-driver-language-2（宿主 cc 编 enum_forward.c 失败，异于已裁外杀） | 首错签名不匹配 rulings.tsv，故未消费旧裁 | 请逐项裁：是否追加 H2 并允许 H2 切口修一轮 |
| F 门禁自身 | 1 | gate-layers（unclassified gate suite: hostcheck） | 本轮新增 6 个套件未登记层；主树 48b8b9ae 已修，运行树旧码保留红 | 记已修，主树复核 rc0 |
| G 产品/驱动功能红 | 4 | com-forward-multi（ok 1 wrong 2）、com-multi（multi-unit FAILING）、com-difftest_o-1（refuse 3）、realprog（lua） | 产品 .com 路线的真实功能不符/拒收 | 需逐项定位：真实产品缺陷还是宿主差异；不得记入宿主基线了事 |
| H 其他 rc1 | 5 | exec-bridge-linux（clang -arch arm64）、windows-resolver-host（需 zig/真 SDK）、seed-construct-parse2/-2/-3（inspect-parse2 图断言） | 前两项是宿主工具链缺项；后三项为构造器断言 | 前两项记 H2 宿主基线；seed-construct-parse2 三项需定位 |

## 同时提请

1. 授权一次 verification 全量（当前 main，含 attempt 类别、per-suite history、实时内存准入、stage 对安装修片，约 2.5 h，不进 Draft），用于与本基线对比测得提效；未授权前不跑。
2. 浪费榜首项（tail DEFER 4816.9 作业秒）的改进已在主树，需上项全量方能测得实效。
