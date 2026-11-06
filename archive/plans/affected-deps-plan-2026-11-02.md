# `tools/affected.cx` 依赖映射改进计划（事实表/文档 → 生成器/批次）

日期：2026-11-02，omp-ds。来源：cc 采纳 `plans/w5-affected-batches-review-2026-11-02.md` 的复核，
列为冻结后债务。本文件只写计划，不改代码。

## 一、现状（已实测，不是推测）

`tools/affected.cx`（528 行）识别六种 via：`source`、`use`、`literal`、`literal-use`、`arg`、`arg-use`，
匹配对象是 `tests/batches.tsv` 里 `mode=SOURCE`、`IMPLEMENTED` 的行，根取 `source`/`entry`/`args` 的路径 token。

实测缺口（W5 三补丁，6 个文件）：

```
sh ./unisacc.com tools/affected.cx --files cx/cx0/bundle.cx exec/lower/codeend-manifest.tsv \
  exec/parse2/units-result.tsv exec/pp/include-location-result.tsv exec/pp/macro-byte.tsv
→ AFFECTED cx0-bundle / cx0-e2 / cx-layers；AFFECTED_SUMMARY changed=5 batches=3 scanned=355
```

四个事实表零映射。原因是结构性的：这些表的消费者是 `tools/*-gen.cx` 生成器与产品编译链，不是某个
`.cx` 批次的 `source`；`literal` 只扫**字符串字面量**且要求路径存在，而事实表路径多以 `cx_fmt("exec/facts/%s.tsv", stem)`
拼接；文档引用则写在注释里（`tests/formats-elf-ape.cx:15` 之类），连字面量都不是。

affected.cx 自己的文档头已经写明 `Not modelled: generated inputs, files read through a computed path`。
本计划就是补这两条，并用登记表兜住"计算路径"这类无法静态发现的情况。

## 二、目标

1. 改一个事实表 → 输出该表的读者（生成器）→ 生成器的产物 → 依赖产物的批次，给到批次级结论。
2. 改一份被注释点名的文档/手册 → 输出 `via=comment` 的**弱依赖**，默认不判红、只提示（注释不是合同）。
3. 任何 `exec/**/*.tsv`、`exec/**/*.md` 既不在生成物清单里、也没有登记行时，`--strict` 下 rc=1，
   不允许"静默零映射"。

## 三、登记表：`tests/affected-deps.tsv`

六列，TSV，`#` 开头为注释，空行跳过：

```
path	reader	role	product	target	note
exec/pp/macro-byte.tsv	pp-negative	batch	-	-	补丁 03 直接消费
exec/pp/include-location-result.tsv	pp-negative	batch	-	-	同上
exec/lower/codeend-manifest.tsv	stream-lower-independent	batch	-	-	codeend 语义
exec/lower/codeend-manifest.tsv	stream-lower-domain-format	batch	-	-	
exec/parse2/units-result.tsv	cx-unit-graph	batch	-	-	64→128 上限
exec/parse2/units-result.tsv	objects-old-unit-version	batch	-	-	单元版本边界
exec/facts/pp-autoinc.tsv	facts-gen	generator	exec/facts/pp-autoinc-gen.tsv	-	已核实：facts-gen.cx 读 pp-autoinc.tsv 写 pp-autoinc-gen.tsv
exec/facts/pp-autoinc-gen.tsv	facts-gen	generator	exec/facts/pp-autoinc-gen.tsv	-	产物本身也被产品读（名字表），登记以覆盖直接改产物的情况
exec/pipeline/formats.md	stream-lower-domain-format	comment	-	-	注释点名，弱依赖
exec/c/PACKAGE.md	formats-elf-ape	comment	-	-	注释点名，弱依赖
```

- `role=batch`：读者本身就是批次 id（必须存在于 `tests/batches.tsv`）。
- `role=generator`：读者是 `tools/*.cx` 可执行体；`product` 是它的产物（相对仓根）。
- `role=comment`：弱依赖，affected 与 `--strict` 都只打印 `DOC`，不计入必跑。
- `target` 留给"产物再喂给谁"不直接写批次的情形；能直接写 batch 就不写 target，避免二级图。

`product` 非空时，affected.cx 把产物当作一次新的 changed 文件递归展开（深度上限 2，防环），
命中的批次 via 记为 `deps:<generator>`。

## 四、affected.cx 的改动点（冻结后实施）

