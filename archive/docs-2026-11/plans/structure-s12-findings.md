# S1/S2 结构调查结果(cc,2026-11-07;私有克隆 /tmp/cc-s12,基线 1d8162b53b)

输入:`plans/structure-5-layer.md`(S1/S2)、`plans/unit-text.tsv`(81 单元 .text,lnx/x86_64 切片 `/tmp/cdx-bom-k2-precheck-04/sl-lnx`)、`plans/slice-units.tsv`、`plans/slice-nn-migration.md`、`tools/com-tree.cx` 输出、`tools/ape-pack.cx`、`tools/build-cxcc-ape.cx`、`tools/release-ape.cx`。

## S2 引导闭包(L2 可迁份额量化)

### 方法
1. `slice-units.tsv` 的 `#pragma unisa use` 列不足以求闭包:40 个单元 uses 为「-」却被符号引用(如 cli/route/deps/tapelink/modeldecode 仅通过符号被 m0/drivelink/executor 调用),按 use 做传递闭包只能到 m0 直接 use 的 14 个模块,不可信。
2. 改用符号引用:对 81 个 `.o` 跑 `nm -g --defined-only`(857 个定义)+ `nm -u`(567 个未定义,除 `__init` 外全部在 81 单元内解析),单元级闭包从 m0.o(dv_main)出发 = 78/81(只剩 drive.o/entryexec.o/hostnative.o 不可达):单元粒度几乎全连通,因为 dv_main 是所有子命令的分发点。
3. 再细到函数级:`objdump -dr --section=.text` 取 R_X86_64_PLT32/PC32 重定位、直接 `call/jmp <sym>` 和 `lea …# <sym>`(函数指针)三类边,共 5,704 条;函数大小按 `nm -n` 相邻符号地址差(`__unisacc_L*` 标号并入所在函数;cx 不发 .size)。根 = `m0.o:dv_main`,切断非默认子命令入口 `dv_as/dv_view/dv_ar`(driveface,default_path=NO)。间接 `call *reg` 的目标只在其 lea 可见时计入,因此函数级数字是下界。
4. 口径 A(主结论,按任务定义「从入口跑完 e2→e1→e3→e4→prune→lower→elf 全流水线」):单元只要有任一函数可达即属闭包。口径 B(敏感性):只从 tape 输入之后的根出发(`dv_read_input/dv_run_route/dv_finish/dv_link_objects_route/dv_run_lower(_n)`),对应「L2 已是 tape、L1 只需 tape→elf」。

### 表一 闭包内单元(口径 A;列:名|.text|分类|A 可达字节|B 可达字节)

