# 词法上下文 preflight 测量提案（2026-10-31）

状态：**提案，不授权开测/建工具/改 cxmod 或 entry-build/spec**。本文件是可审合同草案，不是产品预算、不是身份一致性实现、不是超限拒绝负例。W-1/CX-U-4/W-6/W-7 不变。

## 1. 目的与不覆盖

测同一 commit 下，从显式根集合展开的**词法上下文**规模：contexts、edges、path bytes、单向 pair 遍历的 visited/peak；为未来身份一致性预算提供观察值。

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
- 核对门：树清单与 archive 解出集合逐项比对（path、mode、blob 内容 hash）。缺失/多余/不一致→`SNAPSHOT-MISMATCH`，相关根 UNKNOWN、聚合非零。
- 160000 gitlink/submodule：无法由 archive 展开→该路径 UNKNOWN，不递归工作区。
- 120000 软链：单列其词法 path/target；只在快照内解析；绝对目标、逃出快照、悬空、成环→UNKNOWN；禁止回读工作区/仓外。普通文件与软链分开，不把软链送入 `--in`。
- 路径无法无损表达（换行/回车/NUL、以 `#` 开头等）→具名拒绝/UNKNOWN。
- 快照可变：所有实际读取文件、每级解析与 symlink 目标在结束时复核 sha/identity；检测到变化→该根 UNKNOWN，不称原子只读。

## 3. 索引与产品身份

- `--in` 列表只含树中 mode 100644/100755 且 path 以 `.cx` 结尾的普通文件，相对快照根；清单 sha 记录。
- 在快照根执行：`sh <PRODUCT_ABS> tools/symindex.cx -- --index <OUT>/index --in <OUT>/in.lst update --verify`。
- `PRODUCT_ABS` 是显式传入的已装产品绝对路径，记录 sha256 与版本；不从 PATH/仓库根自动猜。当前示例：`/home/devali/repos/unisacc/unisacc.com`，`unisacc 0.0.43`，sha256 `fc1d4513fb1363c8506df8afa08444882337e615bf38d82dd42f022e2c23dc6f`；实际测量必须重新显式绑定，不继承示例。
- 索引出现 `DEFERRED`（45s 上限停止）或 manifest 不完整→本次 UNKNOWN，不当完整索引。
- 记录：commit、ls-tree 原始 sha、archive sha、解出清单 sha、in.lst sha、index manifest sha、product sha。

## 4. 根清单：来源、分类与核对门

拟提交 `tests/cx-context-roots.tsv`（未建），列：
`root<TAB>category<TAB>provenance<TAB>expect<TAB>sha256`

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

- context 键：对“父 context 词法路径目录 + 模块名点转斜杠 + `.cx`”执行与编译器**同一 cxm_norm**（src/cxmod.c:112；cx/cx0/plan.cx:162；cx/compiler/cxmod.cx:??）：去除空段/`.`，`x/..` 词法折叠，不做文件系统查询。不得自创仅压缩斜杠。
- 注意并记录：`x/..` 按词法折叠，不按“x 是软链时 OS 解析到目标父目录”；这是现行编译器实际语义。preflight 只测量，不修正。
- 每条 `use` 从父 context 目录解析；模块名语法同编译器；`std` 按现行处理；不 realpath 合并 context。
- 同一根内相同规范化字面路径只展开一次；不同根分别展开；聚合再按字面路径去重。
- 环：目标字面路径已在当前展开栈上→计 cycle 边、不再展开、根 UNKNOWN（编译器既有 cycle 拒绝另记）。
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
- 聚合：各根累计求和 + 跨根 unique_contexts（按字面路径去重）；不把求和冒充全局同时峰值。

## 7. pair-policy v1（测量策略，不是生产算法）

版本标识：`pair-policy-v1`。若生产算法改变，另立版本，不复用此数。

