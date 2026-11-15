# 外宿主测试计划（cc，2026-10-28 19:0x；fable 裁定：持窗期唯一非算力、非保留域且影响发布资格的空白）

范围：18 张域矩阵中状态 `NEEDS_FOREIGN_HOST` 的全部 79 格（lnx/arm64 19、osx/x86_64 18、osx/arm64 18、win/x86_64 18、win/arm64 6），与 `plans/dev-methodology-2026-10-28.md:37` 的 79 一致（本计划由 `awk -F'\t' '$4=="NEEDS_FOREIGN_HOST"' tests/*-domain-matrix.tsv` 机械生成，可复核）。本计划只准备；任何格在真实宿主实跑前保持 NEEDS_FOREIGN_HOST，不记 PASS。

## 1. 宿主通道合同（每个宿主一次）

1. 输入身份：候选产品 = 当前准入 Root（写入时 4c2831c3，`sha256sum unisacc.com`），源 = 同一 HEAD 的 `git archive`；两者 sha 写入 `out/foreign/<host>/IDENTITY.tsv`（host、uname、产品 sha、HEAD、日期）。宿主标识 `<host>` ∈ lnx-arm64 | osx-x86_64 | osx-arm64 | win-x86_64 | win-arm64。
2. 每条命令 ≤60 s（`tools/bound 55 …`；Windows 用 `timeout /t` 等价或 PowerShell `Start-Process -Wait` + 超时），无本机路径依赖：所有路径相对导出根。
3. 跑法：`sh ./unisacc.com tests/run.cx -- --list` 列出登记批；外宿主格对应的"原生执行"证据由本机已验证的同名批在宿主上重跑（镜像/对象批在宿主产出并执行），结果以 `BATCH_RUN … candidate=<sha>` 行为准，落 `out/foreign/<host>/<matrix>.tsv`（列：cell、batch、rc、stdout 字节、期望、实况、PASS|FAIL|UNRUN）。
4. 回流换标：只在下一次慢车道纪元、由矩阵主人把 `NEEDS_FOREIGN_HOST` 改为 `COVERED_NATIVE@<sha>`，证据格式 `batch@<sha>:N/0;out/foreign/<host>/<matrix>.tsv;cells`；UNRUN/FAIL 原状保留。

## 2. 逐格清单（host、矩阵、cell、要求、预期/oracle、证据落点）

