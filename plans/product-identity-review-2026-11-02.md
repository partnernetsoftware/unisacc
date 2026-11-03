# 产品身份实现第 0-4 步评审（只读）

范围：提交 `4588098fb8..56473f9eab`（含 `f83b66ee04`、`7122928d7d`、`56473f9eab`），依据 `plans/product-identity-2026-11-02.md`。
方法：只读源码/差异；未改文件。结论按“必须修 / 建议修 / 可接受”三档，每条附文件:行与复现思路。

## 0. 结论

- **必须修 9 条**：`pp_digest_or_zero` 吞错可造成 false PASS；`streams.cx`/`properties.cx` 忽略候选参数；`hostseed-controls` 成员集 vs 整文件比较；`classic18-product-nets` 同类比较；成员集截断/顺序/canon 与计划不一致；`pp_layout` 把 EACCES 当缺失；v2 无 `candidate_kind` 被静默当 legacy；`run-batch` after-hash 在 reap 前且超出 `seconds`。
- **建议修 6 条**：符号链接、权限位、大文件边界、`pp_identity_kind=none`、材料收据版本、哈希性能。
- **可接受 4 条**：APE 退化、v1 all-aggregate 标 legacy-no-kind、v1 不能继续在 split 下跑、`pp_copy_member` 保留 mode。

## 1. 必须修

### 1.1 `pp_digest_or_zero` 把身份失败变成空串，可能 false PASS

**文件**：`tools/cxlib/productpkg.cx:262`
**调用点**：`tests/formats.cx:14/17`、`tests/properties.cx:54/66`、`tests/stream-*.cx`、`tests/ri-*-controls.cx` 等大量 `pp_digest_or_zero(root,before/after)`。

**问题**：`pp_product_digest` 失败（成员缺失/EACCES/布局坏）时，`pp_digest_or_zero` 把 `hex[0]=0`。很多测试只做 `tx_eqz(before, after)`；两侧都失败时两个空串相等，`candidate-unchanged` 会**误报 PASS**。这与计划“每个成员失败要具名拒绝，不能跳过”直接冲突。

**复现**：让 `ape-receipt.txt` 指向一个不存在的成员（或让 receipt 本身 EACCES），跑一个只做 before/after 的测试（如 `tests/formats.cx`）；观察 `CANDIDATE sha256=` 为空而非 REFUSE，`candidate-unchanged` 仍 PASS。

**建议**：guard 点改用 `pp_product_digest`，失败 `die("product identity: <member>")`；或让 `pp_digest_or_zero` 返回一个不可能相等的 sentinel 并打印具名原因，禁止空串参与 `tx_eqz`。

### 1.2 `tests/streams.cx:102` 忽略 `CANDIDATE_APE`

**文件**：`tests/streams.cx:102`、`:106`
**问题**：`argc==5` 时 `product=realpath(argv[4])` 是外部候选；替换后 `pp_digest_or_zero(root, product_identity)` 哈希的是 **root 的安装产品**，不是 `product`。后续 `identity.product.unchanged` 也哈希 root。候选参数被静默忽略。
**复现**：`tests/streams.cx ROOT STREAMS OUT /path/to/other-candidate`，打印的 `CANDIDATE sha256` 仍是 root 的身份；即使候选不同，测试也可能通过。
**建议**：对 APE 候选用 `pp_product_digest` 需要其所在 root；若 `argv[4]` 是单文件 APE，至少应 `sha256(product)` 并在打印/比较中显式区分 `candidate_kind`。不能直接传 `root`。

### 1.3 `tests/properties.cx:54/66` 同样忽略候选

**文件**：`tests/properties.cx:52`（`argv[3]` 可为候选）、`:54`、`:66`
**问题**：与 1.2 同类；`before/after` 都哈希 root，候选 `product` 只用于后续执行，身份与执行对象不一致。
**复现**：传一个与 root 不同的 `argv[3]`；`CANDIDATE sha256` 不随候选变化。
**建议**：同 1.2；候选身份与 root 身份分开打印/比较。

