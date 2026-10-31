# 词法上下文 preflight 测量提案（2026-10-31）

状态：**提案，不授权开测/建工具/改 cxmod 或 entry-build/spec**。本文件是可审合同草案，不是产品预算、不是身份一致性实现、不是超限拒绝负例。W-1/CX-U-4/W-6/W-7 不变。

## 1. 目的与不覆盖

测同一 commit 下，从显式根集合展开的**词法上下文**规模：contexts、edges、path bytes、单向 pair 遍历的 visited/peak；为未来身份一致性预算提供观察值。

dependency_diff>0 或 public_diff>0 均表示未证明一致，不能称 identity 通过。
明确不覆盖：
- 不证明现有仓低于任何产品上限；测量上限不是产品预算。
- 不实施合并前拒绝，不替代超限负例。
- 不做所有排列；pair-policy v1/v1r 只覆盖两种顺序。
- 不快照原子性；结束复核只是变化检测。
- 不覆盖 macOS/Windows/ARM，不安装候选，不转授当前工作区。
- 不把 `-cx-plan` 的 realpath 去重图当作完整词法图。

## 2. 真源：同 commit 树与 archive

- 选定 commit C（完整 sha）。共享 index 的 `git ls-files` 不作为真值。
- 树清单：`git ls-tree -rz --full-tree C`，保留 mode/type/object/path；原始输出保存并记 sha256。
- 快照：`git archive --format=tar C | tar -xf - -C out/context-preflight/<epoch>/snap`；tar 与解出文件清单各记 sha256。
- 核对门分类：
  | mode | 对拍键 | 结果 |
  |---|---|---|
  | 100644/100755 | path + mode + 解出文件 sha256（也记录 tree blob id） | 缺失/多余/内容或 mode 不同→SNAPSHOT-MISMATCH |
  | 120000 | path + mode + target 字节（不解析后比较） | 目标字节不同→MISMATCH |
  | 160000 | ls-tree 记录，archive 允许无该 path | GITLINK-UNEXPANDED + 相关根 UNKNOWN，不升格 MISMATCH；若解出同 path 常规文件→MISMATCH |
  保存 ls-tree 原始、extract 清单、分类计数。
- 120000 软链：按 target 字节单列；解析限快照内，累计 seen_paths（规范化词法相对 SNAP）；再遇已见→symlink-cycle；绝对目标、`..` 逃出 SNAP、悬空分 reason→UNKNOWN；禁止回读工作区/仓外。目录软链+词法 `..` 不等于 OS 父目录，不标 symlink-cycle，按多 context 计数。软链不进 `--in`。
- 路径无法无损表达（换行/回车/NUL、以 `#` 开头等）→具名拒绝/UNKNOWN。
- 快照可变：所有实际读取文件、每级解析与 symlink 目标在结束时复核 sha/identity；检测到变化→该根 UNKNOWN，不称原子只读。

## 3. 索引与产品身份

- `--in` 列表只含树中 mode 100644/100755 且 path 以 `.cx` 结尾的普通文件，相对快照根；清单 sha 记录。
- 在快照根执行：`sh <PRODUCT_ABS> tools/symindex.cx -- --index <OUT>/index --in <OUT>/in.lst update --verify`。
- `PRODUCT_ABS` 是显式传入的已装产品绝对路径，记录 sha256 与版本；不从 PATH/仓库根自动猜。当前示例：`/home/devali/repos/unisacc/unisacc.com`，`unisacc 0.0.43`，sha256 `fc1d4513fb1363c8506df8afa08444882337e615bf38d82dd42f022e2c23dc6f`；实际测量必须重新显式绑定，不继承示例。
- 索引出现 `DEFERRED`（45s 上限停止）或 manifest 不完整→本次 UNKNOWN，不当完整索引。
- 测量回执强制字段：`C, lstree_sha, archive_sha, extract_sha, in_sha, index_sha, product_path, product_sha, policy_doc_sha, implementation_source_sha, implementation_binary_sha, ENFORCEMENT, roots_tsv_sha`。
- cxm_norm 实际行号：src/cxmod.c:113、cx/compiler/cxmod.cx:143、cx/cx0/plan.cx:163（以当前文件为准复核对拍）。
- 根生成依赖 symindex `D main`，前提是索引完备；索引 DEFERRED/未完成即 UNKNOWN。

## 4. 根清单：来源、分类与核对门

