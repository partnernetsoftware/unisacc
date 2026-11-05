# lab-loader —— 分拆加载实验线（L1–L5 + P3 单模型替换）

实验线,不改正式产品。正式构造仍走 pack 链,产品身份不经过这里。
合同页见 [CONTRACT.md](CONTRACT.md),逐条实测数字与发现见 [experiments.tsv](experiments.tsv)。

---

## 已归档（2026-11-05，被 `unisacc stream STAGE` 取代）

L1（`slice.cx`/`check-slice.cx`）、L2（`cx0-slice.cx`/`cx0-main.cx`/`check-cx0-slice.cx`）
与 L7（`archive/lab-loader-legacy/tools/phase-pipe.cx`/`archive/lab-loader-legacy/tools/stream-runtime.c`/
`archive/lab-loader-legacy/tools/lab-loader/check-phase-pipe.cx`）已归档到
[archive/lab-loader-legacy/](../../archive/lab-loader-legacy/)，对应批次退役并逐断言记去向
（[retired-batches.tsv](../../archive/lab-loader-legacy/retired-batches.tsv)）。

分段管道现在是产品子命令 `unisacc stream STAGE`（`src/drivestream.cx`）：
`e2 e1 e3 e4 prune lower elf tapecheck units` 每个阶段都是一个 stdin→stdout 过滤器，
跑的是同一份包与同一套网。校验批次是 `tests/check-stream-pipe.cx`（批次 `stream-pipe`，
与 `-o` 逐字节相同）和 `tests/check-cx-stages`/`tests/check-stages.cx`（批次 `stages`，
分段用例 + 链式约束 + 锚点）。

---
## L3 通用 boot 头（`boot-head.cx` + `boot-native.cx`）

```
sh ./unisacc.com tools/lab-loader/boot-head.cx -- NEW_OUTDIR [TARGET...]
sh ./unisacc.com -b lnx/x86_64 tools/lab-loader/boot-native.cx -o OUT/boot-native-lnx-x86_64.com
sh ./unisacc.com tools/lab-loader/check-boot-head.cx      # 批次 boot-head-check
```
多格式头（shell + MZ 前缀）→ 每宿主一小段原生代码（读 manifest、选 base、**boot 层校验 sha256**）→ exec；
shell 只转交，不做判断。不依赖宿主 sha256sum（macOS/Windows 都没有）。
gate：头两代字节相同；头里无 entry.elf/P3/流名；同一头换 manifest 分别启动带包产物与最小程序；
sha 改一字符必具名拒绝；三类缺失具名拒绝；17 项。
数字：纯 CX 的 SHA-256 → 1.18MB 校验约 139ms（占启动 91%）；改走 AF_ALG 后约 16ms（152→30ms）。
启示：平台差异要收进自己的代码（L0 口径），而不是假定宿主有某个工具。

## L4 实验表（`experiments.tsv`）

产品七流（`UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1`，5 次中位）：墙钟 **152.7ms**；construction 48.05ms
（其中七阶段 inflate 80.39ms）；inference 11.72ms；lookup_hit 0。阶段 inflate：e3 46.8 最大。
单流分项：e2 13.81 / e1 9.31 / e3 39.90 / e4 5.09 / prune 4.19 / lower 12.43 / elf 6.47（空程序基线 0.39）。
关键 FINDING：同一 SHA-256 任务，cx 生成代码 939ms vs 宿主 68ms（**约 14 倍**）——
生成代码质量是独立于编译速度的一条提速线。

## L5 单 NN 六架构模板（`template.cx` + `net-check.cx` + `utf8-gen.cx`）

```
sh ./unisacc.com tools/lab-loader/template.cx -- NEW_OUTDIR MODEL_BIN MODEL_NAME [TARGET]
sh ./unisacc.com tools/lab-loader/net-check.cx -- TABLE NET        # 批次 net-check
sh ./unisacc.com tools/lab-loader/utf8-gen.cx -- --check|--write ROOT
sh ./unisacc.com tools/lab-loader/check-utf8.cx                    # 批次 utf8-net
sh ./unisacc.com tools/lab-loader/check-net-check.cx
```
模板 = boot（782B）+ 原生段（102,886B）+ 执行器切片（302,684B）+ 唯一模型槽 + manifest。
模型槽按输入自动选包版本：文本网 → P2 明文；已压缩 M 记录 → P3 原样；UNINETB1 → P3 codec=1。
`net-check` 用产品同一 `cxmd_decode` + `core_transition` 逐键穷举（prune 228,848 观测、UTF-8 3,614 观测）。
`check-utf8` 另有手写 RFC 3629 规则 oracle（不读任何表），22 组样例逐一对齐。
gate：**换权重不换基座**（base/boot/原生段字节相同，只有模型与 manifest 变）；换装 L1 的 e1 与 L1 逐字节相同；
utf8.com 四类输入与 oracle 一致；执行器占 99.66%，模板开销 <1ms。
启示：模板体积由执行器决定（99.66%），模型轻则启动接近 base 自身成本。