### 1.4 `tests/hostseed-controls.cx:36` 成员集 vs 整文件

**文件**：`tests/hostseed-controls.cx:36`
**问题**：`pp_digest_or_zero(root,ps)` 在 split 下返回**成员集**哈希；`digest(models,ms)` 是对 `models` 整文件的 sha256。`if(!tx_eqz(ps,ms))die("baseline MODELS is not current full product")` 在 split 下必然失败（除非 models 也按成员集解析）。
**复现**：split 布局 root，`models` 为完整 APE 副本；跑 `hostseed-controls`，在 baseline 处 die。
**建议**：对 `models` 也走 `pp_product_digest`（若它是 APE，退化整文件；若是 split，成员集），或把 `models` 的期望改成显式的 `candidate_kind` + 对应哈希。

### 1.5 `tools/classic18-product-nets.cx:36/49` 成员集 vs 候选文件哈希

**文件**：`tools/classic18-product-nets.cx:36`（`neg_root`）、`:49`（gate）
**问题**：`hroot` 是成员集；`hcand=digest(cand)` 是候选**文件**（`realpath(product)`）的整文件哈希。split 下 `tx_eqz(hroot,hcand)` 永远不等，`neg_root` 会 `die("neg-expected-equal-sha")`；gate 也永远不会把“候选等于 root”判出来。
**复现**：split root，`candidate=root/unisacc.com`；`--gate` 或 `neg_root` 失败。
**建议**：比较同一身份；候选是单文件 APE 时用其整文件哈希，root 是 split 时用成员集，二者只在 `candidate_kind` 相同且语义可比时比较；或明确拒绝“split root + APE candidate”的负例。

### 1.6 成员集截断/顺序/canon 与计划不一致

**文件**：`tools/cxlib/productpkg.cx:118`（`PP_MAX_MEMBERS 16`）、`:207`（`n < max` 静默停止）、`:212`（`pp_put` 截断到 63 字节）、`:216`（receipt 存在但 0 条有效行时回退两名字）、`:226`（只从 i=1 排序，launcher 固定第一）、`:159`（canon 多出 `bytes` 列）。

**问题**：
- 成员超过 16 个时静默截断，digest 只覆盖前 16 个；缺失成员不被发现。
- basename ≥64 字节被截断，可能两个成员合并成同一名字；`pp_receipt_line` 对 ≥256 字节的名字返回 0（跳过）。
- receipt 存在但解析 0 条有效行时，`n==1` 回退 `p3-package.bin`+`entry.elf`，可能静默漏掉 receipt 里其他成员。
- 计划写“按 basename 字典序排序”，实现把 launcher 固定第一、其余排序；计划 canon 是 `<basename>\t<sha256>\n`，实现是 `<basename>\t<sha256>\t<bytes>\n`。两处都与计划文本不一致，跨实现会得到不同身份。
**复现**：在 `ape-receipt.txt` 加第 17 个成员；或把两个成员 basename 做成同 63 字节前缀；对比 `pp_product_digest` 与计划手工 canon。
**建议**：超 max/超长/解析失败一律具名拒绝；明确“launcher 是否参与排序”；更新计划 canon 或去掉 bytes；版本化身份（见 2.5）。

### 1.7 `pp_layout` 把 EACCES/ENOTDIR 当缺失

**文件**：`tools/cxlib/productpkg.cx:80-85`
**问题**：`if (sy_exists(p) == 1) return PP_SPLIT;` 只认 1；`sy_exists` 对 EACCES/ENOTDIR 返回负 errno，代码继续走 `pp_layout_of_file(root/unisacc.com)`，可能误判 APE 或返回 `-ENOENT`。项目规则是“只有 ENOENT 当不存在，ENOTDIR/EACCES 必须报告”。
**复现**：`chmod 000 unisacc.com.dev` 或 `p3-package.bin`；`pp_identity_kind`/`pp_product_digest` 不报 EACCES，可能给出 ape1 或 none。
**建议**：`int e=sy_exists(p); if(e==1) return PP_SPLIT; if(e!=0) return e;`，并把负 errno 具名上报。