| 单元 | .text | 分类 | A 可达 | B 可达 |
|---|---:|---|---:|---:|
| cxmod.o | 120061 | UNBOUNDED | 112972 | 1336 |
| m22.o | 93049 | UNBOUNDED | 62301 | 23960 |
| host.o | 92561 | SYS-IO | 85878 | 69469 |
| tapegate.o | 78680 | UNBOUNDED | 73612 | 73612 |
| driveroute.o | 69288 | SYS-IO | 62356 | 62280 |
| m0.o | 51361 | MIXED | 42482 | 29621 |
| runexec.o | 48286 | SYS-IO | 33760 | 25194 |
| m2.o | 43791 | UNBOUNDED | 35588 | 30503 |
| stages.o | 39347 | MIXED | 37825 | 35827 |
| driveunits.o | 39305 | MIXED | 36664 | 0 |
| drivelink.o | 35019 | MIXED | 32381 | 32381 |
| executor.o | 33978 | UNBOUNDED | 32288 | 29375 |
| m20.o | 33329 | SYS-IO | 26616 | 26616 |
| m5.o | 30660 | UNBOUNDED | 25175 | 25175 |
| hostout.o | 30076 | SYS-IO | 28252 | 28252 |
| m29.o | 30040 | MIXED | 21299 | 21299 |
| tapelink.o | 29890 | UNBOUNDED | 25949 | 25949 |
| m32.o | 26441 | UNBOUNDED | 18038 | 18038 |
| m15.o | 26427 | SYS-IO | 22111 | 21989 |
| modeldecode.o | 25496 | UNBOUNDED | 20248 | 20248 |
| m19.o | 24906 | SYS-IO | 893 | 893 |
| codec.o | 21634 | UNBOUNDED | 14595 | 14595 |
| m30.o | 20805 | MIXED | 17096 | 17096 |
| m10.o | 20625 | SYS-IO | 9949 | 9949 |
| cli.o | 20203 | MIXED | 16615 | 0 |
| deps.o | 19109 | MIXED | 1359 | 0 |
| m35.o | 18075 | UNBOUNDED | 14487 | 0 |
| drivestream.o | 17788 | MIXED | 13053 | 0 |
| m21.o | 14932 | SYS-IO | 3323 | 984 |
| hostres.o | 14545 | SYS-IO | 9795 | 9747 |
| entrybind.o | 14188 | MIXED | 6384 | 3730 |
| args.o | 13796 | MIXED | 8446 | 0 |
| m28.o | 13750 | MIXED | 5151 | 5151 |
| drivetapebin.o | 13582 | MIXED | 7831 | 7831 |
| route.o | 13399 | FINITE | 9793 | 0 |
| fwdstub.o | 12208 | MIXED | 8174 | 8095 |
| m18.o | 11719 | SYS-IO | 7822 | 7822 |
| m34.o | 11620 | MIXED | 8062 | 0 |
| m12.o | 11385 | MIXED | 5918 | 0 |
| m1.o | 11158 | TRANSFORM | 2390 | 1802 |
| m25.o | 10717 | MIXED | 4293 | 4293 |
| packmodel.o | 10613 | UNBOUNDED | 5926 | 2712 |
| m3.o | 10210 | SYS-IO | 5557 | 5009 |
| attributeframe.o | 10060 | MIXED | 6068 | 5207 |
| m31.o | 10002 | MIXED | 5768 | 5768 |
| driverun.o | 9868 | MIXED | 5962 | 4023 |
| m26.o | 9069 | MIXED | 5499 | 5499 |
| m6.o | 8883 | MIXED | 44 | 22 |
| m13.o | 8543 | FINITE | 262 | 262 |
| bindstage.o | 8438 | MIXED | 1772 | 0 |
| m14.o | 8420 | FINITE | 949 | 949 |
| m33.o | 8404 | MIXED | 1234 | 1234 |
| driveplan.o | 7548 | MIXED | 5906 | 0 |
| modelnumber.o | 7338 | FINITE | 2887 | 2011 |
| rescache.o | 7228 | MIXED | 2939 | 2939 |
| input.o | 7180 | MIXED | 3181 | 3181 |
| m7.o | 7099 | UNBOUNDED | 2812 | 2812 |
| m11.o | 6979 | TRANSFORM | 3248 | 3248 |
| m4.o | 6920 | MIXED | 3338 | 1817 |
| transition.o | 6581 | UNBOUNDED | 287 | 0 |
| driveargs.o | 6374 | FINITE | 2810 | 0 |
| m8.o | 6224 | TRANSFORM | 2106 | 1736 |
| compat.o | 6186 | MIXED | 2610 | 0 |
| routestep.o | 5776 | MIXED | 2212 | 2212 |
| m9.o | 5553 | MIXED | 622 | 622 |
| memlayout.o | 5396 | MIXED | 1820 | 1820 |
| m17.o | 5278 | SYS-IO | 1714 | 1714 |
| m16.o | 5166 | MIXED | 1602 | 0 |
| path.o | 5112 | SYS-IO | 1530 | 1478 |
| stagehandoff.o | 4925 | MIXED | 1367 | 1367 |
| m24.o | 4664 | MIXED | 1106 | 1106 |
| m23.o | 4407 | MIXED | 837 | 837 |
| driveutil.o | 4371 | MIXED | 807 | 410 |
| modelload.o | 4359 | UNBOUNDED | 801 | 0 |
| m36.o | 4072 | FINITE | 508 | 0 |
| bind.o | 3763 | MIXED | 30 | 0 |

合计:76 单元,.text 1,548,238 B;其中函数级可达 A=1,097,345 B(59.5% 全切片),B=753,107 B(40.8%)。

### 表二 闭包外单元(口径 A;列:名|.text|分类|default_path)

| 单元 | .text | 分类 | default_path |
|---|---:|---|---|
| m27.o | 223602 | UNBOUNDED | NO |
| driveface.o | 61641 | MIXED | NO |
| drive.o | 5031 | MIXED | YES |
| entryexec.o | 3610 | MIXED | YES |
| hostnative.o | 3558 | SYS-IO | YES |

合计:5 单元,297,442 B;其中 UNBOUNDED+MIXED = 293,884 B(m27 asmtext、driveface、drive.o、entryexec)。
注:drive.o/entryexec.o/hostnative.o 与 m0/host 同源或为备用入口对象,未被 entry 链接路径引用;真正可迁的是 as/view/ar 子命令树(driveface+asmtext,285,243 B)。

口径 B 闭包外:23 单元 514,609 B(UNBOUNDED+MIXED 487,206 B),多出的 18 单元是 CLI/参数/路由/包解码/单元装配(args、cli、compat、deps、driveargs、driveplan、drivestream、driveunits、route、bind、bindstage、m12、m16、m34、m35、m36、modelload、transition)——这才是「L2 program」想迁走的驱动面;但它们是 L1 从源码编出 L2 的必需件,只有当 L2 以 tape 分发且 L1 不再需要从源码自举时才出闭包。