## L6 ISA 切片由模型物化（实验,无新工具;命令都是产品现成的）

```
./unisacc.com -S t3.c -o t3.tape                 # 592B
./unisacc.com t3.tape -b lnx/x86_64 -o a         # 与直接编 t3.c 逐字节相同
./unisacc.com t3.tape -b lnx/arm64 -o b          # 同一 tape 得到 aarch64 ELF
./unisacc.com -S -c -b lnx/x86_64 -funit SRC -o OBJ.tape   # 逐对象（整程序 -S 不通）
```
同一份与架构无关的 tape 能写出六个目标镜像,这就是「ISA 切片由模型推导出来」的实测依据。
① 体积:43 个对象 tape 合计 **7,941,104B**（合并 gzip 710,138B / xz 325,964B）。
   一份 tape 要替换的是**五个原生切片**,不是一份 entry.elf:四片 Unix 切片合计 8,365,157B
   （gzip 1,157,093B / xz 676,892B）,加 win/x86_64 701,440B 共 9,066,597B ——
   所以 xz 后 326KB 的 tape 相对约 677KB 的 Unix 切片净省约 **350KB**,
   相对五片全量（xz 约 800KB）净省约 **470KB**;用 gzip 的 710KB 则约省 447KB / 90KB。
   整编译器单文件 `-S src/unisacc.cx` 报 `undefined function`（43 对象链接而成,与 UNITOK2-CAP 同根）。
   重复度:12,412 个函数、唯一函数体 5,557 个;函数体字节 1,121,079,重复 511,986（**45.7%**）——
   43 个对象各自编进同一套 libc/运行时,与 C44-LIBCUNIT-01 同根;去重能把真实体积再压近一半。
   tapebin:只支持整文件 `-S`,与 `-c -funit` 在 route 层互斥,对象级产不出;可测样例
   t3 383B、net-check 207,002B（同为 32% 的文本 tape 体积）。
② 物化对拍:t3（592B tape）与 tools/lab-loader 的工具级 tape 都与直接编译逐字节相同。
③ 耗时:`ms ≈ 95 + 0.00117 × tape_bytes`（4 点拟合）;t3 76ms、525KB 756ms、845KB 1,050ms。
   外推到整编译器 7.94MB ≈ **9.4s** 首次物化（按对象合并后一次 lower+elf）。
   注意:此曲线含固定启动约 74ms（空 tape 实测）,净物化约 `0.00117 × bytes`。
④ 前置条件:整编译器 tape 需要「43 个对象先链接成单一 tape」的能力,当前 `-S` 只处理单编译单元,
   整程序路线受 `undefined function`（forward-signature）阻塞。可行路径是先链接后 `-S z`,
   或让物化器直接吃「对象 tape 列表」。二者都不在 lab-loader 现有范围,照实记为缺口。

## P3 单模型替换（`p3-swap.cx`）

```
sh ./unisacc.com tools/lab-loader/p3-swap.cx -- PKG INDEX OUT NEW_NET
sh ./unisacc.com tools/lab-loader/p3-swap.cx -- PKG MODEL OUT NEW_NET --model
sh ./unisacc.com tools/lab-loader/check-p3-swap.cx         # 批次 p3-swap
```
换一张网不必跑完 12 分钟整链：按包版本正确编码 + 改 M 行（压缩长/原始长/codec/CRC32）→ 输出新包正文。
gate：原 M 记录自替换输出与原包**逐字节相同**；序号与名字等价；替换后 e1 正文一致且单流输出相同；
越界/未知模型具名拒绝且不落盘；改坏 crc 被 runtime 具名拒绝（11 项）。
启示与限制：只给解压后正文时 `dz_deflate` 产 52,217B 而产品包是 51,848B（解压一致）——
这是 P3B 换标的动因；P3B 落地后字节一致性可恢复。

---

## 已知约束（详版见 CONTRACT.md §7）

- **P2 与 P3 互斥**：P2 的 M 行只有长度、载荷必须明文且首字节 `N`；P3 是四字段 + raw deflate，
  解压后必须 `UNINETB1` 开头。选错版本得到 `package requires networks` / `package requires binary networks`。
- **EOF 必须是终止动作**（`[[ACCEPT]]`）；写成回到读键状态的空动作会自环到 step budget exhausted。
- **状态名不进 tbl/net 的 S 表**（S 只存动作引用的字符串），按名找状态要读原始表 DSL 或序号。
- 执行器切片与宿主绑定：换模型不换基座成立，换宿主必须换 base。

## 模块符号链接

`cxlib/`、`runtime/`、`core/`、`mem/`、`sys/`、`cx0/`、`extx/` 下的是指向仓内真实模块的符号链接
（mode 120000，不计 toolsloc）：`#pragma unisa use` 只在用者文件旁解析模块名。
包根解析已实现于 c3ee8ee247（`cx/spec.md` CX-U-7），产品安装后这些链接可统一删除。
