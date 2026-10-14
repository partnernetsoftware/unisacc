# 提交闸的派生一致性检查：自动追加方案（只读设计，不改门禁）

标签：**[读源码]** = file:line；**[实际跑过]** = 只读运行（耗时为单次实测）。
不改 `release/tools/commitgate.sh`、`tests/gatequeue.py`；登记文件未动。冻结期间只写稿。

## 1. 派生文件清单 [读源码]
| 派生文件（已提交） | 生成命令 | 核对套件（gate.sh） | 输入（源）路径集合 |
|---|---|---|---|
| `exec/facts/*.tsv`（30 张表，`exec/facts/export.py` 的 `TABLES`） | `python3 exec/facts/export.py` | `facts-export`（`export.py --check`，:591） | 123 个输入文件（`exec/build/*.py`、`exec/facts/*.tsv`、`exec/assemble.py` 等，由 `TABLES` 列出；`export.py:1–9`） |
| `tests/abi.manifest` | `abimanifestcheck.cx --write PATH`（`:2–5`） | `abi-manifest`（`--check`，:327） | `exec/c/*.c`、`exec/c/*.h`、`include/*.h`、`src/*`（由 `abimanifestcheck.cx` 的推导决定） |
| `kernel/*`（`unisa_core.c`、`unisa_self.c`、`unisa_model.inc`、头） | `python3 -m unisa emit-kernel --out DIR`（`tests/kernel.sh:3,14`） | `kernel`（`tests/kernel.sh`：“generated files up to date” 比较，:576） | `weights/built.*`（由 `weights/gold/*.tsv` 构造）、`unisa/gold.py`、`unisa/catalog.py`、`include/*.h`、`unisa/ckernel.py` |
| `weights/gold/*.tsv` | `unisa gold-export`（`unisa/gold.py` 构造） | 无“重新生成后比较”的套件；`unisa-gold-*`（K6-18…28，:650–660）只对拍 C 参考与 `gold.py` 源，不核对 tsv 的字节 | `unisa/gold.py`、`unisa/catalog.py` |
| `exec/tapecheck/shape.tsv`、`unisa/tapebin_shape` 派生 | `python3 -m unisa.tapebin_shape --write`（约定） | `tapebin-shape`（`--check`，:597） | `unisa/tape.py`（SHAPE） |
| `tests/INVENTORY.md` | `python3 tests/inventory.py --write`（`tests/inventory.py:56`） | `script-inventory`（`--check`，:599） | `tests/*.sh|*.py|*.cx`、`exec/**/*check*`、`tests/gate.sh`、`.github/workflows/*` |
| `tests/gatedeps.json`（stamps 与守护） | `python3 tests/refresh_gatedeps.py`（`make gatedeps`） | 无独立核对套件（`gatelayers`、`inventory` 间接依赖其声明） | HEAD 的 `git archive`（`refresh_gatedeps.py:2–12`）；声明在脚本内 |

注：`kernel` 与 `facts-export` 都是“重新生成到临时位置后比较”的形式，最适合被自动追加；`abi-manifest` 依赖根产品；`gold` 与 `gatedeps` 目前没有“重新生成后比较”的套件。

## 2. 提交闸的最小改动方案（只写设计）
1. **路径→套件映射表**（一个 TSV，放 `release/` 下，由门禁读取）：
   - `exec/facts/*`、`exec/build/*.py`、`exec/assemble.py`、`src/front_pp.c`、`unisa/tape.py` → `facts-export`（`src/front_pp.c` 与 `unisa/tape.py` 是今天两次变红的输入，已在 `:591` 的注释中）；
   - `exec/c/*.c|*.h`、`include/*.h`、`src/*` → `abi-manifest`；
   - `kernel/*`、`unisa/gold.py`、`unisa/catalog.py`、`weights/gold/*`、`include/*.h` → `kernel`；
   - `unisa/tape.py`、`unisa/tapebin_shape.py` → `tapebin-shape`；
   - `tests/` 中新增、删除或改名的 `*.sh|*.py|*.cx`、`tests/gate.sh`、`.github/workflows/*` → `script-inventory`（只在路径集合变化时追加，见 §4）；
   - `tests/gatedeps.json`、`tests/refresh_gatedeps.py` → 新增“gatedeps 重算一致”检查（重算到临时文件并比较，这是缺失的一环）。
