# 六切片 U1 头部与填充审计报告

审计对象: 已装产品 `./unisacc.com` (`unisacc 0.0.93`, `sha256=c561e7e0f24d9b82e71b40c86920bc92c12b7035751c8d6e958930bbfe1dd843`, `8,997,165 bytes`) 及同链 (`a206d09fa1`) `fixedpoint/g2/` 切片。
审计工具: `tools/slice-audit.cx` (自测批次 `slice-audit` / `tools/check-slice-audit.cx`), `tools/com-tree.cx`。

## 一、`com-tree.cx` 填充段 100% 字节归属(并非空白填充)

### 1. `sh ./unisacc.com tools/com-tree.cx -- ./unisacc.com` 原行输出

```text
./unisacc.com  8997165 bytes  sha256 c561e7e0f24d9b82e71b40c86920bc92c12b7035751c8d6e958930bbfe1dd843
head: PE image + shell launcher       3419136   38.0%  win/x86_64 stored raw; the DOS stub holds the sh launcher
slice Linuxx86_64                      507505    5.6%  gzip; inflates to 2379494 bytes (21%)
(padding)                                  15    0.0%
slice Linuxaarch64|Linuxarm64          483718    5.3%  gzip; inflates to 2571950 bytes (18%)
(padding)                                  10    0.0%
slice Darwinx86_64                     525240    5.8%  gzip; inflates to 3104482 bytes (16%)
(padding)                                   8    0.0%
slice Darwinarm64                      524679    5.8%  gzip; inflates to 4194274 bytes (12%)
(padding)                              632393    7.0%
package (P4)                          2904461   32.2%  P 4; 40 models, 1823 stage rows, 77 resources
```

### 2. 核心发现: `632,393 B` (0.0.92 口径 `631,744 B`) 的逐段精确归属

`tools/com-tree.cx` 仅扫描 DOS stub shell 脚本中 4 条 Unix `LABEL) o=... n=...` 行, 未解析 `tools/ape-pack.cx:540-549` 紧随其后写入的第 5 个切片(`win/arm64` gzip)与两份 macOS `cache-manager` gzip (`h=`/`z=` 字段), 因而把这三段有效载荷整体误标为 `(padding)`。

用 `tools/slice-audit.cx` 逐字节归属(`accounted=8,997,165 == total=8,997,165`, 0 未归属字节):

| 偏移 | 长度 (B) | 归属段 | 说明 |
|---|---|---|---|
| `0` | `3,419,136` | `head_pe_launcher` | `win/x86_64` 原始 PE + DOS stub shell launcher + 版本资源(PE 对齐已为 16 倍数, 尾对齐 `0 B`) |
| `3,419,136` | `507,505` | `slice_Linuxx86_64` | gzip 成员, 解压 `2,379,494 B` |
| `3,926,641` | `15` | `align_pad` | 16 字节对齐全零填充 (`zero=1`) |
| `3,926,656` | `483,718` | `slice_Linuxarm64` | gzip 成员, 解压 `2,571,950 B` |
| `4,410,374` | `10` | `align_pad` | 16 字节对齐全零填充 (`zero=1`) |
| `4,410,384` | `525,240` | `slice_Darwinx86_64` | gzip 成员, 解压 `3,104,482 B` |
| `4,935,624` | `8` | `align_pad` | 16 字节对齐全零填充 (`zero=1`) |
| `4,935,632` | `524,679` | `slice_Darwinarm64` | gzip 成员, 解压 `4,194,274 B` |
| `5,460,311` | `9` | `align_pad` (原 632,393 B 内) | 16 字节对齐全零填充 (`zero=1`) |
| `5,460,320` | `546,584` | **`hidden_slice_win_arm64`** (原 632,393 B 内) | 第 5 个切片 `win/arm64` gzip 成员(`1f 8b 08 ...`), 解压 **`4,575,744 B`**(含 512 B 版本资源); 未列于 Unix sh 脚本故被 `com-tree` 计入 padding |
| `6,006,904` | `8` | `align_pad` (原 632,393 B 内) | 16 字节对齐全零填充 (`zero=1`) |
| `6,006,912` | `43,733` | **`cache_manager_Darwinx86_64`** (原 632,393 B 内) | macOS x86_64 缓存管理器 gzip (`h=6006913 z=43733`), 解压 `247,906 B` |
| `6,050,645` | `11` | `align_pad` (原 632,393 B 内) | 16 字节对齐全零填充 (`zero=1`) |
| `6,050,656` | `42,036` | **`cache_manager_Darwinarm64`** (原 632,393 B 内) | macOS arm64 缓存管理器 gzip (`h=6050657 z=42036`), 解压 `264,418 B` |
| `6,092,692` | `12` | `align_pad` (原 632,393 B 内) | 16 字节对齐全零填充 (`zero=1`) |
| `6,092,704` | `2,904,461` | `package_p4_plus_footer` | P4 模型包 `2,904,445 B` + 16 B `UNIPKG1` footer |