拟提交 `tests/cx-context-roots.tsv`（未建），列：
`root<TAB>joint_id<TAB>double_count<TAB>category<TAB>provenance<TAB>expect<TAB>sha256`

类别：
- `production`：`src/**`、`release/**` 中 tracked 且含 main 定义的入口。
- `tool`：`tools/**` 中 tracked 且含 main 定义的入口。
- `batch-positive`：tests/batches.tsv 中 SOURCE，且该批次 coverage 预期为正常完成。
- `batch-negative`：SOURCE 属故意拒绝/缺模块/运行时生成的夹具；必须逐行给出 expect=negative-known 与理由。
- `generated`：路径由测试运行时生成或不在 tracked 树中的入口；expect=unknown-ok。
- `unclassified`：未归类的候选；不算覆盖，聚合 UNKNOWN。

候选全集生成规则（核对门，不静默）：
1. 从 commit C 的树清单取所有 mode 100644/100755 的 `.cx`；
2. 用固定同一 commit 的 symindex 索引查 `main` 定义（D main），得到入口候选；
3. 并上 tests/batches.tsv 的 SOURCE 去重；
4. 每个候选必须出现在 roots.tsv；缺失→`ROOTS incomplete`，聚合 UNKNOWN/非零；
5. roots.tsv 中不存在于候选全集的行→`ROOTS stale`，单列；
6. 候选数超过测量 ROOTS 上限→UNKNOWN/非零，不截断。
产品/工具根逐项由 1–2 生成后再人工固定 category/provenance；负例逐行固定，不用泛化分类代替。

## 5. 词法展开语义

- context 键：对“父 context 词法路径目录 + 模块名点转斜杠 + `.cx`”执行与编译器**同一 cxm_norm**（src/cxmod.c:113；cx/cx0/plan.cx:163；cx/compiler/cxmod.cx:143）：去除空段/`.`，`x/..` 词法折叠，不做文件系统查询。不得自创仅压缩斜杠。
- 注意并记录：`x/..` 按词法折叠，不按“x 是软链时 OS 解析到目标父目录”；这是现行编译器实际语义。preflight 只测量，不修正。
- 每条 `use` 从父 context 目录解析；模块名语法同编译器；`std` 按现行处理；不 realpath 合并 context。
- 同一根内相同规范化字面路径只展开一次；不同根分别展开；聚合再按字面路径去重。
- 环：目标规范化字面路径已在当前根的展开栈上→计 cycle 边、不再展开、该根标 UNKNOWN（编译器既有 module cycle 拒绝另记，不混报）。
- 依赖目标比较：两边 use 目标经 cxm_norm 后 stat/realpath 比较；缺失=unresolved，ENOTDIR/EACCES/读失败=UNKNOWN；目标不同记 `pairs_dependency_diff`，不误记为 public set。
- 缺失模块=unresolved+UNKNOWN；ENOTDIR/EACCES/读失败/路径规范化失败（过长/段数）=INVALID+UNKNOWN；不截断、不降级。
- U 事实：由快照索引 shard 读取；context 的 realpath 映射回 tracked path/sha 后取 shard；alias 软链用 realpath 查，但 context 身份仍是字面路径。未映射/未跟踪→UNKNOWN。
- 清单：按 context 词法目录+尾名读 cxpack；ENOENT/无匹配行=全集；EACCES/ENOTDIR/读失败/超长/非法行→UNKNOWN。有效公开集的比较只按第 7 节的 pair 策略执行；比较结果与计数见该节，§5 本身不另做拒绝。
- 跨入口对拍（开测前必须）：夹具覆盖 `./`、空段、`a/../b`、目录软链+`..`、绝对/相对；preflight 键与 unisacc 当前入口、gcc_cx/clang_cx 实际解析路径逐项一致，否则设计不通过。

## 6. 计数口径

- `contexts_total`：该根不同 context ID 数（累计集合大小）。
- `edges_total`：已遍历 use 指令数（累计，含 unresolved/cycle）。
- `path_bytes_total`：该根所有 context 规范化字面路径长度+1 的累计和；另报 longest_path。
- `max_group`：同一 realpath 的 context 组最大成员数；同 realpath 只用于报告与 pair，不用于展开去重。
- `unresolved`、`cycles`、`manifest_errors`、`invalid` 分别计数。
- `std_edges`：`use std`/`std.cx` 的边数（不建 context、不展开）；`use std.X` 按编译器具名拒绝，单列。
- `pairs_dependency_diff`：同 pair 两侧直接目标 realpath 不同的计数；按状态机处理。
- 聚合：各根累计求和 + 跨根 unique_contexts（按字面路径去重）；不把求和冒充全局同时峰值。