- canonical 选择：每 realpath 组按“roots.tsv 行序 → BFS 层序 → use 行序”首个 context 为 canonical。
- 方向：单向 canonical→candidate；memo 键 `(canonical context ID, candidate context ID)`；每个 candidate 对照其组 canonical 执行递归核验（依赖递归同样用该组最新选定的 canonical）。
- 遍历：显式工作队列，按上述顺序；不递归 C 栈。记录 `pairs_visited_total`（首次进入的 memo 对）与 `pair_peak`（队列/核验栈同时存活的对数高水位）。
- 第二策略 `pair-policy-v1r`：同一根仅把兄弟 use 行序逆序，其余同 v1；单独报告 visited/peak。明确只覆盖两种顺序，不等于所有排列。
有效公开集比较与计数策略（v1 全遍历）：
- 每个 pair 比较 canonical 与 candidate 的有效公开集（按 §5 的清单规则，逐名集合；无清单/无匹配行=全部非 static）。记录 `pairs_public_equal`、`pairs_public_diff`、`pairs_errors`。
- v1 **不因差异或错误停止遍历**：差异只记录，错误把根标 UNKNOWN 并计入 `pairs_errors`，结构遍历继续。`pairs_visited_total`/`pair_peak` 是该全遍历策略的值，不是“首次差异即停”的生产算法值；若未来生产算法在首差异中止，本值只能作为上界。
- 若实现没有执行有效集比较，只能输出 `pairs_proxy`，不得称实际递归核验，也不得用于 pair 峰门。
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
`ROOT<TAB>category<TAB>status=complete|UNKNOWN<TAB>reason<TAB>contexts_total<TAB>edges_total<TAB>pairs_v1<TAB>pair_peak_v1<TAB>pairs_v1r<TAB>pair_peak_v1r<TAB>path_bytes_total<TAB>longest_path<TAB>max_group<TAB>unresolved<TAB>cycles<TAB>manifest_errors<TAB>invalid<TAB>sha256`
聚合：`AGGREGATE<TAB>status<TAB>...` + 每类别 complete/unknown-known/unknown-unexpected 计数。
仅当 production 类全部 complete、无 ROOTS incomplete、无触限/超时，聚合才 complete；否则 UNKNOWN 非零。negative-known 的 UNKNOWN 单列，不算生产覆盖，也不阻塞汇总为“测量完成”。

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
- 先证明方案可用：测量前做 dry-run（bwrap 启动、/work 只读写失败、/out 可写、看得到 /product 与所需 libs、宿主工作区不可见）；任一失败→UNKNOWN，不进入测量。
- 若 `bwrap` 或等价只读挂载不可用，本次测量状态 UNKNOWN，不用“前后复核”冒充强制只读。
- 次选：`chmod -R a-w SNAP` 并记录所有目录/文件 mode；这只是同用户可逆的约束，不构成强制；采用时明确标 `enforcement=permission-only`，不能称只读保证。
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
2. 若无 bwrap，尝试 rootless mount namespace：
   `unshare --user --map-root-user --mount sh -c 'mkdir -p /tmp/probe-snap /tmp/probe-out; mount --bind SNAP /tmp/probe-snap; mount -o remount,ro,bind /tmp/probe-snap; touch /tmp/probe-snap/x 2>/dev/null; test $? -ne 0; touch /tmp/probe-out/x'`
   只有“只读 bind 拒绝写 + OUT 可写 + 宿主路径不可见”三项同时成立才记 `ENFORCEMENT=unshare-ro-bind`；单 `unshare --user --map-root-user true` 成功不算。
3. 任一命令缺失/权限拒绝/挂载失败/无法验证隔离→`ENFORCEMENT=none`，本次 preflight UNKNOWN；不得退化为 chmod/permission-only 后称只读。
4. 记录每条探针命令、退出码、mount 表（若可读）、产品 `file`/`ldd` 与所需只读绑定清单；探针失败不阻塞设计评审，但阻塞测量。

本机当前观察（仅探针，不是 preflight）：`command -v bwrap` 退出1（未找到）；`unshare --user --map-root-user true` 退出0，只证明可建该用户命名空间，不证明 mount namespace/只读 bind/运行时挂载可用。因此当前 `ENFORCEMENT` 未定，测量必须 UNKNOWN，直到第 1 或第 2 条完整通过。