| host | matrix | cell | requirement | oracle | evidence file |
|---|---|---|---|---|---|
| lnx/arm64 | bootstrap | E12.native-chain | same candidate runs and rebuilds natively, bytes equal | sha256 | out/foreign/lnx-arm64/bootstrap.tsv |
| osx/x86_64 | bootstrap | E12.native-chain | same | sha256 | out/foreign/osx-x86_64/bootstrap.tsv |
| osx/arm64 | bootstrap | E12.native-chain | same | sha256 | out/foreign/osx-arm64/bootstrap.tsv |
| win/x86_64 | bootstrap | E12.native-chain | same | sha256 | out/foreign/win-x86_64/bootstrap.tsv |
| lnx/arm64 | candidate | ci.native | same tuple observed natively | same | out/foreign/lnx-arm64/candidate.tsv |
| osx/x86_64 | candidate | ci.native | same | same | out/foreign/osx-x86_64/candidate.tsv |
| osx/arm64 | candidate | ci.native | same, arm64 process (not Rosetta) | same | out/foreign/osx-arm64/candidate.tsv |
| win/x86_64 | candidate | ci.native | same | same | out/foreign/win-x86_64/candidate.tsv |
| lnx/arm64 | formats | native.image-and-object | lnx/arm64 image and linked unit object execute | exit/stdout | out/foreign/lnx-arm64/formats.tsv |
| osx/x86_64 | formats | native.image | Mach-O x86_64 image executes after ad-hoc signature check | exit/stdout | out/foreign/osx-x86_64/formats.tsv |
| osx/arm64 | formats | native.image | Mach-O arm64 image loads via dyld and executes | exit/stdout | out/foreign/osx-arm64/formats.tsv |
| win/x86_64 | formats | native.image | PE x86_64 image executes | exit/stdout | out/foreign/win-x86_64/formats.tsv |
| win/arm64 | formats | native.image | PE arm64 user image executes natively | exit/stdout | out/foreign/win-arm64/formats.tsv |
| lnx/arm64 | interfaces | if.host.lnx-arm64 | required cells on native lnx/arm64 | this matrix | out/foreign/lnx-arm64/interfaces.tsv |
| osx/x86_64 | interfaces | if.host.osx-x86_64 | required cells on native osx/x86_64 | this matrix | out/foreign/osx-x86_64/interfaces.tsv |
| osx/arm64 | interfaces | if.host.osx-arm64 | required cells on native osx/arm64 | this matrix | out/foreign/osx-arm64/interfaces.tsv |
| win/x86_64 | interfaces | if.host.win-x86_64 | required cells on native win/x86_64 (pipes, '-' stdin, binary stdout mode) | this matrix | out/foreign/win-x86_64/interfaces.tsv |
| lnx/arm64 | library | lib.all-local-cells | all lnx/x86_64 cells natively (AAPCS64 layout constants) | same oracles with AAPCS64 derivation | out/foreign/lnx-arm64/library.tsv |
| osx/arm64 | library | lib.all-local-cells | all cells natively; macOS FFI bounded host capability | same oracles | out/foreign/osx-arm64/library.tsv |
| osx/x86_64 | library | lib.all-local-cells | all cells natively | same oracles | out/foreign/osx-x86_64/library.tsv |
| win/x86_64 | library | lib.all-local-cells | all cells natively; LLP64 layout; Toolhelp module order for us_declare_import; dllexport s | Win64 ABI derivation | out/foreign/win-x86_64/library.tsv |
| win/arm64 | library | lib.all-local-cells | all cells natively with native ARM64 PE | ARM64 Windows ABI derivation | out/foreign/win-arm64/library.tsv |
| lnx/arm64 | negative | host.diag-and-loader-corpus | diagnostic and loader cells on native host | same literals | out/foreign/lnx-arm64/negative.tsv |
| osx/x86_64 | negative | host.diag-and-loader-corpus | same | same literals | out/foreign/osx-x86_64/negative.tsv |
| osx/arm64 | negative | host.diag-and-loader-corpus | same | same literals | out/foreign/osx-arm64/negative.tsv |
| win/x86_64 | negative | host.diag-and-loader-corpus | same | same literals | out/foreign/win-x86_64/negative.tsv |
| lnx/arm64 | optimization | host.six-flag-execution | six-flag images execute to literals natively | same literals | out/foreign/lnx-arm64/optimization.tsv |
| osx/x86_64 | optimization | host.six-flag-execution | same | same literals | out/foreign/osx-x86_64/optimization.tsv |
| osx/arm64 | optimization | host.six-flag-execution | same | same literals | out/foreign/osx-arm64/optimization.tsv |
| win/x86_64 | optimization | host.six-flag-execution | same | same literals | out/foreign/win-x86_64/optimization.tsv |
| win/arm64 | optimization | host.six-flag-execution | same (user image only; product-native ARM64 is a hosts/candidate cell) | same literals | out/foreign/win-arm64/optimization.tsv |
| lnx/arm64 | phase-timing | host.phase-report | same report natively | same | out/foreign/lnx-arm64/phase-timing.tsv |
| osx/x86_64 | phase-timing | host.phase-report | same | same | out/foreign/osx-x86_64/phase-timing.tsv |
| osx/arm64 | phase-timing | host.phase-report | same | same | out/foreign/osx-arm64/phase-timing.tsv |
| win/x86_64 | phase-timing | host.phase-report | same | same | out/foreign/win-x86_64/phase-timing.tsv |
| lnx/arm64 | properties | prop.fold.six-targets | one tape -> six images; native stdout/exit identical on each host (lnx/x86_64 local leg co | literal outputs across hosts | out/foreign/lnx-arm64/properties.tsv |
| lnx/arm64 | properties | prop.all-local-cells | all lnx/x86_64 cells natively | same oracles | out/foreign/lnx-arm64/properties.tsv |
| osx/arm64 | properties | prop.all-local-cells | all cells natively | same oracles | out/foreign/osx-arm64/properties.tsv |
| osx/x86_64 | properties | prop.all-local-cells | all cells natively | same oracles | out/foreign/osx-x86_64/properties.tsv |
| win/x86_64 | properties | prop.all-local-cells | all cells natively | same oracles | out/foreign/win-x86_64/properties.tsv |
| win/arm64 | properties | prop.all-local-cells | all cells natively with native ARM64 PE | same oracles | out/foreign/win-arm64/properties.tsv |
| lnx/arm64 | resources | res.all-local-cells | all lnx/x86_64 cells re-observed natively on same candidate | same oracles | out/foreign/lnx-arm64/resources.tsv |
| osx/arm64 | resources | res.all-local-cells | all cells natively (RLIMIT_AS semantics differ on Darwin; NOT_RUN where unsupported, never | same oracles | out/foreign/osx-arm64/resources.tsv |
| osx/x86_64 | resources | res.all-local-cells | all cells natively | same oracles | out/foreign/osx-x86_64/resources.tsv |
| win/x86_64 | resources | res.all-local-cells | equivalent limits via Job Objects; Windows watchdog unfinished | same oracles | out/foreign/win-x86_64/resources.tsv |
| win/arm64 | resources | res.all-local-cells | as win/x86_64 with native ARM64 PE | same oracles | out/foreign/win-arm64/resources.tsv |
| lnx/arm64 | ri | ri.all-local-cells | every lnx/x86_64 cell above re-observed natively on same candidate | same oracles | out/foreign/lnx-arm64/ri.tsv |
| osx/arm64 | ri | ri.all-local-cells | every lnx/x86_64 cell above re-observed natively on same candidate | same oracles | out/foreign/osx-arm64/ri.tsv |
| osx/x86_64 | ri | ri.all-local-cells | every lnx/x86_64 cell above re-observed natively on same candidate | same oracles | out/foreign/osx-x86_64/ri.tsv |
| win/x86_64 | ri | ri.all-local-cells | every lnx/x86_64 cell above re-observed natively on same candidate | same oracles | out/foreign/win-x86_64/ri.tsv |
| win/arm64 | ri | ri.all-local-cells | every lnx/x86_64 cell above re-observed natively on same candidate (native ARM64 PE, not e | same oracles | out/foreign/win-arm64/ri.tsv |
| lnx/arm64 | stream-e1 | e1.host.lnx-arm64 | required cells on native lnx/arm64 | this matrix | out/foreign/lnx-arm64/stream-e1.tsv |
| osx/x86_64 | stream-e1 | e1.host.osx-x86_64 | required cells on native osx/x86_64 | this matrix | out/foreign/osx-x86_64/stream-e1.tsv |
| osx/arm64 | stream-e1 | e1.host.osx-arm64 | required cells on native osx/arm64 | this matrix | out/foreign/osx-arm64/stream-e1.tsv |
| win/x86_64 | stream-e1 | e1.host.win-x86_64 | required cells on native win/x86_64 | this matrix | out/foreign/win-x86_64/stream-e1.tsv |
| lnx/arm64 | stream-e2 | e2.host.lnx-arm64 | required cells run natively on lnx/arm64 | this matrix | out/foreign/lnx-arm64/stream-e2.tsv |
| osx/x86_64 | stream-e2 | e2.host.osx-x86_64 | required cells run natively on osx/x86_64 | this matrix | out/foreign/osx-x86_64/stream-e2.tsv |
| osx/arm64 | stream-e2 | e2.host.osx-arm64 | required cells run natively on osx/arm64 | this matrix | out/foreign/osx-arm64/stream-e2.tsv |
| win/x86_64 | stream-e2 | e2.host.win-x86_64 | required cells run natively on win/x86_64 | this matrix | out/foreign/win-x86_64/stream-e2.tsv |
| lnx/arm64 | stream-e3 | e3.host.lnx-arm64 | required cells on native lnx/arm64 | this matrix | out/foreign/lnx-arm64/stream-e3.tsv |
| osx/x86_64 | stream-e3 | e3.host.osx-x86_64 | required cells on native osx/x86_64 | this matrix | out/foreign/osx-x86_64/stream-e3.tsv |
| osx/arm64 | stream-e3 | e3.host.osx-arm64 | required cells on native osx/arm64 | this matrix | out/foreign/osx-arm64/stream-e3.tsv |
| win/x86_64 | stream-e3 | e3.host.win-x86_64 | required cells on native win/x86_64 | this matrix | out/foreign/win-x86_64/stream-e3.tsv |
| lnx/arm64 | stream-e4 | e4.host.lnx-arm64 | required cells on native lnx/arm64 | this matrix | out/foreign/lnx-arm64/stream-e4.tsv |
| osx/x86_64 | stream-e4 | e4.host.osx-x86_64 | required cells on native osx/x86_64 | this matrix | out/foreign/osx-x86_64/stream-e4.tsv |
| osx/arm64 | stream-e4 | e4.host.osx-arm64 | required cells on native osx/arm64 | this matrix | out/foreign/osx-arm64/stream-e4.tsv |
| win/x86_64 | stream-e4 | e4.host.win-x86_64 | required cells on native win/x86_64 | this matrix | out/foreign/win-x86_64/stream-e4.tsv |
| lnx/arm64 | stream-elf | elf.host.lnx-arm64 | required cells and lnx/arm64 images executed natively | native exit/stdout | out/foreign/lnx-arm64/stream-elf.tsv |
| osx/x86_64 | stream-elf | elf.host.osx-x86_64 | osx/x86_64 Mach-O executed natively (ad-hoc signature accepted by the loader) | native exit/stdout | out/foreign/osx-x86_64/stream-elf.tsv |
| osx/arm64 | stream-elf | elf.host.osx-arm64 | osx/arm64 Mach-O executed natively | native exit/stdout | out/foreign/osx-arm64/stream-elf.tsv |
| win/x86_64 | stream-elf | elf.host.win-x86_64 | win/x86_64 PE executed natively | native exit/stdout | out/foreign/win-x86_64/stream-elf.tsv |
| lnx/arm64 | stream-lower | lower.host.lnx-arm64 | required cells on native lnx/arm64 | this matrix | out/foreign/lnx-arm64/stream-lower.tsv |
| osx/x86_64 | stream-lower | lower.host.osx-x86_64 | required cells on native osx/x86_64 | this matrix | out/foreign/osx-x86_64/stream-lower.tsv |
| osx/arm64 | stream-lower | lower.host.osx-arm64 | required cells on native osx/arm64 | this matrix | out/foreign/osx-arm64/stream-lower.tsv |
| win/x86_64 | stream-lower | lower.host.win-x86_64 | required cells on native win/x86_64 | this matrix | out/foreign/win-x86_64/stream-lower.tsv |
| lnx/arm64 | stream-prune | prune.host.lnx-arm64 | required cells on native lnx/arm64 | this matrix | out/foreign/lnx-arm64/stream-prune.tsv |
| osx/x86_64 | stream-prune | prune.host.osx-x86_64 | required cells on native osx/x86_64 | this matrix | out/foreign/osx-x86_64/stream-prune.tsv |
| osx/arm64 | stream-prune | prune.host.osx-arm64 | required cells on native osx/arm64 | this matrix | out/foreign/osx-arm64/stream-prune.tsv |
| win/x86_64 | stream-prune | prune.host.win-x86_64 | required cells on native win/x86_64 | this matrix | out/foreign/win-x86_64/stream-prune.tsv |

## 3. 宿主执行顺序（每宿主约 10–15 min）

1. IDENTITY.tsv → 2. `tests/run.cx -- --list` 存档 → 3. 本矩阵顺序：interfaces → stream-e2/e1/e3/e4/prune/lower/elf → ri/library/resources → formats/optimization/properties → bootstrap/candidate/phase-timing/negative；每批独立 bound，失败不阻塞后续。
4. win/arm64 的 6 格需原生 ARM64 PE 产品（当前 BLOCKED 项之外的 user image 只需宿主）。
5. 结束：`sha256sum out/foreign/<host>/*.tsv > out/foreign/<host>/SUMS` 随导出目录一并交回主刀。