2. **干净导出**：`git archive HEAD` 解到临时目录，再把本笔的暂存内容覆盖进去（只有本笔的路径）。派生检查在这个导出里运行，因此别人的在途文件不进入检查（今天 cdx3 生成清单时吸入在途文件正是这个问题）。
3. **根产品**：`abi-manifest` 运行根产品，干净导出里没有 `unisacc.com`。两种办法：要么把它列为“需要新鲜构建”的套件（没有新鲜构建则 fail closed，而不是通过）；要么在导出里用 `make com` 重新构建（耗时远超 60 秒，不可行）。推荐前者。
4. **映射表没有覆盖的派生路径**：若本笔改了一个派生文件（如 `exec/facts/x.tsv`）却没有任何映射行、或改了源却没改派生文件，门禁**拒绝**提交，并列出未覆盖的路径。这把“事后扫描才发现”改为“提交前拒绝”。

## 3. 与 fail-closed 的关系
- 今天 `commitgate` 的规则是：点名的套件必须有 gatedeps 声明，否则拒绝（“has no declared inputs … (fail closed)”，本仓库已多次遇到）。追加的派生检查若没有声明，同样会被拒绝。
- 建议：派生检查**不**走点名通道，而走映射表的固定集合，且每项必须在 gatedeps 中有声明（由 `refresh_gatedeps.py` 生成，见 §4 的落地顺序）。映射表里有、gatedeps 里没有的 → 门禁失败（fail closed）；映射表里没有、但本笔改了派生路径的 → 门禁失败。
- `abi-manifest`、`facts-export`、`gate-layers` 今天在 gatedeps 中分别是：已声明、未声明、未声明。先补声明，再接入映射表。

## 4. 耗时与拆分（风险）
单次实测（只读）：`facts-export` 0.47 秒（7000 队列日志）；`kernel` 0.66 秒（同）；`tapebin-shape` 0.16 秒（同）；`gate-layers --check` 0.42 秒（实测）；`abi-manifest --check` 约 2.0 秒（实测，但它在当前工作区报 rc1，见下）；`script-inventory` 36.6 秒（同 7000 日志），这是唯一的大项。

- 默认追加的开销约 4 秒（facts + kernel + tapebin-shape + abi），在 60 秒窗口内。
- `script-inventory` 只有在 tests/ 中新增、删除、改名脚本，或改 `gate.sh` 时才追加（否则不跑）；这样普通提交不增加 36 秒。
- 若两者合计超过窗口，把派生检查拆成独立的 `commitgate` 第二次调用（各自 ≤58 秒），第一次的通过结果作为输入记录，而不是合并到一个进程。

## 5. 当前状况（实测，只读）
`sh unisacc.com -run tests/abimanifestcheck.cx -- --check`（根产品）返回 rc1，输出提示“commit the manifest with the change”，即 `tests/abi.manifest` 与当前工作区源不一致。该不一致来自工作区中他人的未提交改动（`exec/c/libunisacc.c` 等），不是 HEAD 的状态；这正说明派生检查应该在干净导出中运行。

## 6. 建议的落地顺序（待主人裁定）
1. 先补声明：`facts-export`、`gate-layers` 进入 gatedeps（经 `refresh_gatedeps.py`，需登记窗）。
2. 写映射表与干净导出（在 `release/` 下，不改 `commitgate.sh` 之前先作为只读工具验证）。
3. 验证：对历史上两次红的提交（`unisa/tape.py` 与 `src/front_pp.c` 那两笔）回放，映射表应报出缺失的派生文件。
4. 最后才接入 `commitgate.sh`（需它的 owner 批准）。
