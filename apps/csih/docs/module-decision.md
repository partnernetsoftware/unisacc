# 模块化决策（csih，2026-10-10）

状态：采纳并已落地两个模块（csih.cols、csih.home）。不改变 unisacc 工具链。

## 决定

1. 模块边界由**公开头**定义：每个模块一个公开 C99 头（声明的公开集合），一个 `.c` 提供实现。
2. 消费方不用 `#include` 引入自家模块。公开头由 `csih.sh` 通过 unisacc `-include "$ROOT/<头>"` 注入。
3. 提供方 `.c` 也不 `#include` 自家公开头；注入对所有单元生效。
4. 公开头沿用原文件名（如 `csih_cols.h`、`csih_home.h`），因为 `probes/reload_candidate.py` 冻结输入清单引用这些名字。
5. 文件表（`csih.sh`）列出每个模块的 `.c`。
6. 不采用 `.cx` 后缀作为模块标记：unisacc 把 `.cx` 当脚本路径，改名会导致 `no host function`。待 cx 单元模式与清单落地后再议。

## 理由

- 源码 `#include` 会把整文件的静态函数与依赖拉进每个单元，是依赖污染的来源。
- `-include` 在现有工具链上可用，不需要改 unisacc。
- 公开头 + `.c` 对纯 C99 使用者同样可用（同一份头，`cc -std=c99 -pedantic` 编译通过）。

## 已验证

- `./csih.sh selftest` rc=0（unisacc）。
- `cols.c`、`home.c` 在 cc 与 clang 下以相同 `-include` 注入通过 `-std=c99 -pedantic`。
- `csih_cols` 的码点对拍（U+0001..U+10FFFF）与原 static 头输出一致。

## 未做 / 未解

- 剩余约 46 处引号 `#include`：`reload_support.inc`（10）、`tui.c`（9）、`agent.c`（3）、`csih.c`（2）等。其中 `.c` 文本包含（如 `tui.c` 包 `render.c`）需要改成文件表单元，才能去掉。
- 整表 cc/clang 编译仍失败：`reload_state.c` 隐式声明 `rs_upto4095`；`tui.c` 重声明 `mkdir`；`net.c` 依赖 unisacc 专有头 `unisacc_ffi.h`。
- `-include` 是全局注入，没有真正的模块隔离；隔离要等分开编译（`docs/toolchain.md` §7）与接口工件。
- `probes/reload_candidate.py` 为他人未提交改动；其冻结清单仍引用 `csih_cols.h`、`csih_home.h`，本决定保持了这两个文件名。

## 文本 include 转单元：实测阻塞（2026-10-10）

- csih.c 只有两行 include：reload_support.inc 与 tui.c；main 在 tui.c:2697。文件表直接列出 tui.c 与各 reload 单元，unisacc 报：`context_index.inc:3:5: unknown identifier`（cmi_work、csih_message 等类型由 tui.c 之前的文本提供）。
- reload_support.inc 的 `#define set_why checkpoint_set_why` / `#undef` 是两个库各自 set_why 的私有改名；转单元前必须给每个库建公共头并消除非静态重名。
- csih.c 注释称 argv 已到 unisacc 结构 id 上限（netdb.h），textual include 是当时的绕法；转单元前需确认该上限在当前 unisacc 版本是否仍然成立。
- 结论：转单元是逐单元的公共头化工作（每单元：类型与声明入头、去跨单元 static、删改名宏、context_index.inc 改为显式声明），不是改 csih.sh 能完成的。

### context_index.inc 的真实阻塞（2026-10-10 实测）

- context_index.inc 直接使用 csih_message_io.c 的私有实现：`cmi_work`（结构定义在 csih_message_io.c:13）、`cmi_id`、`cmi_path`、`cmi_cleanup`，并自行 calloc/free `cmi_work`。
- 它还依赖 tui.c 的 `tui_state`（tui.c:192）。
- 因此它不是"缺 include"，而是模块边界缺失：一个消费方绕过了 csih_message_io 的接口直接摸私有状态。正确做法是把 mailbox 的只读扫描（列出、预览）收进 csih_message_io 的公共函数，context_index 只调公共接口；这是设计改动，不是机械的文件搬迁。
- 在该改动完成前，tui.c 与 reload_support.inc 的单元化不能完成；这是转单元的前置条件。
