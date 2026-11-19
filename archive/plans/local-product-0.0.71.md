# 本地产品 0.0.71 测量记录(LOCAL 混合候选,非正规链候选)

日期 2026-11-16。输入身份:main HEAD c70986446cf19540c6ab066d180e76d3d41fb66e(git archive 解到 /tmp/lp71/root)。对照:已装 0.0.67(APE sha a99ea71de7ed,回执提交 e02a8f8099)。机器 16 核,测量期间 load1 在 1.8 到 5.1 之间(另有他人作业),数字只作 LOCAL 证据。

## 结论
- 本文前半部分保留最初 LOCAL 混合候选实验；其中“三处正规链拒绝”是当时的历史状态，不再代表当前 main。后续链已走到 pack-recheck。第五条链与计划备忘的接班核验见文末同名节；正式打包仍留到冻结时按发布配方准备私有 src 树，本记录不宣称 QUALIFIED。
- 最初 LOCAL 混合候选由 HEAD 源重编入口(entry-build)、HEAD 表重建私有网包(cx-dev)与 HEAD 三个头文件换包构成，约 2.5 分钟可得。具体输入身份与当时命令见下文。
- 当时混合候选上的收益为：分配密集复现 14.3 秒降到 0.33 秒；8MB sha 979 降到 839 毫秒；编译墙钟同条件降 1.7% 到 7.9%(prune 相 345 降到 144 毫秒)；首次启动 655 降到 574 毫秒。sort、memcpy、缓存命中后的重复启动基本持平。这些旧测量不迁移到最新 HEAD。

## 正规链路与阻塞

正规入口(prd.md 能力速查,tools/bg-candidate.cx 文件头):

    env UNISACC_IDENT_HELPER=$HOME/.cache/unisacc/ident-helper/identity-helper-real5 \
      sh ./unisacc.com tools/bg-candidate.cx -- start --rev HEX40 [--base ABS]
    sh ./unisacc.com tools/bg-candidate.cx -- status [REV]
    sh ./unisacc.com tools/bg-candidate.cx -- promote --check --rev HEX40

bg-candidate 起守护进程,守护进程跑 chain-run(--supervise,段 270 秒,总截止 9000 秒),chain-run 按 build/chain-phases.tsv 逐相调 candidate-chain --resume --until PHASE。计划共 499 相,里程碑顺序:snap、build-objects、build-front、build、wave-objects、wave-runner、ctx、host、list、warmup、jobs-tool、jobs-gen、jobs-tbl、jobs-net、final-net-0 到 41、final、routes、p3-guard-init、p3-guard-0 到 39、p3-opguard、p3-models、p3-resources、p3-assemble、p3、gen1 六目标、g2 与 g3 六目标(lnx 按对象分片)、cache-manager、compare、pack。产物是 OUT 下的 APE 与 pack 回执,校验由各相回执、p3-guard、g2 对 g3 的 compare 与 promote --check 完成。历史耗时(build/phase-times.tsv,real6):整链 3752 秒;最长相 g2/g3 lnx objects 各约 725 到 734 秒;pack 37.8 秒。本次未能实测任何 snap 之后的相。

本次实跑的三处拒绝(均 rc 2,已读发出拒绝的源码行):

(1) bg-candidate 默认 base($TMPDIR/unisacc-bg-b1-UID)被 chain-run 拒:"--out must be strictly under repository out or a tmpdir session root"(tools/chain-run.cx:723,sessionroot sr_ok 只认 unisacc-session-* 前缀)。绕行:--base /tmp/unisacc-session-NAME。属 bg-candidate 与 chain-run 的口径缺口。

(2) 起链环境白名单 build/chain-env.tsv 未登记两个变量,默认拒:UNISACC_FLOW_ROOT(devsvc x 注入,可用 env -u 去掉)与 UNISACC_CHAIN_TOOLS_EPOCH(chain-run.cx:628 自己给子进程设的,调用方去不掉)。后者意味着 main 上凡经 chain-run 的整链在 snap 相必拒。修法:在 build/chain-env.tsv 给 UNISACC_CHAIN_TOOLS_EPOCH 加 allow 行(并决定 UNISACC_FLOW_ROOT 是登记 allow 还是由 devsvc 不注入)。表不在本任务写域,未改。

