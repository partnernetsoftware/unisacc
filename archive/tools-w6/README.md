# W6 A 档归档（2026-11-02）

来源：`plans/w6-tools-diet-2026-11-02.md` A 档；只做 `git mv`，内容未改。

| 归档文件 | 原路径 | SHA256 |
|---|---|---|
| `check-cx-sys-fs.cx` | `tools/check-cx-sys-fs.cx` | `3bbb5d078ac8942ac8317567cdd860b7ae5bd80ba2cba0749c7188106dbe6184` |
| `deadloop-guard.cx` | `tools/deadloop-guard.cx` | `b99f2777bc945349a02801cf44910efe83c62d0401abaaa4f5d814eeb70b159d` |

说明：`check-cx-sys-fs.cx` 由 `check-cx-sys-*.cx` 覆盖；`deadloop-guard.cx` 仅盘点文档引用。

**更正（2026-11-02）**：`tools/precommit.cx` 是 `.git/hooks/pre-commit` 的运行时入口，已由 cc 还原到 `tools/`，不在本归档。归档前必须扫描 `.git/hooks`、CI、systemd、cron 与脚本入口。
