# unisacc 0.0.86 发布说明

状态:草稿,未公开。本说明在整链第五次起链运行中起草(整链输入 rev 71b575b2f7),凡标 TBD-CI-RUN、TBD-FREEZE、TBD-PACK、TBD-STRICT、TBD-PUBLIC 的位置在发布流程实际产生证据后回填;未回填前不得据本说明宣称任何 PASS、哈希或资格。整体裁决预定为 NOT_QUALIFIED:全套 s9 本版未跑,产品资格域非整域,严格证据所列批次之外为 UNMEASURED,不得称全批资格。

## 真实身份

| 对象 | 身份 |
|---|---|
| 候选点 T(整链输入) | 71b575b2f7(整链第五次起链;完整 40 位与链目录 TBD-PACK) |
| 冻结提交(发布标签目标) | TBD-FREEZE(F;须核相对 T 产品输入 src cx ext seed weights kernel include exec 零差异) |
| 发布 APE SHA256(unisacc.com) | TBD-PACK(字节数与相对 0.0.85 的 8425614 字节之差同时回填) |
| 源快照 SID | TBD-PACK |
| pkg.bin | TBD-PACK |
| slice-win-arm64 | TBD-PACK |
| SHA256SUMS | TBD-PACK |
| cache-manager | TBD-PACK(0.0.85 为 x64 d34bee6c、arm64 0aae3fbc,本版是否沿用待核) |
| CI run | TBD-CI-RUN(release-exec Y1=TBD-PUBLIC,Y2=TBD-PUBLIC) |

## 产品改动

产品输入相对 v0.0.85 标签到 origin/v0.0.86(71b575b2f7)共 60 个文件 +948/-133(git diff --stat,范围 src cx ext seed weights kernel include exec)。主题为改一行再跑的 run 路单元级分别编译(默认关)加执行器度量与前端诊断收尾。

(1) W2d 加 F5 单元对象守卫:cx-flat.h 生成源对 environ 与 _unisa_sig* 加 RT_EXTERN 守卫,closure-obj 补 signal 与 unistd 头(3a53ed489b);新增 tools/check-unitlink.cx,登记 check-unitlink 与 check-unitlink-more 两批,零 objcopy 等价对拍与先红控(fb54176637)。
(2) W2 run 路按模块分别编译(34b29ad931、1ca7a154f7、f0c1b244c7、fd1c6a28dd):新增 cx/driver/runsplit.cx,src/drive.cx 极小钩子接线;开关 UNISACC_RUN_SPLIT 默认关,1 启用,shadow 只验链接后交整程序路;前置门、单元编译失败、四类跨单元 ABI 预检(多于 6 参、变参、函数指针、浮点)或 ol_link 拒绝一律无声回落整程序路,stderr 不多字节;只影响 -run 路,不动 -o 字节线。默认关的依据见已知缺口 (1)。
(3) M1 RR_TIME 分步纳秒:推理行同次输出每相步数与每步纳秒,新增 check-rr-steps 批次,cx.sys cx.base cx.runtime 升版(c336dfcd0e)。
(4) D1 E1 定位错误与重复形参:E1 词法负例(非法八进制数字、未闭合字符串)经可定位错误记号交 E3 报文件、行、列(800f32cff8);local-redefinition 误吞重复形参诊断修复(d7b4631c90)。
(5) D2 c-negative 收至 62/0:C99 6.5.16p2 拒绝经 const 指针写入(c65f1ba168),c99-warn 隐式声明三类默认精确拒绝且无对象(20dadd1f15),聚合成员与局部逗号声明保留限定(89020874b5),驱动重复声明清理(00ed3f4df7);0.0.85 登记的七例(assign-to-const、write-through-const-pointer、discards-const、distinct-pointer-compare、conflicting-function-types、implicit-function、local-redefinition)本版开发验证 c-negative 62/0。
(6) L1 ext/sysx/linuxns.cx 新增 sy_seccomp_allowlist 与 SY_SECCOMP_FLAG_TSYNC(0d6fc5aa95、6785b0dc95,ext.sysx 升 0.1.9):默认拒绝 BPF,固定 20 条前导守卫先于白名单扫描(AUDIT_ARCH 不符杀进程、x32 位 EPERM、clone3 ENOSYS、clone 带命名空间位 EPERM、ioctl TIOCSTI 与 TIOCLINUX EPERM);参数校验与上限以代码为准,见已知缺口 (2);新增 sy_seccomp_kret 把内核原始返回映射为 0 或 -errno;check-ext-sysx-linuxns 78/0。
(7) parenfold 绑定同步与 generator-location 前移:记号位置在普通定位模式记录,括号折叠从 DL.read 克隆并进入 DL.prefix,保留生成器目标校验(83f790a680);现构宿主 seed-gen 后执行 parse2 --locations,夹具在修复前捕获 clone-push 目标错配,原有 16 项拒绝断言保留(71b575b2f7)。