- **结论**: `com-tree.cx` 报告的 `632,393 B` `(padding)` 中, **`546,584 B` (86.4%) 是 `win/arm64` 压缩切片**, **`85,769 B` (13.6%) 是两份 macOS `cache-manager` 压缩二进制**, **无 PE 签名预留**, 真正的 16 字节对齐零填充全文件合计仅 **`73 B`** (`15+10+8+9+8+11+12`)。因此该段**没有可回收的 630 KiB 空白填充**(唯一可省空间是 U1 架构决策: `win/arm64` 已单独作为 Release 资产 `slice-win-arm64` 分发, 若不再同时嵌进 APE 可立省 `546,592 B`)。

## 二、`win/x86_64` PE 头部 `gzip -9` 实测与压缩可行性评估

### 1. 实测数值
- 裸切片 `fixedpoint/g2/win-x86_64/slice-win-x86_64`: 原始 `3,416,064 B` -> `gzip -9` **`538,500 B`** (`15.8%`, 可省 `2,877,564 B`)。
- `unisacc.com` 头部(`head_pe_launcher`, 含 DOS stub shell launcher + 版本资源): 原始 `3,419,136 B` -> `gzip -9` **`539,557 B`** (`15.8%`, 可省 **`2,879,579 B`** / ~2.75 MiB, 与预估 `538,936 B` 仅差 `621 B`)。
- 若进一步采用“极小 Windows PE 自解压/加载桩 (~16-32 KiB) + `win/x86_64` 改作第 5 个 gzip 切片”, `unisacc.com` 总体积可从 `9.00 MiB` 降至约 **`6.16 MiB`**(若同步去掉重复内嵌的 `win/arm64` 则降至 **`5.61 MiB`**)。

### 2. 可行性与风险评估
1. **自解压桩实现复杂度(中等, 可控)**:
   - 现有 `fab2451e91822a9:cx/text/deflate.cx` 已有纯 `.cx` 解压实现, 可编译出一个极小 `win/x86_64` 桩 PE(仅需 `CreateFileA`/`ReadFile`/`WriteFile`/`CreateProcessA`, 无外部 DLL 依赖), 将自身 gzip 切片解压至 `%LOCALAPPDATA%\unisacc\unisacc-<key>.exe` 后拉起并透传命令行与退出码, 复用与 Unix 相同的 `UNISA_CONTAINER` 环境变量机制。
2. **Windows 启动延迟(低风险, 冷/热分离)**:
   - 首次冷启动解压 `3.4 MiB` 约 20-40 ms, 命中缓存后仅增加一次极小桩进程 `CreateProcess` 开销(~5-15 ms), 对编译器场景可接受; 但须注意管道/句柄继承、Ctrl-C 信号与父子进程退出码透传。
3. **杀软与 SmartScreen 误报(高风险, 最大阻力)**:
   - 无签名 PE 在运行期从自身尾部解压并落地再 `CreateProcess` 执行 `.exe`, 是 Windows Defender / 第三方杀软典型的 dropper 行为特征, 极易触发启发式拦截或 SmartScreen 阻断;
   - 缓解手段需代码签名证书(EV/OV)或改为“仅发布单平台包 + 保留原始未压缩 PE 作为 Windows 形态”。建议 U1 路线先评估去掉冗余内嵌 `win/arm64`(立省 `546 KiB`、零杀软风险), 再权衡是否引入 Windows 自解压桩。

## 三、`osx/arm64` `.text` 重量重测与 `plans/unit-text.tsv` (0.0.87 `lnx/x86_64`) 对照

### 1. 实测原行 (`tools/slice-audit.cx --macho ... --unit-tsv plans/unit-text.tsv`)
- `MACHO_AUDIT file=.../fixedpoint/g2/osx-arm64/slice-osx-arm64 bytes=4194274 cputype=0x100000c text=4106384 seg_text=4112384 seg_data=49152 seg_linkedit=32738 nsyms=0`
- `UNIT_TEXT_REF file=plans/unit-text.tsv units=99 text_total=2441257 obj_total=15197776 default_yes=71 default_yes_text=1649901`

