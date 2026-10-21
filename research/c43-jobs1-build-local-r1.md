# JOBS=1 串行构建改动（本地保留，未落主，r1）

状态：**本地未提交**。`exec/c/buildcompiler.sh` 在主检出的工作树里有 4 行新增、1 行改动（见 `git diff -- exec/c/buildcompiler.sh`）。依 机房主任 2026-10-20 12:59 代裁（方案 2）：本拍不落 gatedeps/commitgate 声明刀，不换根包。

## 改了什么

两处原本并行的位置，加 `JOBS=1` 时改为顺序执行；`JOBS` 未设或为 0 时，路径与原来逐字节相同。

1. `pack_driver` 里的 asm 切片循环（五个目标 `win/x86_64 lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64`）。
2. `all` 分支里的 `pair()`（两两并行的 `shared`/目标对）。

## 为什么

主机负载下，内层 50 秒时限（`b()` 与 `pair` 的 `tests/bound`）会在并行段被打满，`shared` 步 rc142 反复出现。根因在一次孤儿 `seed-gen parse2` 占用 ~2 GB 内存（已清理），串行后分步构建通过。

## 证据（实际跑过）

- 独立副本（`/home/box/c43/0043-serial-r1/clone`，`git clone` 后检出 `fac46869`，只叠加本改动）：`shared`、六个目标、`pack-prep-1..3`、`pack` 全部 rc0，耗时约 1.5 分钟。
- 产物 `model2/unisacc-next.com`，sha256 `ffdc4622226d780465c4c9dc41a9ea50e9e07bbcacd6a656f5fdbdb30487349a`，2486900 字节。
- `ident . check`：rc0，"product freshness: verified"。
- 冒烟：`-run` 与 `-o` 两路返回值均为 7。
- 七个 0.0.43 闸套件（`exec-librarycontract-model`、`exec-librarytypedpartition-model-1..4`、`lib-typed-host`、`lib-variadic-host`、`lib-sig2-host`）各自单独调用，全部 rc0。合并一批跑会撞批次上限（rc142），那不是套件红。

## 审查

子代理只读审查 diff：默认路径不变；`pair()` 的失败语义与原来一致（第一项失败则不跑第二项，返回码透传）；除这两处外，`buildcompiler.sh` 及其直接调用的脚本中没有其他并行构造。

## 未做（本拍边界）

- 没有 gatedeps 声明：`identity-include-consumer-negative` 是唯一声明该文件的套件，且被分类为契约套件，commitgate 因此拒收（"product closure changes need --suite naming a product (non-contract) suite"）。落主需要单独的声明刀。
- 根包 `unisacc.com` 仍是旧引导件（sha `0e0531e3…f8`），未换为 `ffdc4622`。
- 未推送，未 Draft、Latest、签名、公开，未动 resume7000 与 987。

## 恢复或落主的条件

1. 为 `exec/c/buildcompiler.sh` 在某个非契约产品套件的 `tests/gatedeps.json` 中补声明，并通过 commitgate；或
2. 由 机房主任 另授根包替换，并重新验证新鲜度。

## 附录（2026-10-21 22:3x–23:0x）：落主、重建与本机 seed-gen 偶发卡死

- JOBS=1 已以声明刀落主（8408128c）：commitgate 点名 exec-core / exec-librarycontract-model / lib-typed-host（gatedeps 已含 exec 树声明，本笔记「唯一契约套件声明」的旧状已过时）；reviewed-trees 戳按 reviewedtreescheck 规则对工作树自算后单戳替换（refresh_gatedeps 按 HEAD 归档算戳的 catch-22 仍在）。
- 冻结树 wt44（tip 8408128c）候选与 wt43（c5b028fc）候选逐字节同：com `ada5f0f7…`、seed `4fae4286…`——JOBS 未设时路径逐字节不变的声明由两次独立构建印证。
- **本机新坑**：`seed-gen parse2 --warnings --errors` 偶发启动即卡死（user 0.02s、内核态、`tests/bound` SIGKILL 收割不动，一次挂 11 分钟；strace 下同二进制同参数 15 秒退 0；四次观测 2 卡 2 过）。当时 16GB 内存并驻多会话用到 13.1GB，seed-gen 峰值 ~2GB，卡点在 brk/mmap 段，疑宿主沙箱层偶发；非仓库回归。处置：`build_candidate.sh` 外层整轮重试（全新目录、绿即停），一次端到端全绿即收据。机器变钝先查 D 态进程与大 RSS。
