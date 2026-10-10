# 0.0.40 冻结前首观察包（只写入仓；实跑另授）

**状态（2026-10-10 23:3x，机房主任 23:32 自裁）**：只写已齐可入仓；**不** bump、**不**构造、**不**跑首观察。实跑按下文清单、在「冻结获批 + version.h→0.0.40」之后另授。源：仓外 `/tmp/cc40-prep/next/freeze-first-observe-checklist.md`、`closeout-2327.md`、`monotonic-wrapper-stop-design.md`、`ruling-first-observe-scope-2327.md`（三方核过的口径）。

## 门闩结算（入仓时 tip）

| 门闩 | 状态 | 入仓依据（短） |
|---|---|---|
| 快测 WF1 | 通过（限定） | 4ec76cb1；com 产品义务不免 |
| 填尾 WF2 | 未覆盖/顺延 | 7903c89d |
| 等裁空转裁剪 | 缺口已列/顺延 | 151ce46f；非 Stop-hook 首观察 |
| 归档路径预检 | 部分（接线/夹具已证；首观察缺） | 67ea198d、92a96f22 |
| 误 restore | 机制入仓、首窗夹具已证 | f72086f6；非真实 queue 首观察 |
| commitgate | 本地已启用（限定） | 093df19f 等 |
| K2b | 机制入仓，正式白名单空 | 93c8d8ef |
| 分层并发 | 未通过（须具名不依赖/顺延/阻断） | 未测共享冲突读数 |
| 误删防护 | 部分；A7 九删另列 | 不代恢复 |

记录 tip（写包时）：`92a96f22e3986fa854faa63af58f1002f8298d39`；`src/version.h` 仍 `0.0.39`。H1-INHERIT：`archive/plans/v0.0.38.md` sha256=`d1f05f7ddbec20556334630cc0b58bd16a05cad729b0d2661221158e60e35bc9` host=`linux-x86_64-8core-xeon-cloud`。本机 `uname -sm`=`Linux x86_64`，8 核 Xeon（与声明形态一致的云机基线；真核放在实跑窗，只列差）。

## 实跑前置（全部满足才开）

1. 0.0.40 开发/预验完成，授权人批准冻结（含产品/K5 切口已批并达到可冻证据，或该切口明确不依赖项已具名）。
2. 单独版本提交：`src/version.h` → `0.0.40`（freezecheck 起点）；其后产品闭包须 `fix:` 或 waiver。
3. tip 净树；HEAD / origin/main / ls-remote 三者一致。
4. 冻结提交后刷新 gatedeps（refresh≠review）。

## 观察范围（23:27 裁定）

- **只到** `build_candidate` preflight（h1source）→ **第一个 make**；不走完整构造。
- 0.0.39 上禁止构造：**只靠裁定**（bb579567 后 h1source rc0，代码不拦）；代码层版本闸延到「冻结授权 + bump」同批再议。
- 改错声明 sha 负例：**可顺延**，不挡门闩。

## 采集键（实跑时填）

| 键 | 采法 | 备注 |
|---|---|---|
| 树身份 | HEAD、status 行数、version.h | 前后各一次 |
| H1 解析 | 独立 `h1source.py` 同 root；声明目标与 archive38 **前后各算 sha** | resolve 先核 sha 再读表，非原子 |
| preflight→首 make | 外包 monotonic 采点（见下）；stdout「只有顺序」 | step shared 在 make **返回后**打印；目录 mtime 是墙钟，不与 monotonic 混比 |
| argv/env | 结构化保存 | 不打印 token |
| host 实际 vs 声明 | uname/CPU/核数/发行版 vs host=… | 只列差，怎么判由授权人定 |
| 失效键 | source_digest、UA、model、gatequeue stamp、K2b tree_stamp | 白名单空→全树失效核 |
| seed 内存键 | C_SHA / reference_key 等 | 未变不重测；原 UNKNOWN 不因此变绿 |
| exittable | 真实具名 state 上默认与显式 --h1；来源用独立 h1source 绑定 | 92a96f22：显式与声明不一致 rc2；相对路径未实测，只认绝对路径 |
| 子 rc / 日志 sha | 逐步记；不用管道尾 rc | |

## 外包 monotonic 停止设计（已写入；未实现未跑）

详见同批 `research/c40-monotonic-wrapper-stop.md`（从仓外设计迁入）。要点：PATH 前置 `make` 垫片记 monotonic 后 exit 97；不改 `build_candidate`；setsid + 全量 ps/proc 筛 PGID/SID（不用 `ps -g`）；bound 子进程自 setsid，垫片须自记 sid/pgid 再查。rc 97 = 截停预期，非 PASS。

## 不做

不 bump；不构造；不跑窗；不代开全量 queue；不把夹具绿写成真实首观察。
