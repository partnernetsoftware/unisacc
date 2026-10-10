# 0.0.40 gqalive macOS Python.app argv0 大小写（机房主任 21:36 阻塞优先）

**状态（2026-10-10）**：修复入仓；Linux 与 m4pro 夹具绿；不改产品验收、不删 gqalive 的 exit 2、不用 `GQALIVE_TABLE=ps` 绕过。

## 根因

`release/tools/gqalive.py` 的 `PY=re.compile(r'^python[0-9.]*$')` **区分大小写**。macOS Homebrew 经 `ps -axo pid=,args=` 看到的 argv0 是

`/opt/homebrew/Cellar/python@…/Resources/Python.app/Contents/MacOS/Python`

basename = `Python`，不匹配 → 活着的 `python3 … gatequeue.py` 被判无 → `gqalive` rc 1 → `queue.sh` fail-open 开窗。

## 改动

1. `PY` 加 `re.I`（忽略大小写）。
2. `tests/gqalivecheck.sh` 补 Python.app 形式 argv0 夹具（全路径 / `Python` / `PYTHON3.14` 正例；`notgatequeue.py` / `Pythonista` 负例）。
3. 顺手：`queue.sh` 等 gatequeue 存活越界后改为 **exit 2 并保留状态**（不再 fail-open 开窗）。

## 实测

| 宿主 | 命令 | 结果 |
|---|---|---|
| box (Linux) | `./tests/gqalivecheck.sh` | rc 0 |
| box (Linux) | `./tests/queuetimeoutcheck.sh` | rc 0 |
| m4pro tip（修前） | `./tests/gqalivecheck.sh` | **rc 1**：`proc: running gatequeue.py not found (rc 1)` |
| m4pro（修后） | `./tests/gqalivecheck.sh` | **rc 0** |
| m4pro（修后） | 实起 `python3 -u …/gatequeue.py`，`ps` argv0 为 `…/Python.app/…/Python`，`gqalive.py` | rc 0，HIT 该 PID |
| m4pro（修后） | `./tests/queuetimeoutcheck.sh` | rc 0 |

未做：真实 release queue 首窗观察；BSD `ps` 空白切分 argv 非无损（既有边界）。
