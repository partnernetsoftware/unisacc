# unisacc 0.0.80 发布说明

状态:同候选原生 CI run 44452543971(六目标 success,11:35 派发,11:40 通过,一次通过无重派)。整体裁决 NOT_QUALIFIED:全套 s9 本版未跑,产品资格域未整域实跑,严格证据只覆盖下列批次,不得称全批资格。

## 真实身份

| 对象 | 身份 |
|---|---|
| 候选点 T(整链输入) | 5e9333ca54609553bae1b70c851d4c33952856ff(整链 499/499 rc 0,10:21 到 10:46,一次通过无重启) |
| 冻结提交(发布标签目标) | a08294deab06a77874bbe73cc94412696459d9c8(相对 T 产品输入零改动:src cx ext include exec weights seed kernel 无差) |
| 发布 APE SHA256(unisacc.com) | 110ab298bf25065a1ecf7ff36c82609c42d7a4eeaf02bcc8228e0e2271bc8f78(7388948 字节,比 0.0.79 多 1759 字节;--version 打印 unisacc 0.0.80;打包后与链产物同字节,PACK_APE SAME) |
| 源快照 SID | 31fe92e47d8258d313356ec434e85625943643593489ebb4486bf5f579124343(VERIFIED,T 的链快照) |
| pkg.bin | 199407cd6b8dcbcb9b45f99c3fdb3f03622de816ee4edcf7c287bdad2c8843c9 |
| CI run | 44452543971 |

## 产品改动

相对 0.0.79 公开冻结提交 f649bc0507,产品输入三处。整程序 -o 路零变化:17 份 -o 产物与 0.0.79 逐字节相同;-funit 对象产出(lnx/x86_64 的 -c 与 -c -funit)变字节是预期。arm64、osx、win 的新对象行为未做或未实测(见已知缺口)。

(1) F1b -funit 对象带初值全局登记 .init_array:每个对象的 __init_u 改 LOCAL,发射 .init_array(对齐 8)与 .rela.init_array(R_X86_64_64;arm64 为 R_AARCH64_ABS64=257)。改 exec/enc 的 elfobject 与 objectplan 表,只改已分叉节 d0.s17/d1.s18/d0.s23/d1.s24,不碰共用 d0 节。cx/driver/objlink.cx 经 .init_array 重定位找 LOCAL __init_u 并允许该节(旧对象行为不变,条目多于一个 rc 2 具名拒绝);cx.driver 包 0.1.4 -> 0.1.5。新检查 tools/check-objabi.cx:p03 与多对象同链原 failed=2,F1b 后 failed=0。
(2) F2 -funit 对外(非 static)带参函数 SysV 入口:对外名指桩(rdi..r9 搬到内部约定 rax..r8 后 jump 本体 __ub_NAME),单元内 call 与取址指向本体;单元调用 extern 目标时动态 16 字节对齐并反向搬运(imm rax,0 清 al)。只改 exec/lower 的 librarymodule 表,门控 object 且 of_unit=1,整程序网不含。check-objabi 11 例 failed=0 expect_fail=4(p07 weak 与三个 KNOWN-LIMIT,见已知缺口)。
(3) F6 环境变量语义事实表 exec/facts/env-semantics.tsv(14 个变量 27 个读取点),modelcache 改读表驱动,行为不变(67/67 矩阵新旧产品相同);cx.runtime 包 0.1.11 -> 0.1.12。新检查 tools/check-env-semantics.cx。
(4) 独立复核:F1b 可提交带条件(条件已落实);F2 第一轮复核抓出 push r9 栈对齐回归(movaps 段错误),已修为动态对齐,第二轮未再独立复核,只有自跑 check-objabi 与回归;F6 为行为不变的表驱动小改动,未单独复核。

## 开发工具链(不进产品)

(1) register --tool --sync 写入路径首次实战(含 coverage 与 manifest 事务、孤儿清理),新增分片 tools-compile-159。
(2) 选批漏选 release-prep-ratchets 修复。
(3) table-lint 值域修订;新工具 handwritten-census(基线 143 文件 27484 行)与 check-table-lint,由 agyneon 试用题产出,本仓复核实跑后登记批次。
(4) fable 外部评估与路线图修订版 2。

## 严格证据

(1) 冻结前预跑(origin=prefreeze-strict,T2=af24533a88 布局,候选 APE 110ab298,dvqualify --jobs 3,一次 --out 加两次 --resume):affected --files f649bc0507..T2 的 66 个路径选 653 批,与 product-qualification 609 批取交集 223 批;末态 PASS 219、FAIL 1(预期)、UNKNOWN 3、INVALID 0。证据 release/evidence/0.0.80/strict-prefreeze.tsv。
  - FAIL c-negative count=53/9:与 0.0.76、0.0.79 同批同数,登记 expected-fail.tsv。
  - UNKNOWN cx-booter-ape、tools-default-regression、cx-model-decode:affected.cx 报 no-producer(booter-raw-elf、old-checker、p3model-material),材料无生产者,记 UNMEASURED,不记 PASS。cx-model-decode 本版新进集合(F6 改 cx/runtime/modelcache.cx),依赖已归档的 P3 链材料。
  - 首段冷 kit 超窗只写 CHECKPOINT;ape-need-refusal、c99-whitespace、c-negative 首轮 outer-timeout-60s,notcovered 首轮 transport=-1 UNKNOWN,行都保留,--resume 后得终态。
  - 产品等价抽查:calc、dijkstra、mandel 用候选与 0.0.79 公开件 --target lnx/x86_64 -o 逐字节相同。
