# c42 远端验证证据（2026-10-13，封存候选 0.0.42-dev）

性质：证据记录，不是验收结论。本地定点闸 `/tmp/c42-7000` 写本记录时仍在跑。

## 被验证的字节

| 项 | 值 |
|---|---|
| 候选 | `unisacc.com` sha256 `ba11d636b54adcdc…`（`release/candidate.json`，version 0.0.42） |
| 封存 | `ghcr.io/partnernetsoftware/unisacc-candidate@sha256:9da796385d67…`，标签 `0.0.42-dev` |
| 封存自 | 提交 `1fe598af`；`sources_sha256` 与该树闭包一致（`exec/c/provenance.py`） |
| 推送 | `f871dbaf`（封存记录），随后 `a940aa08`（只改 `.github/workflows/darwin-gate.yml`，产品闭包不变） |

## release-check（推送触发，run 38374765471，@f871dbaf）：全部 success

- candidate 六格：`macos-15` osx/arm64、`macos-15-intel` osx/x86_64、`ubuntu-latest` lnx/x86_64、`ubuntu-24.04-arm` lnx/arm64、`windows-latest` win/x86_64、`windows-11-arm` win/arm64。
- `matrix`（六格记录逐字节一致）、`source`、`ccinterop-x86`、`winsuite-prepare`、`winsuite` 两格。
- 说明：此前 `release/candidate.json` 封的是 0.0.40，树比封存新时 candidate 步骤会跳过。这是 0.0.42 候选第一次在 Darwin、Windows 和 arm64 上实际执行。

## darwin-gate（手动触发）

六个只在 `tests/gate.sh` 里、在 Linux 宿主上退 77 的套件，在 macOS runner 上对封存字节运行。

| run | 结果 | 说明 |
|---|---|---|
| 38374783359 @f871dbaf | intel 成功；arm64 失败 | arm64 在 `tests/gate.sh:46` 停下：未设 `UNISACC_FFI_X86_PROVIDER`。工作流接线问题，不是产品断言。 |
| 38387935866 @a940aa08 | 两格成功 | 变量指向一个不存在的路径；这两格的套件不读它。 |

第二次运行的逐套件结果（取自 run 日志）：

| 套件 | runner | rc | 墙钟 |
|---|---|---:|---:|
| exec-bootstrap-osxarm | macos-15 | 0 | 16 s |
| lib-stack-arm | macos-15 | 0 | 1 s |
| exec-bootstrap-osxx86 | macos-15-intel | 0 | 47 s |
| lib-stack-x86 | macos-15-intel | 0 | 3 s |
| lib-stack-x86-hostabi | macos-15-intel | 0 | 3 s |
| lib-windows-gp-native | macos-15-intel | 0 | 1 s |

注意：`exec-bootstrap-osxx86` 用了 47 秒，离 60 秒上限很近（cc2 复核时已预警）。不放宽上限；再慢就按规程拆套件。

## 这份证据关闭了什么，没关闭什么

- 关闭：上述六个套件在 Linux 宿主上的 UNVERIFIED_HOST（rc 77）覆盖义务，证据来源是 run 38387935866。
- 未关闭：本轮改为按规程退 77 的宿主格子里需要 Lima 客机或 zig 的部分（`ccinterop` 的 arm64 格、`syscall6`、`hosthdr` 的 Lima 格、`elfobj` 的 Mach-O 链接与 Lima 格、`windows-resolver-host`）；`forward` 的 macOS `-run` 宿主转发不在 darwin-gate 的套件里。
- 未验证：新的库模式 mmap 路由与按需 exec 转发路径在 macOS 和 lnx/arm64 上的专门用例。release-check 的演示套件跑的是通用程序，不专门覆盖这两条路径。

## 补充（2026-10-14）：darwin-gate 加跑 forward、com-forward

run 38433614903 @56131365（封存字节不变，`unisacc.com` sha256 `ba11d636…`，封存标签 0.0.42），两格成功。逐套件结果取自 run 日志：

| 套件 | macos-15 arm64 | macos-15-intel |
|---|---|---|
| forward | rc 0，14 s | rc 0，15 s |
| com-forward | rc 0，24 s | rc 0，36 s |
| exec-bootstrap-osx* | rc 0，17 s | rc 0，21 s |
| lib-stack-*、lib-windows-gp-native | rc 0 | rc 0 |

读法：`forward` 与 `com-forward` 在 Linux 闸上退 77（宿主转发的 `-run` 路径是 macOS 专有）。这次在 Darwin 上对封存字节实际执行并通过，8 项仍开的 UNVERIFIED_HOST 义务关掉 2 项，还剩 6 项：`ccinterop`、`syscall6`、`hosthdr`（各缺 Lima 客机格）、`elfobj`、`com-elfobj`（缺 macOS 链接器与 Lima arm64 格）、`windows-resolver-host`（缺 zig）。
另：run 38432061548 @95062fbf 是一次误触发，只重跑了原来的六个套件，结果同为成功。
