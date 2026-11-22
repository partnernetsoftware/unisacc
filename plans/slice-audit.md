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
   - 现有 `cx/text/deflate.cx` 已有纯 `.cx` 解压实现, 可编译出一个极小 `win/x86_64` 桩 PE(仅需 `CreateFileA`/`ReadFile`/`WriteFile`/`CreateProcessA`, 无外部 DLL 依赖), 将自身 gzip 切片解压至 `%LOCALAPPDATA%\unisacc\unisacc-<key>.exe` 后拉起并透传命令行与退出码, 复用与 Unix 相同的 `UNISA_CONTAINER` 环境变量机制。
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