## 7. pair-policy v1（测量策略，不是生产算法）

版本标识：`pair-policy-v1`。若生产算法改变，另立版本，不复用此数。

- canonical 选择：**根内**每 realpath 组按“BFS 层序 → use 行序”首个 context 为 canonical；roots.tsv 行序只决定报告遍历，不能跨根选 canonical（见 §14）。联合根必须显式标记，联合根内部才共享一个图。
- 方向：单向 canonical→candidate；memo 键 `(canonical context ID, candidate context ID)`；每个 candidate 对照其组 canonical 执行递归核验（依赖递归同样用该组最新选定的 canonical）。
- 遍历：显式工作队列，按上述顺序；不递归 C 栈。记录 `pairs_visited_total`（首次进入的 memo 对）与 `pair_peak`（队列/核验栈同时存活的对数高水位）。
- 第二策略 `pair-policy-v1r`：同一根仅把兄弟 use 行序逆序，其余同 v1；单独报告 visited/peak。明确只覆盖两种顺序，不等于所有排列。
有效公开集比较与计数策略（v1 全遍历）：
- 每个 pair 比较 canonical 与 candidate 的有效公开集（按 §5 的清单规则，逐名集合；无清单/无匹配行=全部非 static）。记录 `pairs_public_equal`、`pairs_public_diff`、`pairs_errors`。
- v1 **不因差异或错误停止遍历**：有效集差异只记录；上下文/清单/目标解析错误计入 `pairs_errors`，该出错边不入队继续展开，其他边继续；根标 UNKNOWN。`pairs_visited_total`/`pair_peak` 只是该全遍历策略的测量值，不是任意生产算法的值。
- 不能对“任意首差异即停算法”声称上界：只有当生产算法与 v1 使用**相同的配对生成、去重键、canonical 选择和工作队列调度**，且停止仅裁剪同一条遍历时，全遍历值才可作该算法的条件上界；canonical 或调度不同可能使 peak 更大。除此之外一律 UNKNOWN，不比较、不外推。
- 遇错状态机（必须逐条实现）：
  | 事件 | 计数 | 入队/展开 | 根状态 |
  |---|---|---|---|
  | public set 相等 | public_equal++ | pair 已访 | 不变 |
  | public set 不等 | public_diff++ | 不因 diff 停；依赖子 pair 仍可入队 | 不变（纯测量） |
  | 清单 EACCES/ENOTDIR/读失败/非法 | pairs_errors++、manifest_errors++ | 该 context 出边不入队 | UNKNOWN |
  | use 目标缺失 | unresolved++、pairs_errors++ | 不入队 | UNKNOWN |
  | 目标 realpath 不同 | dependency_diff++ | 各按字面目标分别入队，diff 只记账 | UNKNOWN |
  | 规范化失败 | invalid++、pairs_errors++ | 不入队 | UNKNOWN |
  | proxy=1 | equal/diff/errors 必须为空/禁用峰门 | — | 不得 measurement_complete |
  `pairs_visited_total` 含首次进入 memo 的出错 pair；差异只停该边展开，不停全局遍历。
- 若实现没有执行有效集比较，只能输出 `pairs_proxy=1`，不得称实际递归核验，也不得用于 pair 峰门。
- 输出字段统一：`pairs_visited_total`、`pair_peak`、`pairs_public_equal`、`pairs_public_diff`、`pairs_dependency_diff`、`pairs_errors`、`pairs_proxy`（0/1）、`std_edges`；v1r 另列 `pairs_visited_total_v1r`、`pair_peak_v1r`。
- pair 数字只说明该策略的测量值，不作为生产实现上限或产品预算证明。

## 8. 测量上限与 deadline（独立于产品）

候选测量安全上限（待核准，不是产品合同）：
- MAX_ROOTS = 2048（候选超限 UNKNOWN/非零）
- MAX_CONTEXTS_PER_ROOT = 20000
- MAX_EDGES_PER_ROOT = 200000
- MAX_PATH_BYTES_PER_ROOT = 8 MiB
- MAX_MS = 50000（外部 60s 内留清理/汇总）
达到任一上限或超时→该根/聚合 UNKNOWN、rc 非零；不输出“低于上限/完整”。

## 9. 输出与状态

