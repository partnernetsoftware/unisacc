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