### 2. 对照结论与 `UNMEASURED` 说明
| 口径 | 目标 | 总 `.text` (B) | 镜像总大小 (B) | 逐单元明细 |
|---|---|---|---|---|
| 旧口径 (`plans/unit-text.tsv`, 0.0.87 `g3`) | `lnx/x86_64` | `2,441,257` (99 单元; `default_path=YES` 71 单元占 `1,649,901 B` / 67.6%) | `2,379,494` (0.0.93 `lnx/x86_64`) | 已实测 (99 单元) |
| 本次重测 (0.0.93 `a206d09fa1` `g2`) | `osx/arm64` | **`4,106,384`** (`__TEXT,__text`, 占镜像 97.9%; `__TEXT` 段 `4,112,384 B`, `__DATA` `49,152 B`, `__LINKEDIT` `32,738 B`) | `4,194,274` | **`UNMEASURED`** |

- **差异解释**: `osx/arm64` `__TEXT,__text` (`4,106,384 B`) 为 `0.0.87` `lnx/x86_64` 99 单元 `.text` 总和 (`2,441,257 B`) 的 **`1.682x` (`+1,665,127 B`)**, 主要源于 ARM64 定长 4 字节指令密度(x86_64 变长短指令更紧凑)以及 `0.0.87 -> 0.0.93` 期间新增/扩展单元。
- **逐单元 `UNMEASURED` 原因**: 整链缓存 `fixedpoint/g2/osx-arm64/` 仅保留链接产物 `slice-osx-arm64` 且已 strip (`nsyms=0`), 未保留 99 个中间 Mach-O `.o`; 按本轮不跑重负载约束未重编 99 单元, 如实标 `UNMEASURED`。后续若需 `osx/arm64` 逐单元分解, 在独立构建目录编出 99 个 `osx/arm64` `.o` 后直接用 `tools/slice-audit.cx --macho <unit.o>` 即可逐一读取 `__TEXT,__text` 精确尺寸。

## 四、U6 核实: APE 内嵌 `win/arm64` 第 5 切片依赖审计

**判定: `可去重` (无需改启动器; 去重前须同步改造 4 组非运行期打包/导出/测试消费者)**。

以下所有条目均区分「已读证据」与「推断」, 均未修改产品输入、启动器或 `release/`。

### 1. APE 启动器在 Windows arm64 上是否从 APE 自身解出第 5 切片
- **结论: 否, 运行期完全不解出(纯死字节)**。
- **已读证据**:
  - `tools/ape-pack.cx:92-97`: 源码注释明文说明 Windows 直接由 OS PE 加载器执行外层 `win/x86_64` PE, 忽略 DOS stub shell 脚本; 第 5 个 payload 切片(`win/arm64`)虽被打包进文件但没有任何 `case` 分支(`NAME[4]` 仅含 `Linux|x86_64`、`Linux|aarch64`、`Darwin|x86_64`、`Darwin|arm64`)。
  - `tools/ape-pack.cx:188-197` (`managed_script`) 与 `tools/ape-pack.cx:231-242` (`script`): 两条 shell 启动器生成路径均只循环 `i = 0..3`, `off[4]`、`len[4]`、`key[4]` 从未写入 DOS stub 脚本; 未知 `$u` 落入 `*) exit 1;;`。
  - `tools/ape-pack.cx:540,548`: 仅在打包落盘时按 `nposix = 5` 把 `img[4]` (`win/arm64` gzip, 546,584 B) 写入 APE, 并在 `tools/ape-pack.cx:526` 将 `payload_sha[4]` 计入 `UPI1 pid`。
  - `tools/cache-retain.cx:1-68`: 仅由 macOS (`Darwinx86_64`/`Darwinarm64`) 启动器调用的 POSIX 缓存保留管理器(`tools/ape-pack.cx:194,207-224`), 不涉及 Windows, 不解包 APE。
  - `seed/ape.c:3,14,105-117,268`: 仅接受 5 个位置输入(`WIN_X86_64` + 4 POSIX 切片, `argc != 7` 直接拒绝), 根本不支持第 5 payload。
  - `.github/workflows/candidate-native.yml:77,198-199,389`: 明确注明 APE 外层 PE 是 x86_64, 在 Windows arm64 上直接跑只会走 x86_64 仿真, 而非解出内嵌 `win/arm64`。

