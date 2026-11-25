# 0.1.0 链止回填空跑(用 107 链目录演练;2026-11-25 ccogweco)

对象:research/ccogweco/v010-chain-fill-script.md。用 107 第二轮链目录
`~/.cache/unisacc/chains/bg-b1/63354b4a2cb707ecc923a89f0df6030149fa697f` 逐条跑只读取值命令,验证可执行与
输出格式;已在 v010-chain-fill-script.md 修正两处路径/搜索面问题。未动 0.1.0 链,未写任何仓内文件。

## 0 结论

- T1/T2 命令可用:`owner.tsv` 首行字段 2 为 T1 epoch(`OWNER 1795554510 F ...`),`exit.tsv` 字段 2 为 T2
  (`EXIT 1795556095 pid=... rc=0 why=exited`)。
- 固定点四同不在 bg.log,而在 `chain-run-<N>.stdout`(107 为 chain-run-245.stdout):`CHAIN	fixedpoint	slices=same count=6`
  等;脚本必须 `grep -h 'CHAIN	fixedpoint' "$CHAIN"/*.stdout`,只搜 bg.log 会漏。
- 六目标 g2/g3 的路径不是 `g2/<target>/<target>`,而是:
  `lnx-x86_64/entry.elf`、`lnx-arm64/entry.elf`、`osx-x86_64/slice-osx-x86_64`、`osx-arm64/slice-osx-arm64`、
  `win-x86_64/slice-win-x86_64`、`win-arm64/slice-win-arm64`;g3 同构。脚本已改为映射表。
- APE5 命令可用:`ape-final/unisacc-next.com`(107 演练得 b773c7e2…,与当轮 APE 一致);回执 `ape-final/ape-receipt.txt`
  存在;`p3-package.body` 在 ape-final,`p3.pkg` 由 pack 生成,链阶段不要去找 p3.pkg。
- SID5 在链阶段取不到(没有 pack),脚本保持"pack 后再填"。

## 1 逐条演练输出(107 链目录)

    head -1 owner.tsv
    OWNER	1795554510	63354b4a2cb707ecc923a89f0df6030149fa697f	guard=2968888	pid=2968889	pgid=2968889	deadline_s=5400	chain=source
    cat exit.tsv
    EXIT	1795556095	pid=2968889	rc=0	why=exited
    grep -h 'CHAIN	fixedpoint' chain-run-245.stdout
    CHAIN	fixedpoint	objects=same	count=102
    CHAIN	fixedpoint	kernels=same	count=2
    CHAIN	fixedpoint	slices=same	count=6
    CHAIN	fixedpoint	product=same
    (package=present sha=... 同行另在后续行,按实际 grep 取)
    sha256sum ape-final/unisacc-next.com
    b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63

## 2 脚本修正(v010-chain-fill-script.md 已更新)

- §2 搜索面:`grep -h 'CHAIN	fixedpoint' "$CHAIN"/*.stdout`(不是 bg.log)。
- §3 路径映射表改为:

      lnx-x86_64/entry.elf
      lnx-arm64/entry.elf
      osx-x86_64/slice-osx-x86_64
      osx-arm64/slice-osx-arm64
      win-x86_64/slice-win-x86_64
      win-arm64/slice-win-arm64

  每项分别对 g2 与 g3 取 sha 并比较。
- §4 明确 `p3.pkg` 不在链阶段(ape-final 只有 `p3-package.body`),不要提前找;SID5 等 pack。

## 3 5 分钟回填顺序(演练后确认)

1. T1/T2(owner/exit);
2. 四同与 package(chain-run-*.stdout);
3. 六目标 g2/g3 映射对比;
4. APE5 与回执;
5. 三硬断言原行(需 0.1.0 链与 pack);
6. 链后 INDEX 行(可能尚未生成,保持占位)。

## 4 边界

- 本页用 107 链目录演练,0.1.0 链未读未动;命令全只读。
- 演练发现的两处脚本问题已改;若 0.1.0 链目录布局与 107 不同(如 target 目录名),按实际 `ls` 结果再映射,
  不硬编码。
