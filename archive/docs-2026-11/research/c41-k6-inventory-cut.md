# K6-0 盘点：unisa/ 参考与权重构造 → C（材料；不预填 PASS）

**状态（2026-10-11 ≈18:35 SGT，政委并授 K6-0）**：盘点开工并落本页。**不**实现 C 参考；**不**删 Python；**不** bump；**不** Draft/publish/扩签/动 Latest。tip 基线 **510159e4**；`version.h`=0.0.40；Latest=v0.0.40。roadmap：0.0.41 = K6。

## 并授钉（照录 archive/plans/v0.0.41.md）

| 钉 | 值 |
|---|---|
| 文件域 | 只读 `unisa/**/*.py`；写仅本页（计划另提交） |
| 预算 | 材料 ≤30 min |
| 首红停 | 解析失败 / 48 不符无法归因 / 越域 → 停 |
| 产出 | 具名 py 表 + 依赖边 + 1 条 K6-1 草案 |
| 禁 | 写 C、删 py、bump、Draft、publish、扩签、Latest、旧全量 |

## 盘点（git 跟踪，unisa/；本机复算）

| 类 | 数量 |
|---|---:|
| `.py` | **48**（与 10-08 基线一致） |
| `.sh` | 0 |
| 包根 / 子包 | `unisa/` + `control/` + `front/` + `image/` |

### 全表（行数；只读 `wc`）

| 行 | 路径 | 角色粗标 |
|---:|---|---|
| 2 | `unisa/__init__.py` | 包 |
| 835 | `unisa/__main__.py` | CLI 入口（train/acc/tape/run/compile/…/build-weights/gold-export/emit-kernel…） |
| 145 | `unisa/ape.py` | APE 封装 |
| 79 | `unisa/assemble.py` | 汇编 |
| 12 | `unisa/bits.py` | 位宽/对齐工具（叶子） |
| 354 | `unisa/catalog.py` | 阶段目录/表（叶子，体量大） |
| 537 | `unisa/ckernel.py` | 整数推理内核源 |
| 576 | `unisa/construct.py` | 权重构造 |
| 8 | `unisa/control/__init__.py` | 包 |
| 275 | `unisa/control/net.py` | 对照训练网 |
| 42 | `unisa/control/rng.py` | RNG（叶子） |
| 91 | `unisa/control/segment.py` | 分段 |
| 106 | `unisa/control/train.py` | train |
| 178 | `unisa/control/uns1.py` | uns1（叶子偏） |
| 153 | `unisa/docgen.py` | 文档表 |
| 161 | `unisa/driver.py` | 前端驱动 |
| 552 | `unisa/emit_arm.py` | ARM 发射 |
| 731 | `unisa/emit_x86.py` | x86 发射 |
| 331 | `unisa/exec_target.py` | 目标执行 |
| 169 | `unisa/fp.py` | 软浮点（→bits） |
| 0 | `unisa/front/__init__.py` | 包 |
| 345 | `unisa/front/lex.py` | 词法 |
| 2895 | `unisa/front/parse.py` | 语法（最大） |
| 906 | `unisa/front/pp.py` | 预处理（叶子偏，体量大） |
| 341 | `unisa/front/sema.py` | 语义 |
| 635 | `unisa/gold.py` | gold 表/阶段 |
| 15 | `unisa/hostabi.py` | 宿主 ABI 片段（叶子） |
| 79 | `unisa/image/__init__.py` | 镜像聚合 |
| 77 | `unisa/image/elf.py` | ELF |
| 32 | `unisa/image/fat.py` | fat |
| 198 | `unisa/image/macho.py` | Mach-O |
| 261 | `unisa/image/pe.py` | PE |
| 175 | `unisa/intnet.py` | 整数网 |
| 567 | `unisa/ir.py` | IR |
| 179 | `unisa/libneed.py` | 库需求（叶子） |
| 112 | `unisa/linalg.py` | 线性代数（叶子） |
| 452 | `unisa/lower.py` | lower |
| 715 | `unisa/opt.py` | opt |
| 102 | `unisa/oracle.py` | oracle |
| 123 | `unisa/padded.py` | Padded/ZData（叶子） |
| 117 | `unisa/peversion.py` | PE version（叶子） |
| 193 | `unisa/prune.py` | prune |
| 364 | `unisa/tape.py` | tape |
| 390 | `unisa/tapebin.py` | tapebin |
| 62 | `unisa/tapebin_shape.py` | tapebin shape（→tape） |
| 105 | `unisa/tsvgold.py` | TSV gold |
| 190 | `unisa/uns2.py` | uns2 |
| 351 | `unisa/vm.py` | VM |

**合计行数**（含空 `__init__`）：约 **15.6k** 行 py（粗加；非验收）。

### 依赖边（unisa 内部；ast 只读推断，未跑对拍）

出边摘要（`A → B` = A import B；省略 `__init__` 空壳）：

