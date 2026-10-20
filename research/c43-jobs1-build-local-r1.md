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