## 开发工具链(不进产品)

(1) T4 lane-inputs:tools/lane-inputs-gen.cx 以 seed/ident.c list-current 为事实源生成 build/lane-inputs.tsv(带 --check),候选触发键、开发车道与冻结闸改读它,五探针批次 lane-key-probes;冻结闸 @lane:A 改保守拒(a2c02994ee、02eae53054、a90265966f)。
(2) M2 执行器内层循环判定门实测 GO(-20.2% 到 -25.6%,c51603d580),本版只做实验,未产品化进内核。
(3) install-ape 候选模式与回退加固(f5cba6d016、a91bcb5d53):--candidate 装前备份当前 Root 到 .unisacc-root-backups,--rollback 按 manifest 核 sha 与字节后恢复。
(4) 多个工具按 C99 补静态前置原型与 const 修正,行为不变。

## 严格证据

TBD-STRICT:预跑批数、PASS、FAIL、UNKNOWN、SUPERSEDED、INVALID 计数,严格证据文件路径与 sha256,打包(release-run --pack)、预热(warm)、干跑(release-exec --dry)结果,均待发布流程实际产生后按原样回填;本节当前无任何已实跑结论。

## 已知缺口

(1) W2 默认关:0.0.86 维持 UNISACC_RUN_SPLIT 默认关,需显式 UNISACC_RUN_SPLIT=1 启用,任何拒绝回落整程序路。判定依据:190 脚本对拍真实不一致 0,但仅 16/190(8.4%)命中分别编译,其余因 -funit 外调桥不支持多于 6 参、变参、跨单元函数指针、浮点或单文件而回落;命中脚本隔离缓存冷跑慢 1.1 到 3.6 倍;主文件改一行墙钟 1698 ms -> 1187 ms(-30.1%),未达原 600 ms 目标(已移至储备 W3b)。默认开推迟到 0.0.87 之后补外调桥。
(2) sy_seccomp_allowlist 上限:代码参数校验接受 allow_cnt 1..310(越界 -EINVAL),但 BPF 缓冲为 640 条,310 项需 641 条,返回 -ENOSPC;因此实际可安装上限为 309 项,测试对抗组即 309 项。系统调用号须在 0..4095,未知 flags -EINVAL,非 Linux 上参数校验先于 -ENOSYS。clone 无命名空间位与非守卫 ioctl 不论是否列入白名单均放行(agy-cage 合同);x32 守卫只是纵深防御;高危号由调用方生成侧排除,入口不兜。真实多线程 TSYNC 失败路径未实跑。
(3) c-negative 到期项核销:0.0.85 expected-fail.tsv 登记的 55/7 七例 expires=0.0.86,本版开发验证 62/0(83f790a680 记录);发布严格证据中 c-negative 的实测结果为 TBD-STRICT,若为 62/0 则本版 expected-fail 清零为仅表头,否则按实测逐例如实写,不续期冒充。
(4) 全套 s9 本版未跑,整体裁决 NOT_QUALIFIED;外目标执行格仍 UNVERIFIED;CI 资格(TBD-CI-RUN)只覆盖六目标冒烟子集。
(5) 产品资格域非 full:严格证据所列批次之外为 UNMEASURED,不得称全批资格;cx-booter-ape(booter-raw-elf)与 tools-default-regression(old-checker)为 no-producer UNKNOWN,记 UNMEASURED 不记 PASS。
(6) 0.0.85 公开时 GitHub Release v0.0.85 正文为 0 字节(说明未随发布上传)。本版执行 gh release create 或 edit 时须以本文件为正文上传(--notes-file release/notes-0.0.86.md),并用 gh release view v0.0.86 --json body 读回确认长度非 0;读回结果 TBD-PUBLIC。
(7) release/decision.txt 仍为 0.0.74 旧裁决;release-exec 第 7 步用陈旧 evidence.tsv 与 decision.txt 比对恒等(S6a),公开前须改为从当次证据重算并写出;修复状态 TBD-PUBLIC。
(8) __builtin_expect 既有限制、__builtin_unreachable 与 __builtin_trap 未纳入、inttypes.h 诊断位置前缀问题沿用 0.0.85 说明,本版未修。
