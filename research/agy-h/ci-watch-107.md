# 0.0.107 Step 3 CI 监视表与速率核 (Run 45512387135)

窗口: `agy-h` | Run: `45512387135` (`candidate-native.yml`, `v0.0.107`, `X=8a0b293973`, `APE2=b773c7e2...`) | 起跑: `2026-11-25 07:43:46 +0800` (`23:43:46Z`)
对照基准: `0.0.106` 成功 run `45446325698` (`release/evidence/0.0.106/ci-45446325698.json`, 总墙钟 `15m28s` = `928s`, `01:14:23 -> 01:29:51`)

---

## 1. 全 10 作业(前置 3 + 6 原生格 + 汇总)基准对照与预计完成时刻

| 作业名称 (`candidate-native.yml`) | `106` 执行时长 (`45446325698`) | `106` 距开跑累计偏移 | `107` 预计完成时刻 (`07:43:46` 起) | `07:44:40` 实测状态 (`45512387135`) | 速率/异常核对口径 |
|---|---|---|---|---|---|
| `fetch` (`ubuntu-24.04`) | `14.0s` (`+4s..+18s`) | `0m18s` | `07:44:04` | `success` (`07:43:50->07:43:59`, `9.0s`) | `9.0s < 14.0s`, 获草稿 6 资产 |
| `ffi-lnx` (`ubuntu-24.04`) | `22.0s` (`+21s..+43s`) | `0m43s` | `07:44:29` | `success` (`07:44:01->07:44:13`, `12.0s`) | `12.0s < 22.0s`, 参照输出就绪 |
| `cell-tools` (`ubuntu-24.04`) | `62.0s` (`+21s..+83s`) | `1m23s` | `07:45:09` | `success` (`07:44:02->07:45:09`, `67.0s`) | 吻合预计 `07:45:09` (0s 差) |
| `candidate (ubuntu-24.04, lnx, x86_64, yes)` | `42.0s` | `2m08s` | `07:45:54` | `success` (`07:45:12->07:45:56`, `44.0s`) | 首格绿 (`+2s` 差) |
| `candidate (windows-2025, win, x86_64, yes)` | `68.0s` | `2m34s` | `07:46:20` | `success` (`07:45:11->07:46:10`, `59.0s`) | `-10s`, 7 项全过 |
| `candidate (windows-11-arm, win, arm64, file)` | `79.0s` | `2m44s` | `07:46:30` | `success` (`07:45:11->07:46:32`, `81.0s`) | 挡板五后一次绿 (`+2s` 差) |
| `candidate (ubuntu-24.04-arm, lnx, arm64, yes)` | `114.0s` | `3m21s` | `07:47:07` | `success` (`07:45:14->07:47:03`, `109.0s`) | `-4s`, 原生与 SURF 全过 |
| `candidate (macos-15-intel, osx, x86_64, yes)` | `433.0s` (`7m13s`) | `8m40s` | `07:52:26` | `success` (`07:45:16->07:52:05`, `409.0s`) | `-21s`, 无阻塞收尾 |
| `candidate (macos-15, osx, arm64, yes)` | 执行 `212.0s` (`106` 排队至 `+686s`) | 无排队 `~4m55s`; 同 `106` `14m58s` | `07:48:41` (无排队) / `07:58:44` (排队) | `success` (`07:45:13->07:48:41`, `208.0s`) | 无排队直达, 吻合 `07:48:41` (0s 差) |
| `report` (汇总 `42/42` + `SURF`) | `27.0s` | 无排队 `~9m10s`; 同 `106` `15m28s` | `07:52:56` (无排队) / `07:59:14` (排队) | `success` (`07:52:07->07:52:31`, `24.0s`) | `CANDIDATE_QUALIFY verdict=QUALIFIED pass=42 fail=0 not_run=0 invalid=0`; `INDEX` 于 `07:52:48` (`ms=543801`) |

---

## 2. 只读监视模板与速率/停手判据

1. 每 `3-5` 分钟只读轮询命令(`timeout 12`):
   `timeout 12 gh -R partnernetsoftware/unisacc run view 45512387135 --json status,conclusion,updatedAt,jobs --jq '{status:.status,conclusion:.conclusion,jobs:[.jobs[]|{name:.name,status:.status,conclusion:.conclusion,startedAt:.startedAt,completedAt:.completedAt}]}'`
2. 速率与异常触发核:
   - 前置阶段(`fetch + ffi-lnx + cell-tools`): 若 `07:46:30` 仍未全部 `completed`, 检查 `cell-tools` 日志;
   - 前 4 格(`lnx/x64`, `win/x64`, `win/arm64`, `lnx/arm64`): 应在 `07:48` 前齐绿; 若 `win/arm64` 仅现 `version FAIL` 且 `fetch` 曾报 warning 属于 `car2-ci-dryrun.md` 类别 `1` (等整轮结束再单次 `--redispatch`); 任一格出现类别 `3`/`4` 原生失败立即报 `cc` 与 `cdx`, 绝不 `--redispatch` 或提前取消。
   - 长尾双 macOS 格: 若 `macos-15` 在 `07:55` 后仍处 `queued`, 按 `106` `11.4m` 排队口径预期收尾顺延至 `~07:59..08:02`, 不误判卡死。
3. 收尾核收(写入 `chain-to-public-r2.tsv` 与接 `Step 4 Y1`):
   - 出现 `conclusion=success` 后, 由 `agy-flow` 拉取 `ci-45512387135.{json,log}` 至 `release/evidence/0.0.107/` 并校验 `EVIDENCE_HOME_PATHS=0` 与 `CANDIDATE_QUALIFY verdict=QUALIFIED pass=42 fail=0 not_run=0 invalid=0`, 随后进入 `Step 4 Y1`。