1. 启动时读 `tests/affected-deps.tsv`（失败则 die，与 batches.tsv 同等级，不静默跳过）。
2. 匹配阶段在现有六种 via 之后新增：
   - `deps`——changed 文件等于 `path` 且 `role=batch`；
   - `deps-gen`——`role=generator`，产物入队递归；
   - `comment`——changed 文件是 `role=comment` 或由第三阶段的注释扫描发现。
3. 去重：同一批次多条命中保留最强 via（优先级 `source > use > deps > deps-gen > arg > literal > comment`），
   与现有"first changed file on a tie"规则合并成确定性排序，保证输出可 diff。
4. 摘要行加两列：`AFFECTED_SUMMARY changed=N batches=M scanned=K deps=D docs=E strict=0|1`。
5. `--strict`：`exec/**/*.tsv` 与 `exec/**/*.md` 中既非生成物（由 `tools/*-gen.cx` 产出）也无登记行 →
   打印 `UNDECLARED<TAB>path`，rc=1。生成物清单先读 `exec/facts/*.tsv` 里生成器写的那批（facts-gen 头注释列了 30 张），
   后续由 `tools/facts-gen.cx --check` 的 stdout 提供权威清单，避免手抄。
6. 注释扫描（`role=comment` 的自动候选，不是权威）：对批次根 `.cx` 的 `/* */` 与 `//` 文本做
   "repo 相对路径且存在"的匹配，去掉 `./` 前缀，与 `literal` 共用同一套路径规范化；产出仅用于
   `tools/check-deps.cx` 提示"未登记但被注释点名"。

## 五、校验工具：`tools/check-deps.cx`

登记表会腐烂，必须有机器检查，登记为批次 `deps-map`：

1. 每行 `path` 存在（ENOENT 才判缺；ENOTDIR/EACCES 报错退出）。
2. `role=batch` 的 `reader` 在 `tests/batches.tsv`；`role=generator` 的在 `tools/` 且可编译体存在。
3. `product` 若填写，必须存在或能被 `--check` 判为生成物。
4. 重复行（同 path+reader）报 `DUPLICATE`。
5. 反向覆盖：W5 三补丁的 6 个文件 + 两份文档跑一次 affected，断言输出含 27 行批次
   （stream-lower 11、cx0 系 3、unit/objects 系 12、pp-negative 1）与 2 行 `DOC`。
6. `exec/**/*.tsv` 全量扫一遍，未声明且非生成物 → `UNDECLARED`（与 `--strict` 同口径）。

## 六、验收（冻结后第一轮就要能复现）

| # | 命令 | 期望 |
|---|---|---|
| 1 | 现状 before | `batches=3`（已实测，作为回归基线） |
| 2 | after | 同一命令 `batches=27`，其中 `via=deps` 24、`via=deps-gen` 0（本组无生成器型）、`DOC 2` |
| 3 | `tools/check-deps.cx -- /home/devali/repos/unisacc` | `CHECK_DEPS failed=0`，`UNDECLARED` 为空 |
| 4 | 改一个未登记的事实表副本（`exec/opt/answer-targets.tsv` 不动，用 `--files` 指名字符串） | `--strict` rc=1 且打印 `UNDECLARED`；非 strict rc=0 |

## 七、分阶段与风险

- **P1（小）**：表 + `role=batch` 读取 + 摘要扩展。验收 2 里的 24 行里 24 行都属此类，先拿满。
- **P2（中）**：`role=generator` + 产物递归 + `--strict`。
- **P3（小，可选）**：注释候选提示 + `check-deps.cx` 的 `UNDECLARED` 报告。
- 风险 1：登记表腐烂 → 靠 `deps-map` 批次 + `--strict`，不靠人记。
- 风险 2：把 `comment` 升成必跑会造成大面积噪声且无合同价值（注释不参与构建）→ 固定为弱依赖，只有
  `DOC` 行，不进 `batches=M` 计数。
- 风险 3：生成器产物身份随纪元变（`out/ctx/*`），`product` 只写相对仓根的稳定产物，不写 `out/` 下的临时路径。
- 风险 4：`--strict` 若一开始就用会挡正常开发（历史表可能大量未声明）→ 先 `--strict` 仅用于
  `exec/**`，跑通两轮后再进默认批次。

## 八、与既有规则的关系

- 不替代 `tests/run.cx -- --all`：affected 只回答"改这几行最小要跑哪些"，发布资格仍是完整新清单。
- 不缓存结论：表只是"谁读谁"的静态声明，批次结果仍实跑。
- 新工具按规则放 `tools/`、带 `check-deps.cx`、登记 `tests/batches.tsv`（`deps-map`），
  并在 `prd.md` 能力速查补一行 `tools/affected.cx` 的新能力说明。