(3) 绕开 chain-run 直接调 candidate-chain(--rev HEAD --out /tmp/unisacc-session-lp71d/chain --parallel 4 --until build):snap 完成(29.4 秒),post-snap 预检第 6 条 verify-snap 拒:"receipt commit e02a8f8099 != rev and BOOTSTRAP_FROM unset or different"。原因:已装 0.0.67 的回执提交 e02a8f8099 是 main 之外的冻结提交(与 HEAD 的 merge-base 是 855ce83aa9),不是 HEAD 的祖先,candidate-chain.cx:2973 只在祖先关系成立时声明 BOOTSTRAP_FROM。这是结构性阻塞:要么把 v0.0.67 冻结提交合入 main,要么先有一个回执提交在 main 祖先链上的引导产品。伪造祖先关系属绕门,未做。

下一步命令(修掉 (2)(3) 之后):

    env -u UNISACC_FLOW_ROOT UNISACC_IDENT_HELPER=$HOME/.cache/unisacc/ident-helper/identity-helper-real5 \
      sh ./unisacc.com tools/bg-candidate.cx -- start --rev $(git rev-parse HEAD) --base /tmp/unisacc-session-lp71

## LOCAL 混合候选的构造(实际命令与耗时)

均经 devsvc x;工作目录除注明外为共享检出。

| 步 | 命令 | 耗时 | 产物 |
|---|---|---|---|
| 1 | git archive HEAD 解到 /tmp/lp71/root | 0.8 秒 | HEAD 源树 |
| 2 | env CXDEV_HOME=/tmp/lp71/cxdev CXDEV_WT=/tmp/lp71/root CXDEV_PAR=4 sh ./unisacc.com tools/cx-dev.cx -- --build | 16.5 秒 | 装载器、base.bin(已装 lnx/x86_64 切片)、base-tree |
| 3 | 同环境 /tmp/lp71/cxdev/cx-dev --reload | 65.1 秒 | pkg.bin sha a27914d2a10d;nets=13 built=24 local_members=6 stale=0;scope=LOCAL-PARTIAL(BASE_STALE paths=9) |
| 4 | tools/p3-explode.cx 拆 pkg.bin;把 HEAD 的 include/cx-flat.h、stdio.h、stdlib.h 换入 hdr 资源(方法 0 原样存放,F 行与 packlen 手工改);tools/p3-implode.cx 重建 | 约 5 秒 | /tmp/lp71/pkg-hdr.bin sha 6fd4bbf64714,models=40 routes=1823 resources=77 packlen=2199680 |
| 5 | (cwd /tmp/lp71/root)tools/entry-build.cx -- /tmp/lp71/root /tmp/lp71/cxdev/pkg.bin /tmp/lp71/entry2 /tmp/lp71/cxdev/base.bin | 61.1 秒(并行相 27.1 秒,工具自定 jobs=8) | /tmp/lp71/entry2/candidate sha 9b4eec2d3e9a bytes=3836362(lnx/x86_64 ELF 加包尾) |
| 6 | 启动脚本 /tmp/lp71/root/unisacc.com:env UNISA_CONTAINER=/tmp/lp71/pkg-hdr.bin UNISACC_LOCAL=1 /tmp/lp71/entry2/candidate | - | 假根,供四个测速工具用 |

候选身份:入口 9b4eec2d3e9a(HEAD 的 src 与 cx,由已装切片加 HEAD 私有网编出)加包 6fd4bbf64714(HEAD 的 exec 表经 cx-dev 本机配方重建的 lnx/x86_64 网,加 HEAD 三个头)。不是 APE,只有 lnx/x86_64;网不是经正规 PC 与两代 GEN 构造;包资源以方法 0 存头文件(正规包是压缩存放)。

两个事实:第 3 步的包(只换网)编出的分配复现仍是 14.3 秒,头文件是包里的 hdr 资源,cx-dev 不重建它;entry-build 拒绝以薄启动脚本作构造器,须给裸本机切片。

## 端到端数字(已装 0.0.67 对混合候选)

速度只来自四个测速工具(build/speed-baseline.tsv:run-startup-bench、coldcompile、check-bench-gencode、speed-probe),另加分配复现。