### 2. Release 独立资产 `slice-win-arm64` 的消费者是否已可替代
- **结论: 是, 所有 Windows arm64 运行路径与原生 CI 本来就只消费独立资产 `slice-win-arm64` + `pkg.bin`, 已完全替代**。
- **已读证据**:
  - `README.md:111`: Windows arm64 行明文写明 `yes (via slice-win-arm64 and pkg.bin, see the release notes)`。
  - `tools/release-run.cx:16,460-468`: 发布链步 5 (`step_inject`) 直接对裸切片 `win-arm64` 调用 `tools/version-inject.cx` 生成独立资产 `slice-win-arm64`, 不从 APE 抽取。
  - `tools/release-layout.cx:15-16`: Windows arm64 启动脚本 `run.cmd` 直接设 `UNISA_CONTAINER=%~dp0pkg.bin` 并运行 `%~dp0slice-win-arm64.exe`。
  - `.github/workflows/candidate-native.yml:77-94,227,340-352,632-634`: `win/arm64` 原生 CI 直接下载独立资产 `slice-win-arm64` 拷贝为 `out/slice-win-arm64.exe` 执行全部原生测试。
  - `tools/install-ape.cx:1-250`: 仅把 `unisacc.com` 安装到 POSIX 根并以 `sh APE --version` 验活(`install-ape.cx:113-124`), 不消费 `slice-win-arm64`, 也不解析 APE 第 5 切片。
  - `release/README.md:34`、`release/SHA256SUMS.template:5`、`release/candidate.tsv:2`、`tools/release-prep.cx:331`、`tools/release-decide.cx:6,79`: 六资产发布契约已将 `slice-win-arm64` 与 `pkg.bin` 作为独立一等资产发布与核验。

### 3. 若去掉内嵌 `win/arm64`, 会失效的检查、工具与证据(全部已读核实)
1. **高影响 -- `releaseoneshotpack` 独立 6 导出链路(强依赖 APE 内嵌第 5 gzip 切片)**:
   - `tools/cxlib/releaseoneshotpack.cx:48-59`: `rop_load()` 在 4 个 POSIX 切片后强制扫描并解压 `win/arm64` gzip 成员, 缺失即报 `rc=2` (`missing win/arm64 slice` / `invalid win/arm64 slice`)。
   - 受连带影响的消费者: `tools/release-oneshot.cx:66-67` (`--pack`/`--check-pack`)、`tools/cxlib/releaseprepare.cx:186,211`、`tools/cxlib/releaseexec.cx:390,479` (`assetguard` 与 `public` 读回校验会直接红)、`tools/check-releaseoneshotpack.cx:22-31,47-48` (`missing APE win-arm slice rejected`)、`tools/check-releaseexec.cx:90`。
2. **中影响 -- 整链与发布打包参数及 `ape-receipt.txt` `win/arm64` 行**:
   - `tools/candidate-chain.cx:962-963,3541,3555-3558`、`tools/release-run.cx:436-453`、`.github/workflows/macos-sign.yml:216-226`: 三处调用 `release-ape.cx pack` 时均显式传入第 6 切片 `win-arm64`(`tools/ape-pack.cx:488-489` 与 `tools/release-ape.cx:88,123` 原本即已支持 5 切片模式)。
   - `tools/pack-srcdir.cx:10-11,65-72` 与 `tools/check-pack-srcdir.cx:20-32`: 强制要求 `ape-receipt.txt` 含 `win/arm64` 路径行以装配 `SRC_DIR/win-arm64`(供 `release-run` 步 5 `version-inject` 生成独立 `slice-win-arm64`)。若 `release-ape pack` 改为 5 切片且不写 `win/arm64` 收据行, `pack-srcdir` 将 `REFUSE missing 1: win-arm64`。
   - `tools/check-release-ape.cx:28,41-42`: 断言 `six-slice replacement remains supported` 与 `legacy six positional inputs remain supported`。
3. **中影响 -- 审计自测与身份变化**:
   - `tools/check-slice-audit.cx:45,67-80`: 断言已装 `./unisacc.com` 含 `AUDIT_SEG\thidden_slice_win_arm64\t` (`inflates=4575744`) 并在偏移 `5460320` 做篡改负控; 去重版安装为本机 Root 时须同步更新。
   - `tools/ape-pack.cx:526`: `UPI1 pid` 与 APE `sha256` 会因少计 `payload_sha[4]` 而改变(仅影响新版本, 属预期; `tools/release-packcheck.cx:75-86` 只要整链与 `release-run` 两端同步去重仍恒等)。
4. **无影响(已读确认)**:
   - `tests/formats-elf-ape.cx:151-190`(仅检 0.0.43 5 切片 APE)、`tools/check-ape-pack.cx:16,34`(仅测 macOS 缓存)、`tests/runcache-roots.cx:181-195`(仅读前 8 KiB `UPI1 pid=`)、历史 `release/evidence/0.0.71..0.0.93/`(历史版本不可变证据)。
5. **推断(未实跑验证)**:
   - 去掉内嵌 `win/arm64` 后, 新版 `unisacc.com` 预计立省 `546,592 B` (`546,584 B` gzip + `8 B` 16B 对齐零填充, 约 `6.08%`), 且对 Windows/Linux/macOS 运行期零影响; 若将 `releaseoneshotpack` 改为从 `slice-win-arm64` 独立资产读取 `win/arm64`(或接受显式 `--win-arm64 PATH`)并保持 `pack-srcdir` 可取到 `fixedpoint/g2/win-arm64/slice-win-arm64`, 即可安全落地 U6。