(2) 冻结后补差免跑:T2 -> F 的差只有 release/evidence/0.0.80/ 七个白名单路径,产品输入与批次定义零差,预跑整份复用。整合严格文件 release/evidence/v0.0.80-strict-2026-11-19.tsv 为预跑全部段原样拼入加复用说明头,无补差段,无任何删减。
(3) 预热与干跑:N4 读回预热 WARM READY(5 个目标)。release-prep verdict=NOT_QUALIFIED(rc 1;c-negative 被 prep 列 NEW_FAIL,因 0.0.79 expected-fail 的比对口径,本版按同批同数登记),棘轮冻结提交 git archive 15/0。release-exec --dry 结果见 INDEX 的 dry 行。

## 整链

499/499 rc 0,10:21 到 10:46,一次通过无重启。链目录 /tmp/unisacc-session-v080-chain/5e9333ca54609553bae1b70c851d4c33952856ff。promote --check 通过。

## 已知缺口

(1) c-negative 53/9:产品接受 9 个应拒的 C99 负例,0.0.76 起同批同数,列入 expected-fail,不是本候选回归。
(2) cx-booter-ape、tools-default-regression、cx-model-decode 材料无生产者,UNMEASURED。
(3) -funit 对外函数 7 个及以上整型参数、浮点参数、单元内取址外传(qsort 回调)静默算错(FUNIT-SYSV-ARGS-OVER-6-FLOAT-FNPTR-SILENT,归 0.0.82)。
(4) 单元内 printf 无输出(改前就有)。
(5) arm64 的 SysV 桩未做;.init_array 在 arm64、osx、win 对象上未实测,UNMEASURED。公开后补测(2026-11-19,qemu-aarch64-static 与 aarch64-linux-gnu-gcc 实跑,材料为本版公开件):lnx/arm64 的 -c -funit 对象带 .init_array 与 R_AARCH64_ABS64,但 arm64 的 __init_u 没有 AAPCS64 入口包装(代码 ldr x17,[x7]; add x7,x7,#8; ret x17,按内部约定从 x7 栈取返回地址),被 libc 经 .init_array 以 AAPCS64 调入时 x7 无定义而崩溃:任何链接了这类对象的 arm64 程序在启动时段错误(rc 139),与链接单个或多个 -funit 对象无关;改前(0.0.79)arm64 -funit 对象无 .init_array,只有被 C 调用其中函数时才崩。结论:lnx/arm64 的 -funit 对象产出在本版不可用(改前同样不可被 C 调用,本版把崩溃提前到启动),缺口 OBJABI-ARM64-INIT-U-NO-AAPCS-ENTRY,与 F2 arm64 桩同类,0.0.81 与 cx-dev arm64 配方重建一并处理。
(6) -funit 对象 __init_u 符号类型为 NOTYPE。
(7) 真实 apps 的 -c -funit 对象与 gcc crt 链接时 _start 重复定义(改前就有)。
(8) const 仍落在非只读节。
(9) weak 未做(F3 需三源改动,归 0.0.85)。
(10) F2 第二轮未独立复核;F6 未独立复核。
(11) net-health 基线:对象网 x86_64 与 arm64 各 warnings 与 maybe_unwritten +1(跨网输入 eo_initsym,NET-HEALTH-NO-CROSSNET-INPUT-DECL);object-lower-x86_64 各 +3(ccwm ts te,table-lint 不跟踪子程序返回的误报)。
(12) 工具线:DEVSVC-X 外层超时孤儿;CI stream-pipe 分段余量。
(13) 0.0.78 的 safe 静默漏判与误拒未变;旧产品不认识 safe/owns/unsafe pragma,仓内 .cx 采用前不得再用旧 Root 引导。

## 未解决项(沿用)

(1) s9 全套本版未跑;产品资格域未整域实跑;严格证据所列批次之外为 UNMEASURED。
(2) NN-VERIFY-WEAK。
(3) unit-frontcache-equiv0 与 equiv3 deps=DIFF。
(4) -S -funit 返回 NOT_IMPLEMENTED。
(5) 其余开放项见 build/pkg-gaps.tsv。

## 证据入口

release/evidence/0.0.80/INDEX.tsv;release/evidence/0.0.80/strict-prefreeze.tsv;release/evidence/v0.0.80-strict-2026-11-19.tsv;release/evidence/0.0.80/expected-fail.tsv;release/evidence/0.0.80/expected-unmeasured.tsv;release/evidence/0.0.80/ratchets.txt;release/evidence/0.0.80/pack-run.out;release/evidence/0.0.80/cache-manager.tsv;release/evidence/0.0.80/warm-*.tsv。撤回方案:release/rollback.txt(回退 Latest 到 v0.0.79,不删标签与 release)。
