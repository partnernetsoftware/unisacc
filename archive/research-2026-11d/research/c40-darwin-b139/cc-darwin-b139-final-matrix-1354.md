# 五 job 终表（1354 刷新；intel 已结）

身份：run `38111789528`；源码 SHA `82e1a9f06e9e39f805e25f0ad285b7306acb9937`（不跟随主树）；采样 `2026-10-11 13:56 SGT`（API jobs 只读）。
整体 conclusion：**failure**（三 macOS failure；linux×2 success）。

| job | databaseId | conclusion | 首红/关键行 | 证据 |
|---|---|---|---|---|
| linux (ubuntu-24.04-arm) | 114388705229 | **success** | — | jobs 只读；completed 05:02:20Z |
| linux (ubuntu-latest) | 114388705396 | **success** | — | jobs 只读；completed 05:02:50Z |
| macos (macos-14) | 114388705403 | **failure** | `FAIL bootstrap branches B=139 U=0`；acceptance10-Ctape/compare rc=1 | 1302 日志 sha256 `0f361b4e…`；bs_B/bs_U osx/arm64 2328418 B |
| macos (macos-15) | 114388705429 | **failure** | 同形 `B=139 U=0`；acceptance10-Ctape/compare rc=1；`##[error] exit 1` | 1314 日志 sha256 `0b52165f…`；bs_B/bs_U 同 macos-14 统计 |
| macos (macos-15-intel) | 114388705401 | **failure** | `FAIL bootstrap branches B=139 U=0`；**无** acceptance10（SKIP Darwin/arm64） | 1350 日志 sha256 `b2111630…`；bs_B/bs_U **osx/x86_64** 1816546/1800034 B |

## 终表结论
- **五 job 全结**；Draft 仍挡于 Darwin 三红（14/15/intel）。
- 三格同形症状族：bootstrap `B=139 U=0`（bound 映射疑似 SIGSEGV）；U 支成功；未进入 B tape vs C/UT cmp。
- **不可**把三格写成已证同一根因：14/15 为 arm64 同尺寸统计；intel 为 x86_64 不同尺寸；intel **跳过** acceptance10（Ctape/compare 三项红账仅 14/15）。
- 不以 linux success 替代 Darwin；不以 U 成功替代 B；不以本机 B.2 candidate 绿替代本终表。