### 1.8 v2 无 `candidate_kind` 被静默当 legacy；header 越界

**文件**：`tools/cxlib/allsession.cx:217-230`
**问题**：
- `line==0` 接受 `v1|v2`；`END` 分支只按“有没有 candidate_kind”判断 legacy，不检查版本。一个 **v2** manifest 删掉 `candidate_kind` 会被静默标成 `legacy-no-kind`（ape1），而不是 REFUSE。
- `keys[line-1]` 在 `line>10` 时越界；多余 header 行会读 `keys[10]` 之外，可能崩溃而非具名拒绝。
- v1 里出现 `candidate_kind` 也被接受为 v2；版本与字段不一致未拒绝。
**复现**：拷一个 v2 `session.tsv`，删除 `candidate_kind` 行；`all-aggregate` 打 `legacy-no-kind` 而不是拒绝。再加一行 `junk\tx` 到 header，观察越界/崩溃。
**建议**：记录 `version`；v2 必须恰好有 candidate_kind，v1 必须没有；`hdr` 分支对 `line>10` 先拒绝再取 `keys`。

### 1.9 `run-batch` after-hash 在 reap 前且超出 `seconds`

**文件**：`tests/run-batch.cx:54-61`
**问题**：`np_run(..., seconds)` 返回后立刻 `product_digest(root,after)`，**然后**才在 `:57` `waitpid` 回收超时留下的孤儿。after-hash 可能和仍在运行的子进程/孤儿并发；且 after-hash 不计入 `seconds`，在外层 `tools/bound 55` 与 batch `budget_seconds=55` 时，after-hash/回收/打印可能被外层 watchdog 杀掉，收据丢失。
`timed_out` 在 `:61` 用的 `result.residue` 是 reap 前的值；孤儿在 after-hash 后才退出时，status 可能被误判 FAIL 而不是 UNKNOWN。
**复现**：选一个会超时并留下短命孤儿的批次；在 `products_digest` 期间观察孤儿仍在；或把 batch budget 设为 55、外层 `tools/bound 55`，看收据是否偶发缺失。
**建议**：先 `waitpid` 回收，再 after-hash；给 after-hash 预留时间（或把 `np_run` 的 `seconds` 减 1-2s）；`timed_out` 的 residue 在 reaping 后重新判定。

## 2. 建议修

### 2.1 符号链接成员

**文件**：`tools/cxlib/productpkg.cx:144`（`pp_file_hex`）
`sy_exists` 用 lstat（链接本身存在），`cx_read` 跟随目标；split 成员若被替换成 symlink，digest 会哈希外部目标，不拒绝链接。建议 `lstat` + `O_NOFOLLOW`，或把链接目标文本纳入 canon。

### 2.2 权限位

**文件**：`tools/cxlib/productpkg.cx:159`（canon）
canon 只有 name/sha/bytes，没有 mode；`entry.elf` 的 `chmod -x` 不改变 digest。`pp_copy_member` 保留 mode，但身份不覆盖。建议加 mode 并升版本；否则明确“权限位不在身份合同内”。

### 2.3 大文件边界

**文件**：`tools/cxlib/productpkg.cx:144`
`cx_read` 整文件读入，无上限；4GiB 成员会 OOM。建议 bound（如 64MiB）或流式 sha；超限具名拒绝。

### 2.4 `pp_identity_kind` 返回 `"none"`

**文件**：`tools/cxlib/productpkg.cx:121`、`tools/cxlib/allsession.cx:158`
`pp_identity_kind` 对坏布局返回 `"none"`；`as_identity` 直接 `as_copy` 进 `candidate_kind`，写出的 v2 清单带 `candidate_kind=none`，下次 `as_load` 的 header 只接受 ape1/split1，会拒绝。建议 `as_identity` 对 `none` 直接失败，或 `pp_identity_kind` 返回 NULL 让调用方拒绝。

