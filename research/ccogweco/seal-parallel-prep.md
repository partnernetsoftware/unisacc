# 0.1.0 封版车并行预做脚本合同(2026-11-25 ccogweco)

参照 agy-flow 的 car2-parallel-prep(P1-P6,正在写)与 fable 的零改动封版思路。本页只列 0.1.0 的合同;
与 agy-flow 页互引不重复其步骤。原则:预做只准备"输入与模板",不产生结论;107 的链产物不能当 0.1.0 的证据
(版本槽注入使 APE 必不同,fable 已裁否复用)。

## 0 两类依赖

- **A 类:只依赖 107 链产物**(可提前做,用作输入/对照,不作 0.1.0 证据):
  107 的 fixedpoint/g2 六切片与 compare 原行、107 的 strict 表(对照项)、107 的 ape-receipt(格式模板)、
  107 的 old-product 登记(0.0.107 行)、107 的 warm 收据(只作格式样例)、107 的 CI run(只作对照)。
- **B 类:必须依赖 0.1.0 自己的链产物**(链止后才能做,且是 0.1.0 的证据):
  0.1.0 的 fixedpoint/g2 与 g3、ape-receipt、pack 输出(pkg.bin、两个 build.json、SHA256SUMS)、
  0.1.0 的 strict 与 warm 收据、0.1.0 的 CI run。

## 1 并行预做件(P1-P6,0.1.0 版)

| 件 | 依赖 | 可提前时点 | 产出 | 禁止 |
|---|---|---|---|---|
| P1 说明骨架 | 无(只依赖模板) | 107 公开前 | notes-0.1.0 占位与回填命令 | 不填数字、不写满足 |
| P2 证据目录与头 | 文件名规范 | 107 公开前 | release/evidence/0.1.0/ 目录与空壳头 | 不写 sha/行 |
| P3 seal-precheck 输入清单 | 七条路径模板 | 107 公开后 | 七条判据的路径清单 | 清单不当结论 |
| P4 预热计划 | 107 的 warm 格式样例 | 107 公开后 | 0.1.0 pack 后的 warm 命令与 argv | 不用 107 layout 预热 |
| P5 回滚文本 | 107 的 rollback.txt 与 backup-evidence | 107 公开后 | 0.1.0 版回滚骨架(值待填) | 不预写备份 sha |
| P6 old-product 登记准备 | 107 的登记行格式 | 107 公开后 | 0.1.0 行模板(值取公开 APE) | 不预登记、不猜 sha |

- 说明:以上 P1-P6 全部是 A 类输入或模板;任何"用 107 数据填 0.1.0"的动作都不在本合同内。

## 2 版本槽注入点(0.1.0)

- 候选表槽:先把 `release/candidate.tsv` 的版本槽写成 `0.1.0`(唯一数据行),否则打包会拒。
- APE 注版:`tools/ape-pack.cx:294-296` 的 `stamp_version` -> `rv_inject_slot`,只写保留的 NUL 结尾 32 字节槽;
  非法或缺槽直接 `ape: invalid version or missing/duplicate version slot` rc 2。
- 产品版本一致性:`ape-pack.cx:486-492` 对编译器产品读 `release/candidate.tsv` 并要求 `--product NAME VERSION`
  的 VERSION 与候选一致,否则 `ape: product version differs from candidate`。
- 命令示例(参数按实际布局):

      sh ./unisacc.com tools/ape-pack.cx -- --payload p3.pkg --product unisacc 0.1.0 \
          <OUT>/unisacc.com <WIN_X86_64> <LNX_X86_64> <LNX_ARM64> <OSX_X86_64> <OSX_ARM64> [<WIN_ARM64>]

- win-arm64 切片的版本注入在 release-run 的 inject 步(version-inject),同样要求与候选表一致。

## 3 零变化三硬断言(链止后立即,顺序固定)

### (1) 产品闭包零差异

    git diff --stat <V107_FREEZE> <V010_FREEZE> -- src cx include exec kernel seed weights \
        iterate/kernel/typekw.tsv tools/bound tools/bound.c tools/cxbin tools/safedel.cx tools/lib.sh \
        tools/build_ref.sh tools/export_ref.sh tools/refshim.h tools/reffoot.h tools/sourceflat.cx \
        tools/ape-pack.cx tools/release-ape.cx tools/cxlib/releaseversion.cx

判据:无输出(rc 0)。辅核全树只含版本元数据、计划、清单与证据。

### (2) SOURCES_SID 相等

    SNAP_107=~/.cache/unisacc/chains/snap/<V107_FREEZE>
    SNAP_010=~/.cache/unisacc/chains/snap/<V010_FREEZE>
    IH=${UNISACC_IDENT_HELPER:-~/.cache/unisacc/ident-helper/identity-helper-real5}
    test -n "$("$IH" "$SNAP_010")" && test "$("$IH" "$SNAP_107")" = "$("$IH" "$SNAP_010")"

判据:test rc 0;链产物 `NEED	source_sid	<同 SID>` 与 pack 的 `SOURCES_SID	VERIFIED	<同 SID>`。

### (3) 未注版切片与 pkg.bin 相等、APE 由同一输入按版本槽重封

    for f in lnx-x86_64/entry.elf lnx-arm64/entry.elf osx-x86_64/slice-osx-x86_64 \             osx-arm64/slice-osx-arm64 win-x86_64/slice-win-x86_64 win-arm64/slice-win-arm64 p3-package.body; do      test "$(sha256sum <CHAIN_107>/fixedpoint/g2/$f | cut -d' ' -f1)" = \           "$(sha256sum <CHAIN_010>/fixedpoint/g2/$f | cut -d' ' -f1)" || exit 1    done    test "<PKG_107>" = "$(sha256sum -- <OUT>/layout/pkg.bin | cut -d' ' -f1)"    test "<APE_107>" != "$(sha256sum -- <OUT>/layout/unisacc.com | cut -d' ' -f1)"   # 裸字节必不同,差异来自重封传播,不是 32 字节    sh <OUT>/layout/unisacc.com --version    # 期望 0.1.0- 时间线位置:(1)(2) 在**链止后立即**(prep 之前)跑一次;(3) 的切片/pkg 部分在 pack 前,APE 注版部分在
  pack 之后、prep/X 之前再跑一次。任一失败:停,不进入 pack,不取消断言。

## 4 与 agy-flow 页的分工

- agy-flow 的 car2-parallel-prep 写 107 当天的 P1-P6;本页是其 0.1.0 版合同,两者共享"只准备不结论"原则。
- 107 的具体 P1-P6 步骤以 agy-flow 页为准;本页不复制其命令。

## 5 失败处置

- (1) 有差异:查是否把产品文件混入 0.1.0 分支;剥离后重跑,不得跳过。
- (2) SID 不等:用 `"$IH" --list` 比对两快照,定位越界改动;不得改 helper 或放宽比较。
- (3) 切片或 pkg 不等:非零变化,取消封版车按 R37 重排;APE 与 107 相同:说明注版未发生,检查候选槽与
  ape-pack 的 product/version 一致性。