### 结论
**L2 可迁部分占四片原生份额的百分比:单元粒度 16.1%(297,442/1,845,680;四片同构,折合 gzip 后约 237 KB/1,474,065 B);函数级死代码上界 40.5%(748,335 B 未从 dv_main -o 路径可达,但散落在 71 个单元内,不能按单元整体迁出);口径 B(tape 分发)单元粒度 27.9%。** 三个数都低于「L2 值得做」的直觉阈值(结构页记 tape.gz≈原生份额 45%,而可迁 16–28% 要换来首编 25–47 s + L1 自举循环),建议 0.0.48 不把 L2 列主线;更划算的是函数级裁剪(链接期丢弃不可达函数,40% 上界)和 S3 运行库单份化。

## S1 L0 中立化备忘:win/x86_64 PE 头
1. 现状:`ape-pack.cx` 把 win/x86_64 PE 原样放在 APE 头部(`pe_with_stub`),DOS 头 64 字节内嵌 `qFpD='…'` 让 sh 把整个文件当脚本;四片 Unix 以 gzip 成员(默认 stored,`--deflate` 用 `cx/text/deflate.cx`)跟在后面,win/arm64 作第五片只被携带不被执行;`build-cxcc-ape.cx`/`release-ape.cx` 同一布局。com-tree:头 2,631,168 B = 42.6%。
2. 该 PE:PE32+ 控制台,.text 2,581,208 B、.data 45,056 B、.reloc 12 B(无重定位,固定基址 0x140000000)、.rsrc 512 B;只导入 kernel32 26 个符号(VirtualAlloc/VirtualProtect/LoadLibraryA/GetProcAddress/CreateProcessW/CreateFileA/MoveFileExA/DeleteFileA/WriteFile…),无 zlib 依赖;产品自带 inflate(`cx/runtime/codec.cx`,PE 内字符串 `source=inflate`)。
3. 实测 `gzip -9` 头部 2,631,168 → 391,110 B,可省 2,240,058 B(APE 6,163,990 的 36.3%),减去薄引导估 40–60 KB,净省约 2.18 MB,APE 落到约 3.9 MB;再叠加四片 Unix 从 stored 改 `--deflate`(com-tree 显示已是 deflate 比例 12–19%)无额外收益。
4. 薄引导路径甲(与 Unix sh 启动器同构,零新能力):薄 PE = DOS 存根(保留 sh polyglot)+ cx 写的 inflate + 以 `o=/n=` 找到自身 win 切片 → 写 `%TEMP%\unisacc-<sha>.exe`(CreateFileA/WriteFile/MoveFileExA 已导入)→ CreateProcessW 等待退出码;文件缓存按 sha 复用。路径乙(内存加载):VirtualAlloc 固定基址 0x140000000 映射节、GetProcAddress 解析 kernel32 导入、VirtualProtect 置 RX、跳入口;因 .reloc 为空必须拿到首选基址,DllCharacteristics 0x8140 含 DYNAMIC_BASE 时需清位或补重定位表;比甲多一项原生验证且无文件缓存。
5. Windows 侧所需能力:无需系统 zlib(`cxcodec_inflate` 已在产品内,可单独编成薄引导);需要 %TEMP% 可写+可执行(企业策略/AppLocker 可能禁,甲路径风险点;乙路径规避但需 ASLR 处理);Defender/SmartScreen 对自解压再启动的子进程会二次扫描(首启慢,签名策略不变)。
6. 对「六片同候选原生证据」的影响:被执行的 win/x86_64 本体仍是同一候选字节(sha 入 manifest,薄引导只做解压/转交),证据不降级但需新增原生验证项:(a) 薄引导在真 Windows 上 inflate→落盘→exec 往返与退出码透传;(b) win/arm64 经 x64 仿真走同一薄引导;(c) `check-booter-ape`/`check-install-ape`/`com-tree` 的头部识别要从「MZ+PE 原样」改为「MZ+薄PE+gzip 成员」,否则棘轮误判;(d) `formats-macho-pe.cx` 校验薄 PE 结构。
7. 结论:可行且省 36%;前置条件是 cx 能把 codec+sy_* 文件/进程原语编成独立小 PE(当前 win 薄层 sy_* 已覆盖 CreateProcessW/MoveFileExA),建议甲路径先行,乙路径作 0.0.49 研究项。

## 方法可复现
`nm`/`objdump` 脚本在会话 scratchpad(未入库);结果表来自 `/tmp/cdx-bom-k2-precheck-04/sl-lnx/*.o`(unit-text.tsv 同一来源)。gzip 实测:`head -c 2631168 unisacc.com | gzip -9 | wc -c` = 391110。