每根：
`ROOT<TAB>joint_id<TAB>double_count<TAB>category<TAB>status=measurement_complete|UNKNOWN<TAB>reason<TAB>contexts_total<TAB>edges_total<TAB>std_edges<TAB>pairs_visited_total<TAB>pair_peak<TAB>pairs_visited_total_v1r<TAB>pair_peak_v1r<TAB>pairs_public_equal<TAB>pairs_public_diff<TAB>pairs_dependency_diff<TAB>pairs_errors<TAB>pairs_proxy<TAB>path_bytes_total<TAB>longest_path<TAB>max_group<TAB>unresolved<TAB>cycles<TAB>manifest_errors<TAB>invalid<TAB>sha256`
聚合：`AGGREGATE<TAB>status<TAB>...` + 每类别 complete/unknown-known/unknown-unexpected 计数。
状态字段用 `measurement_complete`，不写裸 `complete`，避免被读成全仓/规范通过。仅当 production 类全部 measurement_complete、无 ROOTS incomplete、无触限/超时，聚合才 complete；否则 UNKNOWN 非零。negative-known 的 UNKNOWN 单列，不算生产覆盖，也不阻塞汇总为“测量完成”。

## 10. 拟路径与所有权

- `plans/cx-context-preflight-2026-10-31.md`（本文件，提案）
- 未来获批后：`tools/cx-context-preflight.cx`、`tests/cx-context-roots.tsv`、`tools/check-cx-context-preflight.cx`、batch `cx-context-preflight`
- 快照/索引/报告只写 `out/context-preflight/<epoch>/`；不写仓库根、不改已装产品、不改 cxmod/entry-build/spec。
- 未获四项细化核准前不建上述工具/测试文件、不开测。

## 11. 测中不写快照的强制措施

- 首选：独立 mount namespace + 只读 bind。示意（细节须先做 dry-run 验证）：
  bwrap --unshare-all --die-with-parent --chdir /work \
    --ro-bind SNAP /work --ro-bind PRODUCT /product \
    --ro-bind /usr /usr --ro-bind /lib /lib --ro-bind /lib64 /lib64 \
    --bind OUT /out --tmpfs /tmp --proc /proc --dev /dev \
    --setenv TMPDIR /tmp --setenv UNISACC_RUN_CACHE 0 \
    /product/unisacc.com /work/tools/cx-context-preflight.cx -- ... --out /out/report
  其中 SNAP 与 PRODUCT 外置且只读；OUT 是快照外唯一可写处；索引 /out/index、日志 /out/logs、临时 /tmp。cwd=/work，工具只接收 /work 或 /out 下的显式路径；宿主工作区不挂载。
  动态产品需要的解释器/库按实际 ldd 单独 ro-bind；不把整个 / 暴露为可写。
- 先证明方案可用：完整 dry-run 清单：bwrap 启动；/work 写失败 errno；/out 写成功；/product 只读可见；所需解释器/库可加载；宿主工作区路径探测失败；可见路径白名单核对。任一失败→UNKNOWN，不进入测量。
- 若 `bwrap` 或等价只读挂载不可用，本次测量状态 UNKNOWN，不用“前后复核”冒充强制只读。
- 次选：`chmod -R a-w SNAP` 并记录 mode；这只是同用户可逆约束，不构成强制。采用时 `enforcement=permission-only`，强制状态必须 UNKNOWN，不得 measurement_complete。
- 快照外目录：`out/context-preflight/<epoch>/{index,logs,tmp,report}`；`TMPDIR`、索引路径、工具输出均显式指向这里。
- 已装产品：记录绝对路径、版本、sha256；`UNISACC_RUN_CACHE=0` 只作为候选设置，不能证明无写入。若使用产品 `-run` 执行工具，另设只读 bind/沙箱；否则改为预先构建的工具二进制并用显式产品身份绑定。任何对快照的写入或无法排除的写路径→`SNAPSHOT-WRITE-UNKNOWN`，聚合 UNKNOWN。
- 结束复核仍是检测而非原子保证；只有强制只读措施成功才标 `enforcement=readonly-mount`。

## 12. 根清单与 pair-policy 文件身份

- 根清单：未来 `tests/cx-context-roots.tsv` 的 sha256 与 commit 一并记录；本文件只给生成规则与核对门，不内嵌未生成的根行。
- pair-policy 身份：`pair-policy-v1` 的定义即本文件第 7 节；记录三个独立字段：`policy_doc_sha`（本文件 sha256）、`implementation_source_sha`（实际 preflight .cx 源 sha）、`implementation_binary_sha`（实际执行二进制/产物 sha）。本文件 sha 只是提案身份，不能替代实现身份；测量数字必须绑定后两者。规则或实现变更必须改版本号，不复用 v1 数字。

