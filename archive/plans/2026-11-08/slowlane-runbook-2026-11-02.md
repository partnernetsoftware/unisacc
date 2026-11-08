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

> **7–12 已被 `tools/chain-step.cx` 取代**（一条命令一个相位、每相位核产物身份）。下面保留为口径说明；
> 实跑用 `sh ./unisacc.com tools/chain-step.cx -- <step> …`，见 `prd.md` 能力速查。
> 已过时、不再执行的旧步：`kernel-build.cx`（9）、`source-package-pack.cx`（10）、
> `product-build.cx` / `hostseed-step*` / `selfhost-mixed.cx`（12，仍链归档 C 参考，不能运行）。

7. 快照与 controller：`release-build.cx --rev REV p3` 首次调用即 `git archive` 出 `S=out/release-snap-<rev10>`；
   `B` 从已装 `unisacc.com`（+`.build.json`、`unisacc.com.dev/`）拷入，`S/tools/bound 55 sh B/Root41-bootstrap.com
   -O2 S/tools/source-package.cx -o B/source-package-O2.elf` 现编 controller（**不得 cp 旧 build**）。
8. `source-package.cx prepare-context S B/Root41-bootstrap.com /usr/bin/x86_64-linux-gnu-gcc-11 CTX`
   （bootstrap=unisacc.com 薄头 624B；CC 已装）→ `source-job-wave.cx S CTX B host` → `list`
   （预期 `LIST authoritative_jobs=131 tools=5 gen=42 tbl=42 net=42`）→ 逐相位
   `wave tool|gen|tbl|net FIRST COUNT(≤4) start-job|finish-job` → `final`（必须 `VERIFIED`，`CTX/jobs` 131 record）。
9. **p3 必须带 `--nets`**（缺则报错）：`release-build.cx --rev REV p3 S out/install44/pkg9/compiler.pkg OUT_PKG
   --nets $PWD/CTX/jobs --routes $PWD/out/install44/pkg9/declaration/routes.tsv`。
   验收：sha ≠ `9f1ec852`（旧包）、约 1.78MB、dev-lnx 包裸跑 65/72 rc0、129 rc1 `limit 128`。
10. 五片：`release-build.cx --rev REV slice S OUT_PKG OUT_<t> --target <t>`，
    t = `lnx/x86_64`（先）→ `lnx/arm64` → `osx/x86_64` → `win/x86_64` → `osx/arm64`
    （osx/arm64 用 `OUT/slice-lnx-x86_64` 作构造器）。**构造器一律 `UNISACC_CTOR=<本轮 lnx/x86_64 的 candidate>`**，
    不得用旧 `out/install44/candidate`（早于 140e8a9af8，`DV_MAXU=64`，43 源 76 单元 → `forwarding stub`）。
    whole-o 单作业 ≤120s（`rb_wait_child`）。
11. 打包与冒烟：`release-ape.cx pack OUT_APE OUT_PKG WIN LNX LNX_ARM OSX OSX_ARM` →
    `release-ape.cx smoke OUT_APE/unisacc-next.com`。注意 `ujs/product/ape-pack.cx:308` 目前只接受
    `P 1/2/3` 包头，而 p3 默认写 `P 4`；ujs 属论文 B 车道，**未获该车道支持前 APE 打包不可用**，
    安装走 `release-ape dev-lnx`。
12. 回滚点 2：构造链任何一步失败，`OUT` 目录保留供检查，回到回滚点 1 重跑；不静默 promote 旧文件。

## kind2 / 安装 / 换标
14. `candidate-install.cx`（或 `cxpack-candidate-install.cx`）finish → `verify-prepared`；安装到仓根 `unisacc.com` + `unisacc.com.dev/`（新 entry.elf + p3-package.bin）。
15. 换标：`matrix-flip-gen.cx ROOT MAP.tsv OUT_DIR --migrate 旧候选HEX 新候选HEX LOGDIR`；`tools/check-matrix-stale.cx` 绿。
16. 回滚点 3：安装前已存 `bootstrap-before-install.com[.build.json]`，可恢复旧 unisacc.com。

## 全批 + 换标收尾
17. `sh ./unisacc.com tests/run.cx -- --all`（rc0；覆盖不完整必须非零）。
17a. all-waves 会话(2026-11-06 rc1 教训):init 前必须先拿到材料负责人的最终 `MAP_READY`(带时间与 map 的 sha256),只用那一份 map init;init 后立刻 `diff 会话/materials.tsv 最终map`,不一致就丢弃会话重来。map 每行按批次参数形态写(目录 / 文件 / 64 位 hex),例如 `p3-pack-ctx` 是目录、`chain-gen1` 是 candidate 文件、`assembler` 是 `kernels/assembler` 子目录;写错形态会让批次读空或拒绝,而不是暴露真根因(RC1-MATERIAL-BIND)。
17a-补充(2026-11-06 rc1→rc2 间实测):**材料只能在跑会话的那个克隆里现场生成**,不能在一棵树里造、
另拿一棵树核。理由:`tools/asm-material.cx` 写进 `<材料>/assembler/material.tsv` 的第 4 行是生成时的
`root <绝对路径>`,而 `--verify` 会逐字节比这个文件,所以跨树核必然报
`asm-material: REFUSE prepared output/receipt differs from regenerated material`,
diff 出来的唯一差异就是那一行 root(实测:主树 `/home/devali/repos/unisacc` vs 会话克隆 `/tmp/wB/repo`)。
这不是材料坏了 —— 同一份材料在**会话克隆里**跑 `assembler-cache` 是 PASS,在主树上跑才 FAIL。
同理:会话绑定过的材料目录一律 `chmod -R a-w`,期间任何补跑都要换新目录(如 `/tmp/wB/mat3/kernels`)再做。
18. 棘轮复跑 `tools/ratchets.cx` 9/9 + srcshape 只降不升；记录新 entry+P3 SHA 与慢车道起点 diff。
19. 需主人：签名/Draft/Latest/公开（发布第 7 步）才请示；技术决定 cc 拍板。
