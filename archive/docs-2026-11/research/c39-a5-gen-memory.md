# A5 gen 内存补证与 A4 矩阵身份核对

审阅/测量人：cdx-unisacc；日期：2026-10-10。基线：95ddf2de3584cc9a038dde58fe9400d4d7134f95；独立 worktree，与共享 examples 脏删除隔离。仅记录事实，不改产品、seedmemory 协议/常量、冻结、签名或裁定。

## A5 失配定位

按 tests/seedgencheck.sh 与 seedmemory.reference_key 的实际 shell glob 顺序串联：exec/assemble.py、exec/finite_rules.py、exec/build/*.py、exec/*/*.tsv、exec/facts/*.tsv、weights/gold/*.tsv。共1142项、929个唯一文件（facts由两组glob重复进入；未去重改协议）。旧快照8a433c5d父提交完整SHA1为 e8d10ecb97107e7be798f15ef32a88278dd6217c；当前完整SHA1为 6ec1fe9d67dd07334650b6df2af955f1fe1e8e9d，截前16位就是 e8d10ecb97107e7b→6ec1fe9d67dd0733。

路径和顺序未变，唯一内容差分为 exec/enc/x86-procs-result.tsv：SHA256 5adabfa585d31f426a7bb1444de07be8ce6d1c6fbaee74a7505b8b3f3a4da3b6→81ec4c504a055354b16e5e6f62113b8eeeb0433555edee0aa1280605a3f7436f（main 8a433c5d）。两个txt夹具不入该键。不是仅因gen.c相同就可沿用旧阈值；原gen键仍STALE，未在本笔盲改key。

## 现路线补测

宿主Linux/x86_64；/usr/bin/cc实体为/usr/bin/x86_64-linux-gnu-gcc-14，cc (Debian 14.2.0-19) 14.2.0，实体SHA256 a23ecab8ff08f09ad8c80602c2c5df7f49e09c25905cb8975902e101bf72635f。受约束编译环境变量均未设置；不经过launcher。按当前seedgen脚本编译：`/usr/bin/cc -std=c99 -O2 -Iseed seed/gen.c -o <仓外gen>`，rc0，编译最大RSS167332KiB、子cgroup峰181927936B、墙钟5.80s，oom_kill0（仅本次命令实测）。构建后二进制SHA256 f997b25d8f8391e12034871bd14e20523fa6cc88720cadb3bc6a71a41c591e64，与原证据相同，已实际核对，不以flags文字相近推断。

每条调用从当前脚本spec()提取stage/flags，用新输出路径。包裹链：外层bound55→checkrun LOG --→既有memscope MAX NAME→time RSS/墙钟→bound50→gen STAGE OUT FLAGS。每次独立子cgroup，MAX=min(3GiB,实时可用内存-1GiB)，swap.max=0；本轮读回max均3221225472。begin、available、cap、实际命令、子cgroup peak/oom_kill与原checkrun rc均在仓外日志。首项前打印/proc/self/cgroup归属。未迁移其他进程、未调大重试，30条全rc0、oom_kill0。

峰值表的RSS是time最大进程RSS；cgroup peak为子组记账峰值，二者不混称。每个生成器串行测量；套件配置内部并发仍4，暖C需求应按既有协议汇总对应串行RSS floor+1MiB，再加max(25%,512MiB)余量。没有宣称本窗实跑了四路并发suite或测得四路cgroup峰。Python冷构造、COM与原未测flags继续UNKNOWN；编译资源与生成器运行资源分列，不借二进制一致声称编译资源等价。

| 路线 | stage / flags | rc | RSS KiB | 子cgroup peak B | 墙钟 s |
|---|---|---:|---:|---:|---:|
| e2 | pp --shared-predefines | 0 | 188808 | 215363584 | 0.53 |
| e1 | lex --typed | 0 | 338672 | 353517568 | 0.68 |
| e3 | parse2  | 0 | 1285148 | 1383936000 | 3.94 |
| e4 | opt --o2 | 0 | 41232 | 51134464 | 0.18 |
| o1 | opt  | 0 | 15032 | 20430848 | 0.05 |
| prune | prune  | 0 | 33752 | 43667456 | 0.14 |
| nativeabi | nativeabi  | 0 | 305664 | 328712192 | 0.64 |
| enc-elf | enc --elf | 0 | 145532 | 163782656 | 0.48 |
| enc-macho | enc --macho | 0 | 213788 | 235335680 | 0.64 |
| enc-pe | enc --pe | 0 | 270156 | 294547456 | 0.83 |
| arm-elf | enc/arm --elf | 0 | 145720 | 165187584 | 0.54 |
| arm-macho | enc/arm --macho | 0 | 213524 | 236371968 | 0.69 |
| arm-pe | enc/arm --pe | 0 | 270396 | 295673856 | 0.94 |
| lower-lnx-x | lower --full | 0 | 196728 | 224485376 | 0.68 |
| lower-lnx-a | lower --full --arm64 | 0 | 220880 | 251375616 | 0.73 |
| lower-osx-x | lower --full --osx | 0 | 189684 | 216645632 | 0.63 |
| lower-osx-a | lower --full --osx --arm64 | 0 | 215944 | 246104064 | 0.68 |
| lower-win-x | lower --full --win | 0 | 133908 | 153018368 | 0.43 |
| lower-win-a | lower --full --win --arm64 | 0 | 160936 | 183513088 | 0.53 |
| obj-lower-x | lower --full --object | 0 | 200552 | 229068800 | 0.69 |
| obj-lower-a | lower --full --object --arm64 | 0 | 224984 | 256184320 | 0.78 |
| obj-enc-x | enc --object | 0 | 160376 | 181276672 | 0.53 |
| obj-enc-a | enc/arm --object | 0 | 163588 | 186388480 | 0.59 |
| tokenpp | pp --shared-predefines --no-autoinc | 0 | 39456 | 47685632 | 0.13 |
| tokenlex | lex  | 0 | 122568 | 131469312 | 0.44 |
| warnlex | lex --locations | 0 | 759712 | 786477056 | 1.34 |
| warnparse | parse2 --warnings --errors | 0 | 2186612 | 2314887168 | 5.40 |
| warnunits | parse2/units --locations | 0 | 201604 | 218640384 | 0.58 |
| errorparse | parse2 --errors | 0 | 2171756 | 2298208256 | 5.10 |
| warnpp | pp --locations --shared-predefines | 0 | 185036 | 211333120 | 0.53 |

30个新输出逐一按stage/flags映射到既有COV1 §24矩阵，摘要均与原C O2/Python/ASan一致，未拿陈旧输出作本次rc0证据。本次只测C生产RSS，不新增ASan或Python内存声称。完整有序输入manifest SHA256：80473728092df6373f983d52bb03f023b5a0424af7eb1706501cd151d5e250af.

## A4 当前完整输入是否可复用

当前有序gen键输入逐项与原COV1矩阵实际worktree的路径/内容一致；seed/gen.c、seed/facts.h、exec/c/buildcompiler.sh也字节一致。git diff 8a433c5d..95ddf2de -- seed exec unisa weights为空，故该区间没有遗漏的同树构造器输入变化；gen.c直接头json.h亦在未变seed树内。调用位置/flags清单仍原30项，未删object或目标变体。

所以可复用的是**COV1修片后的**§24矩阵（原根表1–19全三路；20生产/参考原表加attempt2 ASan；21–30在attempt2），不是0.0.38修片前矩阵。旧矩阵的基线commit早于收录修片，不据此判陈旧；实际修改后的输入表SHA和本次30输出已核相符。原ASan detect_leaks=0、warnparse首3GiB OOM保留与已裁诊断独立上限续跑的限定保留；不称LSan绿。工具/源码/flags或任一构造输入再变，则该复用结论必须重核。

交接状态：A5当前30条暖C生产补证已具备，但seedmemory.py仍绑定旧gen参照键，实际准入仍会UNKNOWN rc2；落常量需owner按这份数据另行授权/提交，不把文档当准入已恢复。A4本次核对支持复用上述修片后矩阵；是否认定冻结前置就绪交cc/机房主任，不改账本。

仓外证据索引：cdx39-a5-evidence目录的inputs.tsv、input-audit.json、results.tsv、run.py、run.log，各路线.log和-inner.log（checkrun原rc、time RSS、memscope peak/max/oom），以及现存cdx39-cov1-evidence矩阵索引。过程JSON仅仓外，本提交不收录。

## 2026-10-13 rekey 补测（c42；gen 键 6ec1fe9d67dd0733→20c07a1e0b4ad8e8）

测量人：Claude 子代理（主会话派发）；基线 HEAD 8f0466a8，键内输入未被本会话修改。以下均为**本会话实际跑过**，不是设计推断。

**失配原因**：95ddf2de..HEAD 间 gen 键输入变化为 exec/facts/k2-gen2.tsv、exec/facts/k2-libraryenv.tsv（头部 input-sha 行）、exec/lower/codeprivate-result.tsv、exec/lower/codeprivate-manifest.tsv。有序 1142 项完整 SHA1 20c07a1e0b4ad8e8596dac7dae1ba2db966bc96a。parse2 键（exec/facts 亦入键）同步 3d19910f75b9932c→a7bf4527717a6458。seed/gen.c SHA256 未变（b802e4f2…）。

**身份**：Linux/x86_64；/usr/bin/cc→x86_64-linux-gnu-gcc-14，实体 SHA256 a23ecab8…635f，cc (Debian 14.2.0-19) 14.2.0；受约束环境变量均空；无 launcher。按 seedgencheck.sh 的 flags（-std=c99 -O2 -Iseed）经 tests/bound 55 编译 seed/gen.c 到 scratch：rc0，编译最大 RSS 167384 KiB，墙钟 5.84 s；产物 SHA256 f997b25d…1e64，与 BINARY_SHA 相同（实际核对）。

**与原协议的偏差（如实记录）**：原包裹链中的 memscope（仓外 /tmp/cc38-memscope.sh）与 GNU time 在本机已不存在，且 uid 1000 无权在 /sys/fs/cgroup/agent 下建子组。故本轮**没有**子 cgroup 上限、cgroup peak、oom_kill 读数，也没有 checkrun 层。实际链：外层 tests/bound.py 55 → run.py（每条前读 MemAvailable，parse2 类要求约 3.3 GiB 以上，否则跳过不测）→ trss（约 10 行 C：fork/exec/wait4，报 ru_maxrss，与 GNU time %M 同一内核计数）→ tests/bound 50 → gen STAGE OUT FLAGS。串行、一次一个生成器、每条新输出路径。首轮曾用 Python 直接 wait4，子进程继承了父 Python 的 RSS 高水位（77088 KiB 下限污染 e4/o1/prune 等），该轮不高于下限的读数作废；高于下限的读数保留为“首轮”列。常量取本会话有效读数的最大值，floor 到 MiB（+1 MiB 仍由 seedmemory 在求和时加）。

| 路线 | stage / flags | rc | RSS KiB（trss） | 首轮 RSS KiB | max floor MiB | C_RSS 常量 | 墙钟 s | 输出 SHA256 前16 |
|---|---|---:|---:|---:|---:|---:|---:|---|
| e2 | pp --shared-predefines | 0 | 188684 | 188852 | 184 | 184 | 0.50 | f0c692290607379c |
| e1 | lex --typed | 0 | 338752 | 338708 | 330 | 330 | 0.61 | 3f00b9be66c6741c |
| e3 | parse2 | 0 | 1285024 | 1285148 | 1255 | 1255 | 3.89 | 2187cf660f39cc17 |
| e4 | opt --o2 | 0 | 41236 | — | 40 | 40 | 0.15 | 4ee21644f5392ed3 |
| o1 | opt | 0 | 15024 | — | 14 | 14 | 0.04 | 6e740f294876925e |
| prune | prune | 0 | 33772 | — | 32 | 32 | 0.11 | 068a6a1484c61a67 |
| nativeabi | nativeabi | 0 | 305588 | 305748 | 298 | 298 | 0.60 | d493559a12c20258 |
| enc-elf | enc --elf | 0 | 145400 | 145428 | 142 | 142 | 0.41 | 0884964748e67247 |
| enc-macho | enc --macho | 0 | 213740 | 213876 | 208 | 208 | 0.59 | beaec9d639236f29 |
| enc-pe | enc --pe | 0 | 270308 | 270204 | 263 | 263 | 0.88 | 4ce8f01b26227d9d |
| arm-elf | enc/arm --elf | 0 | 145732 | 145708 | 142 | 142 | 0.44 | cf5f3220c468e59e |
| arm-macho | enc/arm --macho | 0 | 213816 | 213808 | 208 | 208 | 0.61 | ab80c0f8dd41cf8c |
| arm-pe | enc/arm --pe | 0 | 270572 | 270392 | 264 | 264 | 0.86 | fb63baf6ddd43844 |
| lower-lnx-x | lower --full | 0 | 197060 | 197024 | 192 | 192 | 0.66 | edf818f08b67fe92 |
| lower-lnx-a | lower --full --arm64 | 0 | 221128 | 221184 | 216 | 216 | 0.72 | 9b6844b089051b71 |
| lower-osx-x | lower --full --osx | 0 | 190240 | — | 185 | 185 | 0.61 | 61766814fb89bad2 |
| lower-osx-a | lower --full --osx --arm64 | 0 | 216504 | — | 211 | 211 | 0.69 | 7ef4018da1cf75d8 |
| lower-win-x | lower --full --win | 0 | 133668 | — | 130 | 130 | 0.43 | 42f43fa335674067 |
| lower-win-a | lower --full --win --arm64 | 0 | 161088 | — | 157 | 157 | 0.48 | a478c8ab8c26ee29 |
| obj-lower-x | lower --full --object | 0 | 201092 | — | 196 | 196 | 0.67 | 01bd401df5a0a06e |
| obj-lower-a | lower --full --object --arm64 | 0 | 225208 | — | 219 | 219 | 0.77 | 358e407cd45e7112 |
| obj-enc-x | enc --object | 0 | 160444 | — | 156 | 156 | 0.51 | 79a028b32c1fdbab |
| obj-enc-a | enc/arm --object | 0 | 163516 | — | 159 | 159 | 0.53 | c107bf06a10ce14b |
| tokenpp | pp --shared-predefines --no-autoinc | 0 | 39416 | — | 38 | 38 | 0.12 | 79c4d6226bc588b7 |
| tokenlex | lex | 0 | 122560 | — | 119 | 119 | 0.39 | e9175597d840a3bd |
| warnlex | lex --locations | 0 | 759664 | — | 741 | 741 | 1.33 | a7e7d7453b3061e6 |
| warnunits | parse2/units --locations | 0 | 201524 | — | 196 | 196 | 0.51 | 07a123cf99ce49c2 |
| warnpp | pp --locations --shared-predefines | 0 | 184760 | — | 180 | 180 | 0.54 | 319c00a445b451c1 |
| warnparse | parse2 --warnings --errors | 0 | 2186288 | — | 2135 | 2135 | 5.70 | 94f49cd50d8a10e3 |
| errorparse | parse2 --errors | 0 | 2171632 | — | 2120 | 2120 | 5.28 | 5b6321a5aea270f2 |
| p2-locations | parse2 --locations | 0 | 1365084 | — | 1333 | 未落常量 | 4.25 | 49990eccd1a183a3 |
| p2-warnings | parse2 --warnings | 0 | 1378860 | — | 1346 | 未落常量 | 4.11 | b36626787d7a40af |
| p2-locations,warnings | parse2 --locations --warnings | 0 | 1378968 | — | 1346 | 未落常量 | 4.47 | b36626787d7a40af |
| p2-locations,errors | parse2 --locations --errors | 0 | 2171656 | — | 2120 | 未落常量 | 5.99 | 5b6321a5aea270f2 |
| p2-locations,warnings,errors | parse2 --locations --warnings --errors | 0 | 2186684 | — | 2135 | 未落常量 | 5.99 | 94f49cd50d8a10e3 |

与 10-10 常量相比只变三项：lower-lnx-a 215→216（首轮 221184 KiB 恰为 216 MiB）、lower-osx-a 210→211、obj-lower-x 195→196；其余 27 项 floor 后相同。p2-* 五行是 parse2 的 locations/warnings 组合，**只记录不落常量**：seedmemory 的 parse2 路线映射只认 x→e3、errors→errorparse，且 tests/seedmemorycheck.py 的 test_unknown_com_and_unmeasured_parse2_flags 断言 parse2 locations 为 UNKNOWN；把这些数接入准入等于改检查断言，需 owner 另裁。p2-locations,warnings 与 p2-warnings、p2-locations,errors 与 errorparse、p2-locations,warnings,errors 与 warnparse 的输出字节相同（见 SHA 列），RSS 也相当。

**Python 参考构造（冷分支，仅信息，不落常量）**：同样串行，tests/bound 55 包 python3 exec/build/gen.py STAGE OUT FLAGS，RSS/墙钟：e4 149196 KiB/1.14s；o1 25548 KiB/0.19s；prune 124272 KiB/0.81s；e2 202532 KiB/3.81s；e1 50884 KiB/0.44s；nativeabi 126256 KiB/1.32s；enc-elf 121476 KiB/1.78s；enc-macho 136892 KiB/2.13s；enc-pe 148360 KiB/2.49s；arm-elf 130564 KiB/2.15s；arm-macho 146236 KiB/2.58s；arm-pe 157140 KiB/3.19s；warnunits 89732 KiB/0.80s；warnpp 203528 KiB/3.91s；lower-lnx-x 197788 KiB/4.78s；lower-lnx-a 218464 KiB/5.04s；lower-osx-x 190536 KiB/4.90s；lower-osx-a 213768 KiB/5.00s；lower-win-x 134856 KiB/2.63s；lower-win-a 158516 KiB/3.00s；obj-lower-x 203632 KiB/5.17s；obj-lower-a 224360 KiB/5.29s；obj-enc-x 139816 KiB/2.37s；obj-enc-a 153172 KiB/2.53s；e3 590356 KiB/17.00s；warnparse 639708 KiB/29.41s；errorparse 626016 KiB/34.07s；tokenpp 51448 KiB；tokenlex 50112 KiB；warnlex 85188 KiB。errorparse 前两次在宿主负载高（load 9.8、可用内存一度约 2 GiB）时超 55 s 被 bound 杀（rc142），第三次 34 s 通过；可见冷构造在繁忙宿主上贴近 60 s 上限。seedmemory 的冷分支仍 UNKNOWN（identity_reason 只认 warm），本次未改。

**落常量**（tests/seedmemory.py）：REFERENCE_KEYS 两键、C_RSS 三项、文档串。BINARY_SHA、C_SHA、EVIDENCE_IDENTITY、阈值公式、检查断言均未动。

**验证（实际跑过）**：tests/seedmemorycheck.py 14 项 OK（改前 2 项失败 1!=4）；gate.sh --suite seedmemory rc0。默认 TMPDIR（/tmp 无缓存）下 seedgen、-2、-3、-4 仍 rc2，原因由 “generator inputs differ” 变为 “unmeasured cache branch: cold”；seedparse2-1/-2 仍 rc2（unmeasured C flags: locations / locations,warnings）；com 路线 seedmemory.py com 仍 rc2（按代码无条件 UNKNOWN；gate.sh --com --suite com-seedgen 另被 product freshness 挡在前面，未进入套件）。在 scratch TMPDIR 预热 30 个 Python 参考后：seedgen rc0（same 15）、seedgen-2 rc0（same 10）、seedgen-3 rc0（same 3）、seedgen-4 rc0（same 4；需求约 5.8 GiB，当时可用约 5.8–6.1 GiB，余量极薄，宿主繁忙时会 rc77）。

**仍 UNKNOWN**：冷缓存分支（任何新 TMPDIR 的首跑；脚本只在准入之后才建参考，所以不预热就过不了准入）、parse2 六个含 locations/warnings 的 flag 组、COM 构造路线、编译资源、四路并发的实测峰值（仍是串行峰值求和）、cgroup 记账峰值（本轮无）。原始 results.tsv / pyresults.tsv / run.py / trss.c 在会话 scratchpad，未入库。
