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

| `cxproc_ask.cx` | `tools/cxproc_ask.cx` | `8a3292780160963d2d031749edc7dbe1f346b2b0056db631ff30835b2f1c26d9` |
| `driverdiag-candidate-install.cx` | `tools/driverdiag-candidate-install.cx` | `00351b53a773ff1bf612bc115ff3155c292190d8730a0efbf4cc30cf7b940402` |
| `fifo-candidate-install.cx` | `tools/fifo-candidate-install.cx` | `c4e35c81e85aae6666b94b45416b8310733a2a429e75bfcd5bac6a70bc9d49f6` |
| `fsize-exec.cx` | `tools/fsize-exec.cx` | `889b38cbf0917c6fe88c56102e013a0d4e2154eebe86ccf9c956c609098e3f40` |
| `output-file-cases.cx` | `tools/output-file-cases.cx` | `8aed0391123c1cbbe5f58e4fd45d70b95de8210746950876a24c71f2de3d1157` |
| `p3-member-full.cx` | `tools/p3-member-full.cx` | `5161aa7d709894f136e6619624134f8b73fe56e4bdd9fbeb672aa3260d2b0c7f` |
| `pp-built-private.cx` | `tools/pp-built-private.cx` | `96aae0e26b38e50c40385b429b7a56a229e43c54158d9374de17a4baf507841e` |

**更正（2026-11-02）**：`tools/precommit.cx` 是 `.git/hooks/pre-commit` 的运行时入口，已由 cc 还原到 `tools/`，不在本归档。归档前必须扫描 `.git/hooks`、CI、systemd、cron 与脚本入口。
