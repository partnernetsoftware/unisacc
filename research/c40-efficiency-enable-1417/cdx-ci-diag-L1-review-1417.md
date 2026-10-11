# cdx 真审 · L1 CI 诊断留证窄改（1417）

墙钟：2026-10-11 14:19 SGT。角色：cdx 独立审核。对照：`plans/ci-diag-retention-gap-1411.md`、`auth-same-sha-flaky-diag-default-1413.md` §B、`ruling-efficiency-enable-1417.md`。

## 审阅范围（仅此）
- `.github/workflows/ci.yml`：macos suites 设 `UNISACC_DIAG_OUT=${{ runner.temp }}/unisacc-diag`；失败 job `upload-artifact@v4`（retention 5 天，`if-no-files-found: warn`）。
- `tests/bootstrap.sh`：env 门控 `diag_copy`；unset 时 no-op；wait 后、FAIL/exit 前 copy 关键文件。

## 结论：**PASS（可进 tip）**

| 检查项 | 结果 | 说明 |
|---|---|---|
| 仅诊断、不改判定 | PASS | 无 continue-on-error 放宽；`bad`/bootstrap rc 逻辑未改 |
| 失败才上传 | PASS | `if: failure()`；成功路径不产 artifact |
| 门控 NEG | PASS | 本地 unset 无 diag 目录；syntax OK；N1/N2 形状核过 |
| 不依赖 diag 输入 | PASS | 去掉 `inputs.diag`，schedule / 普通 dispatch / 同 tip 下一跑均可带证 |
| 禁项 | PASS | 无 bump/Draft/公开/seal/version；无放宽验收措辞 |
| 文件域 | PASS | 仅 ci.yml + bootstrap.sh；无产品源 |

## 残留/非阻卡点
- attempt2（run 38111789528 @ 82e1a9f0）**不会**回溯带证（钉的是旧 tip）；本刀落地后**下一跑**失败才有 artifact。
- copy 列表不含 codesign stderr / crash report 全文（缺口表仍列）；本刀最小，够 B.log/U.log/bs_* 字节。
- `if-no-files-found: warn`：若 bootstrap 未跑到 wait 即挂，artifact 可能空壳——仍优于现状 total_count=0 且无阶段。

## 禁项自检
未 rerun；未改产品验收；未 bump/Draft/公开。
