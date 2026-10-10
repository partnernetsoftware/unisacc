# K5 首具名切口：exec/ 生成器与检查盘点 + 1 条可批草案

**状态（2026-10-10 23:34 CST，机房主任 23:32 自裁）**：盘点与**一条**可批切口草案入仓；**不**整锅迁移；**不**未批实现；**不**旧全量空转。tip `92a96f22`；`version.h` 仍 0.0.39。roadmap：0.0.40 = K5（cc 检查脚本 / cdx 生成器）；δ 构造器已有 `seed/gen.c` 先例。

## 盘点（git 跟踪，exec/）

| 类 | 数量 |
|---|---|
| `.py` | 119 |
| `.sh` | 56 |
| 合计 | 175 |
| 名含 check 的 .py/.sh | 111 |
| `*-manifest.tsv` / `gen-manifest.tsv` | 132 |

### 按子目录（py / sh / check*）

| 子目录 | .py | .sh | check* |
|---|---:|---:|---:|
| (root) | 2 | 3 | 1 |
| build | 5 | 0 | 0 |
| c | 35 | 24 | 30 |
| enc | 21 | 9 | 29 |
| facts | 2 | 0 | 0 |
| ffi | 2 | 1 | 3 |
| lex | 10 | 3 | 5 |
| lower | 6 | 5 | 11 |
| opt | 0 | 1 | 1 |
| parse | 2 | 1 | 0 |
| parse2 | 12 | 2 | 13 |
| pipeline | 11 | 4 | 9 |
| pp | 9 | 3 | 7 |
| prune | 2 | 0 | 2 |

### 生成器入口（具名）

| 入口 | 角色 |
|---|---|
| `exec/build/gen.py` | 通用 STAGE→δ JSON 驱动（读 `exec/STAGE/gen-manifest.tsv` 或 `STAGE/SUB-manifest.tsv`） |
| `exec/assemble.py` / `exec/finite_rules.py` | 清单解释与有限规则 |
| `exec/build/{graph,parsebase,parse2base,procs}.py` | 图/基座 |
| `seed/gen.c`（`seed-gen`） | C99 构造器先例：特殊路径 prune / enc / enc/arm / lower / parse2；泛化路径接受 `#! base build/parsebase.py` 或 `build/graph.py` + `#! flags`/`#! start`/`#! graph G` |
| `exec/stamp.sh` | 生成物 fresh/stamp（共享重建闸） |
| `exec/c/tbl.py` / `exec/c/net.py` | δ JSON→表→网络（下游；net 已有 `seed/net.c` 对拍先例） |

### 检查入口（具名）

| 入口 | 角色 |
|---|---|
| `exec/check.sh` | 顶层 E0（toy 表 + exec.c） |
| `exec/{enc,lower,opt,c}/check.sh` | 阶段/族检查 |
| `exec/**/*check.py` / `*check.sh` | 约 111 个具名检查（含 asm C 检查由 sh 拉起） |
| `exec/ledger.sh` / `exec/lex/ledger.sh` | 账本 |

### `seed-gen` 相对各 STAGE 头（只读推断，未跑对拍）

| STAGE | 路径 | 备注 |
|---|---|---|
| prune / enc / lower / parse2 | 特殊分支 | 已有实现入口 |
| opt | 泛化候选 | `#! base parsebase` + `#! flags o2`；无 extra/domains |
| pp | 泛化候选 | `#! base graph.py` + `#! graph G` + flags |
| nativeabi | 泛化受限 | 有 `#! start NC.START`（泛化可读 start）；须实跑核 |
| lex | 未覆盖 | `#! graph Delta` / domains / extra → generic `other` |

**本盘点不冒称**任一 STAGE 已与 Python 逐字节等价；对拍须另授实跑。

## 一条可批切口（草案，待政委/机房主任批）

### 名称：K5-1 `opt` δ 构造切到 `seed/gen.c`（保留 Python 参考）

**不做整锅**：只动 `opt` 这一 STAGE 的 δ JSON 构造路径；不删 `.py`；不改 `exec/opt/check.sh` 验收措辞/预算；不改其它 STAGE；不 bump；不冻结。

| 项 | 内容 |
|---|---|
| 动机 | opt 头最简且落在 `seed-gen` 已声明的泛化路径；体量小（单 flag `--o2`）；有 prune/enc/lower 同类先例 |
| 实现轮廓（批后） | (1) 用宿主 cc 编 `seed/gen.c`→`seed-gen`；(2) 同输入 `seed-gen opt OUT.json` 与 `python3 exec/build/gen.py opt OUT.json`（及带 `--o2`）`cmp`；(3) 故意改坏一行 TSV → 两侧均按名拒绝；(4) 在 stamp/fresh 或具名调用点为 **opt 仅** 增加「优先 seed-gen、失败不静默回退冒充」的接线；(5) `opt/check.sh` 仍可先走 Python gen，第二刀再切检查侧 |
| 验收 | 同输入逐字节相等（opt 与 opt `--o2`）；负例红；确定性双跑；identity 含构造器二进制/源、manifest/TSV 闭包、flags、宿主 cc；墙钟/RSS 分列不承诺「迁 C 更快」 |
| 明确不做 | 不删 `exec/build/gen.py` / assemble；不切 pp/lex/enc 全量；不改产品验收；不开旧全量 queue；不把矩阵历史绿当本刀证据 |
| 门闩依赖 | 本刀**不依赖**分层并发、WF2 填尾、K2b 真白名单、真实冻结首观察；依赖现有 bound/夹具与同输入对拍能力。缺项标「本切口不依赖」 |
| 归属建议 | cdx 实现生成器接线；cc 核 `opt/check.sh` 仍绿且不扩验收 |

### 备选（不本刀，仅登记）

- K5-1b：`pp` 同模式（flags 更多，次选）。
- K5-check-1：薄检查壳（如 `enc/shacheck.sh`+`.py`）改 C——属检查族，与生成器刀分开批。

## 禁

未批就全量开干；整目录删 `.py`；旧全量空转；空等不交盘点。
