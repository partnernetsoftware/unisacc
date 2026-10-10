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