| 模块 | → |
|---|---|
| `__main__` | ape, assemble, control, control.train, driver, exec_target, gold, image, linalg, lower, oracle, tape, tapebin, vm |
| ape | image, peversion |
| assemble | tape |
| ckernel | catalog, front.lex, gold, libneed |
| construct | gold, linalg |
| control.net | control.rng, linalg |
| control.segment | gold |
| control.train | control.net, control.rng, gold |
| docgen | __main__, gold |
| driver | front, front.lex, front.parse, front.pp |
| emit_arm / emit_x86 | bits, catalog, hostabi, image.pe |
| exec_target | fp, padded, tape, vm |
| fp | bits |
| front.lex | fp, gold |
| front.parse | fp, front.lex, front.sema, gold, ir |
| front.sema | gold |
| gold | tape |
| image | bits, padded |
| image.{elf,fat,macho,pe} | bits |
| intnet | construct, gold |
| ir | fp, tape |
| lower | padded, prune, tape |
| opt | __main__, gold |
| oracle | control.train, gold, linalg |
| prune | tape |
| tape | fp, padded |
| tapebin / tapebin_shape | tape |
| tsvgold | gold |
| uns2 | intnet |
| vm | bits, fp, padded, tape |

**叶子（对其他 unisa 模块出边=0）**：`bits`, `catalog`, `control.rng`, `control.uns1`, `front.pp`, `hostabi`, `libneed`, `linalg`, `padded`, `peversion`（另 `__init__` 空壳）。

**本盘点不冒称**任一模块已与 C 逐字节等价；对拍须另授实跑。

### 已有测试钩（只读点名，非本刀跑）

| 钩 | 触达 |
|---|---|
| `tests/apeversioncheck.py` | `unisa.peversion` |
| `tests/layout_check.py` / `ape_stage.py` | `padded`, tape, lower, assemble, image |
| `tests/tsvbuild_check.py` | intnet, tsvgold, gold |
| `tests/vm.sh` | `python3 -m unisa vm` |
| `tests/kernel.sh` / `headerchain.sh` | `emit-kernel` |
| `tests/acceptance.sh` 等 | gold / compile / tape 宽面 |
| `kernel/` 生成物 | 由 `emit-kernel` / build-weights 写出（禁手改） |

### C 先例（仓内，非 unisa 逐模块移植）

| 路径 | 备注 |
|---|---|
| `seed/gen.c` / `seed/net.c` / `seed/ape.c`… | K5 生成器 C 化先例；**不是** unisa 模块对拍 |
| `kernel/unisa_*.c/.inc` | 由 Python 发射的产品内核片段 |

---

## 一条已批切口（K6-1：YES，政委 2026-10-11 ≈18:36）

### 名称：K6-1 `unisa.bits` → C 参考对拍（保留 Python）

| 项 | 内容 |
|---|---|
| 动机 | 真叶子（出边 0）；仅 12 行；入边高（image/emit/vm/fp 等约 18）；API 面极窄（`MASK64`, `round_up`）；适合验证「同输入逐字节 / 负例同拒 / 身份闭包」管道在 unisa 域可复用 |
| 实现轮廓（批后） | (1) 宿主 cc 编最小 `unisa_bits` C 参考；(2) 同输入对拍 `round_up` / 掩码边界；(3) 故意越界/坏宽两侧同拒；(4) 确定性双跑；(5) **不**改 `__main__` 默认路径；Python 保留 |
| 验收轮廓 | 同输入结果相等；负例红；identity 含 C 源/二进制、输入闭包、宿主 cc；墙钟分列**不**承诺更快 |
| 明确不做 | 不删 `bits.py`；不切 gold/construct/emit-kernel；不改产品验收；不开旧全量；不 bump |
| 门闩依赖 | **不依赖** R9 顺延整表、分层并发、WF2、K2b 白名单；依赖优化管道纪律 + K5 已证明的对拍/材料→真审→写键→复算节奏。缺项标「本切口不依赖」 |
| 备选（不本刀） | `hostabi`（15 行叶子，字符串表）；`padded`（有 `layout_check`/`ape_stage` 钩，稍重）；`peversion`（已有 `apeversioncheck`）；`control.rng`（训练对照，非出货主路径） |
| 归属建议 | cdx 实现 C 参考与对拍；cc 核既有钩仍绿且不扩验收 |

**已批实现。** 落地回执：`research/c41-k6-1-bits.md`（gate `unisa-bits` same 7 / diff 0；不删 py；不 bump）。

### K6-1 进度（追加）

| 项 | 状态 |
|---|---|
| C 参考 | `seed/unisa_bits.c` |
| 夹具 | `tests/unisabitscheck.sh`；gate job `unisa-bits` |
| 读数 | rc0；SAME×7 / DIFF×0（MASK64、round_up×20、det×2、NEG-a0、NEG-badint） |
| 明确未做 | 不删 bits.py；不切下一叶子；不 bump；不 Draft/Latest/签 |
| 点采样边界（≈18:54） | same7=断言组点采样（20 手写）；非穷举；`seed/unisa_bits.c` 不在产品闭包；穷举/更大随机另排（见 archive/plans/v0.0.41.md + c41-k6-1-bits.md） |

---

## 禁

未批就整目录迁 / 删 py；旧全量空转；二次 publish / 扩签 / 动 Latest / 签 / bump。

## 首红记录

K6-0 盘点：ast 解析 48/48 成功；`.py` 计数=48；未越文件域。**无首红。**

K6-1 实现：编译绿；对拍 7/0；负例两侧同拒；未越 K6-1 文件域。**无首红。**

---

*K6-0 材料 2026-10-11 ≈18:35 SGT / 政委并授；K6-1 代裁授 ≈18:36 SGT*