## 13. 只读强制能力探针（先探测，后谈测量）

独立探针只回答“本机能否强制快照只读且隔离宿主工作区”，不读源码、不建索引、不做 preflight 测量。步骤与判定：

1. `command -v bwrap`；若存在，运行 dry-run：
   `bwrap --unshare-all --die-with-parent --ro-bind SNAP /work --bind OUT /out --tmpfs /tmp --proc /proc --dev /dev --chdir /work sh -c 'touch /work/probe 2>/dev/null; test $? -ne 0 && touch /out/probe && test -f /out/probe'`
   同时验证 `/product` 只读可见、所需解释器/库可加载、宿主工作区路径不可见。全部成功→`ENFORCEMENT=bwrap-ro-bind`。
2. 若无 bwrap，尝试 rootless mount namespace（含 private propagation）：
   `unshare --user --map-root-user --mount --propagation private sh -c 'mkdir -p /tmp/probe-snap /tmp/probe-out; mount --bind SNAP /tmp/probe-snap; mount -o remount,bind,ro /tmp/probe-snap; touch /tmp/probe-snap/x 2>/dev/null; test $? -ne 0; touch /tmp/probe-out/x'`
   只有“只读 bind 拒绝写 + OUT 可写 + 宿主路径不可见”三项同时成立才记 `ENFORCEMENT=unshare-ro-bind`；单 `unshare --user --map-root-user true` 成功不算。
3. 任一命令缺失/权限拒绝/挂载失败/无法验证隔离→`ENFORCEMENT=none`，本次 preflight UNKNOWN；不得退化为 chmod/permission-only 后称只读。
4. 记录每条探针命令、退出码、mount 表（若可读）、产品 `file`/`ldd` 与所需只读绑定清单；探针失败不阻塞设计评审，但阻塞测量。

本机当前观察（仅探针，不是 preflight）：`command -v bwrap` 退出1（未找到）；`unshare --user --map-root-user true` 退出0；另一次独立探针在自有 out/cx-ro-probe-FUiMLX 下用 `unshare --user --map-root-user --mount --propagation private` 对 bind 后 remount,bind,ro，触摸得到 `Read-only file system` 并打印 READONLY_WRITE_REFUSED、rc0。这只证明当前 Linux 用户+mount namespace 的只读 bind 可用；尚未证明运行 cx 工具所需解释器/库/产品依赖齐全、其他路径不可写/工作区不可见、快照合同完整。因此 `ENFORCEMENT` 仍不是完整通过，测量保持 UNKNOWN，直到第 1/2 条完整 dry-run（含依赖与隔离）通过。

## 14. 根分区与作用域

- 每个根代表独立编译图。context ID、canonical 选择、pair memo 与 pair 统计都以根身份分区：ID 含 root 前缀，或每个根独立编号后加 root；不得跨根按同一 realpath 合并、比较或声称会合并——它们未必在同一链接图。
- roots.tsv 的行序只决定报告/遍历顺序，不能把另一根的消费结果当本根 canonical，也不能用根序推断跨根合并。
- roots.tsv 增列 `joint_id`：空=独立根；同一 `joint_id` 的多行组成一个联合根，成员 root 行可重复。无 joint_id 的多 path 行拒绝。联合根与成员单根同时存在时须显式 `double_count=1`，否则核对门拒绝重复计量。
- 多源码同一命令是“联合根”场景，必须显式加 category/joint 标记；一个联合根的图内可以有自己的 realpath 组与 pair，但不等于把多个独立根合计后再比较。
- 各根 pair_visited/peak 的作用域仅该根（或显式联合根）；跨根求和只作统计量，不能作为任一图的峰值或产品上限证据。
- 跨根 aggregate 的 unique_contexts 也只用于规模统计，不触发身份比较或拒绝。


## 15. 本轮状态（csr 复审 4559d89ca6 后）

csr 判定项 3/4/5/6/8b 闭合，项 2/7/8a 主体闭合；本轮补：§5 cxm_norm 行号 113/143/163；§4 TSV 增 joint_id/double_count；§9 ROOT 增 v1r 列、AGGREGATE/category 统一 measurement_complete；§7 统一字段补 dependency_diff/std_edges；§1 声明 diff>0 非一致证明。仍未建工具/根清单、未开测；需 csr 对 4559 的硬残留再确认。