| 工具与指标 | 已装 0.0.67 | 混合候选 | 条件 |
|---|---|---|---|
| check-bench-gencode 8MB sha 中位 | 979.097 ms | 839.476 ms(只换网与头、旧入口:816.991) | 3 轮;gcc-O0 106 到 107,gcc-O2 47 到 48 |
| check-bench-gencode 8MB sort 中位 | 1096.416 ms | 1064.970 ms(旧入口:1068.531) | gcc-O0 468 到 495,gcc-O2 309 到 314 |
| check-bench-gencode 8MB memcpy 中位 | 12.077 ms | 12.026 ms | 持平 |
| check-bench-gencode 1MB sha、sort、memcpy | 当日未留存(输出被截) | 104.778、117.112、1.491 ms | 已装 1MB memcpy 1.428 |
| check-bench-gencode 汇总 | valid=6 noisy=0 invalid=0 failed=0 | valid=6 noisy=1(1MB sort gcc-O2)invalid=0 failed=0 | 三档 RESULT 逐字节相同 |
| run-startup-bench first_e2e | 655.193 ms(全新 TMPDIR;默认缓存目录下 674.959) | 574.282 ms | 各 1 次调用,9 项断言 failed=0 |
| run-startup-bench repeat 中位 | 27.251 ms(默认目录 28.613) | 29.768 ms | 候选经 sh 脚本加 env 再 exec,多一层进程 |
| speed-probe ratchets total | 5245 ms | 4833 ms | 两方各自全新 TMPDIR,各 1 次 |
| speed-probe speedprobe total | 6018 ms | 5770 ms | 同上 |
| speed-probe startup total | 8732 ms | 8581 ms | 同上 |
| speed-probe prune 相(三作业) | 344.9、377.7、178.9 ms | 143.7、174.4、184.8 ms | R1 在前两个作业生效 |
| speed-probe lower 相(三作业) | 883.9、1017.0、1652.3 ms | 884.6、1152.7、1694.6 ms | 持平到略慢,单样本 |
| coldcompile cold_ms 三文件 | 5076、6088、8984 | 5221、6145、8590 | 负载不同(已装 1.8 到 2.0,候选 4.2 到 4.7),不可直接比 |
| 分配复现 /tmp/ar/churn.c 默认档墙钟 | 14.331、14.499、14.717 秒(user 8.5 到 8.8,sys 5.5 到 6.2) | 0.341、0.325、0.329 秒 | 候选直接编,不用 nalloc 宏;输出 sum=51242720 相同 |
| 分配复现 -O2 档 | 未测 | 0.314 秒(旧入口加新包) | - |

/tmp/ar2/churn4.c 是把 churn.c 包上 nalloc 宏的壳,候选直接编 churn.c 即同一负载;已装产品编该壳为 0.339 秒,与候选直接编一致。

## 正确性(混合候选上实跑)

| 项 | 结果 |
|---|---|
| tests/runcache.cx(候选经启动脚本) | RUNCACHE assertions=24 failed=0;cold_ms=485 hit_ms=11 |
| tests/runcache-units.cx(候选经启动脚本) | RUNCACHE_UNITS assertions=6 failed=0 |
| tests/runcache-roots.cx(裸候选 entry2/candidate) | RUNCACHE_ROOTS assertions=17 failed=0 |
| tests/runcache-roots.cx(候选经启动脚本) | failed=1(identity-miss):该项靠复制候选文件换身份,复制的是脚本,真实产品没变,属启动方式造成,非产品缺陷;以上一行为准 |
| tests/runcache-roots.cx(已装 0.0.67 对照) | failed=1(b-first-hit):X1 键归一是新行为,已装产品没有,符合预期 |
| check-x86-enc-fuse(HEAD 版夹具,--launch 候选 --expect-fuse) | cases=15531 recs=37068 failed=0;movzbl=3359 movzwl=3359 shift_imm=5382 high_mask_imm=5895 |
| check-x86-enc-fuse(共享检出工作区版,含他人未提交的 F6 族) | cases=18339 执行比对 failed=0,但 shift-cl 形式断言 FAIL;该断言对应尚未入 HEAD 的 X3 B 批,HEAD 候选不带,不计为本候选缺陷 |
| 探针 /tmp/x3s/probe.c、/tmp/x3m/mp.c、/tmp/x3d/dp.c、/tmp/x3r/hi.c | 默认档与 -O2 档共 8 次,输出与 gcc -O0 逐字节相同,退出码均 0 |
| bootstrap-0.0.59 多单元编译(freeze-gate boot 门,HEAD 树,v0.0.67 以来改动的 9 个 src 与 cx 源) | GATE boot PASS units=10 ok=10 bad=0 both-reject=0 |

## 未验项

- 正规链候选不存在:PC、两代 GEN、g2 对 g3 比对、p3-guard、pack、APE 六目标切片全部未跑;本记录数字不能当候选证据。
- 混合候选的网来自 cx-dev 本机配方,只含 lnx/x86_64;osx、win、arm64 未构造未测。
- 入口由"已装切片加新网"一代编出,未做自举第二代,未与正规 g2 入口比字节。
- 头文件以未压缩资源换入,运行期装载路径与正规压缩资源不同;只验证了分配复现与探针输出。
- 测速样本少:启动与 speed-probe 各方 1 次调用,gencode 3 轮;未做 ABAB 交错;coldcompile 两方负载不同,不下结论。
- 重复启动候选慢约 2.5 毫秒,疑为启动脚本多一层进程,未定因;AF_ALG 命中路的收益因此未能单独量出。
- sort -O2 档回退(308f2216e0 提交说明已记)未在混合候选上复测。
- 慢车道、affected --run、ratchets 未跑。

