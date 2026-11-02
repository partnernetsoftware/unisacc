# W6 A 档归档（2026-11-02）

来源：`plans/w6-tools-diet-2026-11-02.md` A 档；只做 `git mv`，内容未改。

| 归档文件 | 原路径 | SHA256 |
|---|---|---|
| `check-cx-sys-fs.cx` | `tools/check-cx-sys-fs.cx` | `3bbb5d078ac8942ac8317567cdd860b7ae5bd80ba2cba0749c7188106dbe6184` |
| `deadloop-guard.cx` | `tools/deadloop-guard.cx` | `b99f2777bc945349a02801cf44910efe83c62d0401abaaa4f5d814eeb70b159d` |

说明：`check-cx-sys-fs.cx` 由 `check-cx-sys-*.cx` 覆盖；`deadloop-guard.cx` 仅盘点文档引用。

| `check-core-execute-real.cx` | `tools/check-core-execute-real.cx` | `47c722b5a1690cf9e2d338df1322c3e74d9be393574b76d4e981bdf159b23d9a` |
| `check-core-transition-real.cx` | `tools/check-core-transition-real.cx` | `7ad855e461e1769d2dcec35ebd1725964696a21a9cc5632e346621551b1afd53` |
| `check-cx-cross-units.cx` | `tools/check-cx-cross-units.cx` | `a8b849fb1d0375c948f08d0fd75afff477a0fe9601190f2ecad46ba5bd3f2727` |
| `check-cx-driver-target-unit.cx` | `tools/check-cx-driver-target-unit.cx` | `95fbdecbcea43409b46073fac69095edebbeb9204602c0e29ed5a8ef6060634f` |
| `check-cx-separate-use.cx` | `tools/check-cx-separate-use.cx` | `94bc01465ffe62489b2296606f4399f13d96ee081ef7f8b9bf8e43834c8cefc4` |
| `check-cx-unit-graph.cx` | `tools/check-cx-unit-graph.cx` | `ffb7f9b6202dec56e616fc90e3804e93fc6045290df2ba8e19dd66939a339492` |
| `check-driver-target-long.cx` | `tools/check-driver-target-long.cx` | `3e2425f00de895d65c9a74372c8715e0b68e358fd1c2a051145d490876d88ee8` |

**更正（2026-11-02）**：`tools/precommit.cx` 是 `.git/hooks/pre-commit` 的运行时入口，已由 cc 还原到 `tools/`，不在本归档。归档前必须扫描 `.git/hooks`、CI、systemd、cron 与脚本入口。