### 2.5 材料/工具收据语义变更未升版本

**文件**：`tools/object-material.cx:20`（`objects-material-v1` 的 `product` 从整文件变成员集）、`tools/package-refresh.cx:47/62`（`baseline` 变成员集）。
旧收据/旧消费者若按整文件 sha 解析会错配。建议加 `product_kind=ape1|split1` 并升 `objects-material-v2` / package-refresh 版本；旧收据保持原样。

### 2.6 哈希性能

**文件**：`tools/cxlib/productpkg.cx:144` vs `tests/run-batch.cx:31` 的 `digest`
`run-batch` 的 `digest` 对 ≥64KiB 用 `sy_sha256_hex`（AF_ALG/OS），`pp_file_hex` 一律 `cx_sha256_hex`（软件）。split 三成员约 3.6MB，每批 before/after ≈7.2MB；重批/21 个测试×2 可能形成热点。建议 `pp_file_hex` 对 ≥64KiB 也走 `sy_sha256_hex`，或在 wave 内缓存产品身份；用 `TIMING` 实测后再决定。

## 3. 可接受

1. **APE 退化**：`pp_product_digest` 单文件返回 sha256(whole file)，与旧证据字节不变；符合计划。
2. **v1 all-aggregate**：`as_aggregate` 不调用 `as_ident_diff(live)`；v1 载入后 `legacy_no_kind=1`，聚合行打 `identity_scope=legacy-no-kind`；旧收据可聚合。
3. **v1 不能在 split 下继续**：`as_ident_diff` 对 v1(ape1) vs live(split1) 返回 `identity-kind`，all-waves start 拒绝；这是合理的“不同身份不可比”，不是回归。
4. **`pp_copy_member` 保留 mode**：复制产品时保留源 mode，正确。

## 4. 复现清单（按优先级）

| # | 最小动作 | 预期/观察 |
|---|---|---|
| 1 | 让 `ape-receipt.txt` 指向缺失成员，跑 `tests/formats.cx` | `pp_digest_or_zero` 空串；`candidate-unchanged` 误 PASS |
| 2 | `tests/streams.cx ROOT STREAMS OUT OTHER_CANDIDATE` | 打印的 CANDIDATE 不随 OTHER 变化 |
| 3 | `tests/properties.cx ROOT ... OTHER_CANDIDATE` | 同上 |
| 4 | split root + `models` APE，跑 `hostseed-controls` | baseline `ps != ms` die |
| 5 | split root + `--gate`/`neg_root` in `classic18-product-nets` | `hroot != hcand`，负例 die / gate 漏判 |
| 6 | receipt 加第 17 个成员 / 64 字节 basename | digest 静默少成员/合并名字 |
| 7 | `chmod 000 unisacc.com.dev` | `pp_layout` 不报 EACCES，误判 ape1/none |
| 8 | v2 session 删 `candidate_kind`；header 加一行 | 静默 legacy / 可能越界 |
| 9 | 超时批次留孤儿；`tools/bound 55` 跑 budget 55 | after-hash 在 reap 前；收据可能被外层杀 |
| 10 | `chmod -x entry.elf`；或 symlink entry.elf | digest 不变/跟随链接（建议修） |
| 11 | 4GiB 稀疏成员 | `cx_read` OOM（建议修） |
| 12 | 重跑 21 个测试，计 `pp_product_digest` 时间 | 估算 7.2MB/批 的软件 SHA 热点 |

## 5. 未核实

- `package-refresh` 的 `baseline_sha` 常量是否已按 split 成员集更新；若仍是旧 APE 整文件哈希，split baseline 会 refused。
- `classic18-product-nets` 的 `bind_evidence(hroot)` 是否接受成员集字段；旧 evidence 可能按整文件。
- `pp_receipt_line` 的 `NOT_` 跳过是否符合所有 receipt 变体；本文只按当前实现判断。
- 未实跑上述复现；所有行号按当前 `HEAD`（`56473f9eab`）。
