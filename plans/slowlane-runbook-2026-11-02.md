# 慢车道 runbook（2026-11-02）

只编排。所有命令在主树仓根跑；`ROOT`=`$PWD`，`OUT`=新私有目录；构造链内部已逐作业 bound，允许一次调用跑完一个相位；普通测试/未知命令单条 ≤60s。每步失败即停，先核对回滚点，不盲目续跑。

## 前置（冻结，全绿才开跑）
1. `git diff HEAD -- tools tests cx src include weights exec` 为空（HEAD=工作区）；`git status --short` 无他人 in-flight。
2. 所有 omp 窗口（bdy/go/ds/go2）提交完毕，`../moltbaby/plans/omp-board.md` 无阻塞。
3. `sh ./unisacc.com tools/ratchets.cx` 9/9；`sh ./unisacc.com tools/check-source-job-count.cx` 绿；`git ls-tree` 核 kernelinputs 符号链接为 120000。
4. 记录慢车道起点 SHA：`git rev-parse HEAD`。

## 应用补丁（每步 git apply --check 后 apply；失败 = 回滚点 0）
1. `git apply plans/w5-patches/05-unit-cap128-full.patch`（E：bundle 64→128 + cx0-bundle 65→129）
2. `git apply plans/w5-patches/06-lower-codeend.patch`（D：codeend 一行）
3. `git apply plans/w5-patches/07-pp-negative.patch`（F：pp 两表）
4. **H**（host-provider 撤销）：已在工作区 staged（`D tools/check-host-provider.cx` + `M cli.cx/drive.cx/entrybind.cx/check-driver-cli.cx`），cc 先跑 `tools/check-driver-cli.cx`（def-provider-not-an-option 绿）再提交；若未 staged，按 `plans/host-provider-2026-11-02.md` 选 B 撤销。
5. **G**（FFI 字节表入 include/）：`tools/gen-ffi-bridge.cx --check` 绿 + `tools/check-ffi-bridge-bytes.cx` 绿 + `include/ffi_bridge_bytes.h` 是**普通文件**（非符号链接，rb_walk 只收 kind=='f'）。归 go，由 go 产出后 cc 审入。
6. 回滚点 1：`git apply -R` 逐条反向或 `git reset --hard <起点SHA>`（慢车道起点后无他人提交时）。

## 构造链（按序；每段独立 OUT 目录）
7. `source-package.cx prepare-context ROOT Root41-bootstrap.com /usr/bin/x86_64-linux-gnu-gcc-11 CTX`（bootstrap=unisacc.com 薄头 624B；CC 已装）。
8. `source-job-wave.cx ROOT CTX BUILD host` → `... list`（预期 `LIST authoritative_jobs=131 tools=5 gen=42 tbl=42 net=42`）→ `... wave tool|gen|tbl|net FIRST COUNT start-job|finish-job`（每相位 start+finish，count≤4 逐批覆盖 5/42/42/42）→ `... final`（必须 `VERIFIED`）。
9. `kernel-build.cx ROOT OUT bootstrap|prepare|prepare-assembler|preprocess-{x86_64,arm64}|assemble-{x86_64,arm64}|link-{x86_64,arm64}`（九步，kernels.md 顺序）。
10. `source-package-pack.cx prepare/model×34/route×6/declare/pack/seal/--verify`（README.md 构造链段落）。
11. `release-build.cx p3 ROOT SRC_PKG OUT_PKG` → `release-build.cx slice ROOT PKG OUT` → `entry-build.cx ROOT P3 OUT`。
12. （暂缓）`product-build.cx` slice×5 + wrap（两代 GEN）：`selfhost-mixed.cx endpoint prior/fresh/after-prior/after-fresh` ×4 + `compare`（固定点，SELFHOST_MIXED failed=0）。**product-build.cx / hostseed-step* / selfhost-mixed.cx 仍链已归档 C 参考，不能运行（README §run 承认），本步先跳过，等 33 对象入口链接回归后恢复。**
13. 回滚点 2：构造链任何一步失败，`OUT` 目录保留供检查，回到回滚点 1 重跑；不静默 promote 旧文件。

## kind2 / 安装 / 换标
14. `candidate-install.cx`（或 `cxpack-candidate-install.cx`）finish → `verify-prepared`；安装到仓根 `unisacc.com` + `unisacc.com.dev/`（新 entry.elf + p3-package.bin）。
15. 换标：`matrix-flip-gen.cx ROOT MAP.tsv OUT_DIR --migrate 旧候选HEX 新候选HEX LOGDIR`；`tools/check-matrix-stale.cx` 绿。
16. 回滚点 3：安装前已存 `bootstrap-before-install.com[.build.json]`，可恢复旧 unisacc.com。

## 全批 + 换标收尾
17. `sh ./unisacc.com tests/run.cx -- --all`（rc0；覆盖不完整必须非零）。
18. 棘轮复跑 `tools/ratchets.cx` 9/9 + srcshape 只降不升；记录新 entry+P3 SHA 与慢车道起点 diff。
19. 需主人：签名/Draft/Latest/公开（发布第 7 步）才请示；技术决定 cc 拍板。