## 留在 /tmp 的东西

/tmp/lp71(混合候选、假根、测量输出)、/tmp/unisacc-session-lp71 与 /tmp/unisacc-session-lp71d(被拒的链输出)、/tmp/unisacc-bg-b1-1001(第一次被拒的候选目录)。snap 相在 ~/.cache/unisacc/chains/snap/c70986446c-045c5a38 留了一份 HEAD 快照(链工具自己的会话根,非发布根)。未安装、未发布、未改已装产品。

## 第五条链与计划备忘接班核验

输入为 main 提交 4c2e7ad1676b3c744986258adda5378738a27668。这是性能实验，不是发布资格：沿用 cc 的决定，只跑到 pack-recheck；缺正式配方的私有 src 树时 pack 的 rc 2 是已知终点，不据此修打包或放宽门。

(1) 真实终态已核。会话 /tmp/unisacc-session-lp71/4c2e7ad1676b3c744986258adda5378738a27668/exit.tsv 为 rc=2，chain-run-216.stderr 是既有 cache-manager 前置拒绝；pack-recheck 为 rc=0。接班时没有该链进程，不重起同输入。g2 与 g3 的两个 Linux 目标共四份 fixedpoint.planmemo-gN-I.stat 均为 reused，证明各自末次调用命中。该文件每次覆盖，不能还原全程命中率，也不能据它证明所有调用的 TMPDIR 都稳定。

(2) 同口径重算。使用 tools/phase-times-calc.cx，对第四条 ab212d647d 与第五条 4c2e7ad167 各自的 chain-run.tsv、plan.tsv 重算：closure 域的尝试墙钟合计分别为 748.3 与 668.9 秒，均 16 次，差 79.4 秒；所有尝试合计 4047.1 与 3903.2 秒。这些是尝试耗时之和，不是整链墙钟或单因素收益。两条均没有完整成功终态，该工具明确输出 wall_s=UNMEASURED。第五条仍有整目标相超时，不能宣称已消除该瓶颈。

(3) 独立开关对照。使用第五条真实冻结树 ~/.cache/unisacc/chains/snap/4c2e7ad167-045c5a38、compiler.p4.pkg、gen1/lnx-x86_64/candidate、钉住的 entry-build.pin.d/tools/entry-build.cx，以及原 tmp-g2-lnx-x86_64 的 TMPDIR。仅运行 --closure-list，不编译对象、不修改原链会话；备忘复制到 /tmp/codex-lp71-memo-nQt2Rs/plan.memo，输出也写该目录。A 将 UNISACC_EB_PLAN_MEMO 设空，B 指向副本。两者均经已装产品运行同一个钉住工具，故耗时包含相同的工具启动与输入核验。

tools/abab-bench.cx --n 3 的 A/B 交错结果：A 为 3714、3852、3715 毫秒，B 为 3162、3178、3269 毫秒；中位数 3715 与 3178 毫秒，差 537 毫秒，下降 14.45%；CV 为 1.72% 与 1.47%，load1 为 0.42 至 0.55，noisy=0。六次调用均 rc=0。之前的预跑和交错结束后均用 cmp 核 off.tsv 与 on.tsv，逐字节一致；备忘状态为 reused。原始 ABAB 输出在 /tmp/devsvc-flow 执行记录对应的 devsvc run 回执；本次会话的输出和备忘副本在 /tmp/codex-lp71-memo-nQt2Rs。临时目录清除后，原始材料不再可用。

(4) 判断边界。已证明真实链末次命中，以及一组真实冻结输入上复用计划能降低列闭包调用的耗时、输出不变。未证明全链命中率，未给对象编译或正式资格新增通过结论。约 3.5 秒前导还包含封签等工作，不能全部记成 44 根规划可省成本；B 仍约 3.18 秒，剩余各项比例未独立测量。下一步优先分账，而不是直接删去首尾输入核验。

(5) 中断小单保全。cc 额度耗尽时，对象入口桩、tape 绕过 elf 网、整目标限流三单均未完成验收，不合入半成品。已找到原材料 /tmp/ord-objstub-S9avpO、/tmp/ord-tapebypass-d7v7SR、/tmp/ord-wholeo-4VNQlE；其中 wholeo 树已有前端子步耗时表的未完成修改，不能视作已完成限流。共享检出的 tests 路径有大量 100644 -> 100755 变化而内容无差异，来源未核，接班未改动或提交这些路径。
