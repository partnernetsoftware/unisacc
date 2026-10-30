# 同一份 L0 不能编进两只产品对象（2026-10-30）

## 事实

- `tools/entry-build.cx` 用 `-c -funit` 编每一只对象。闭包里的函数体进这只对象，符号是全局的。
- `out/grk-iproto/drive.o` 已经导出 `tx_eqz`、`tx_len`，以及 `sy_fstat`、`sy_close`。全目录 `nm` 里 `tx_eqz` 只有这一处。
- `path.cx`、`compat.cx` 若再 `use text.text`，下一只 `path.o` / `compat.o` 会再带一份 `tx_*`，和 `drive.o` 链接时重复。这一刀没有改这两个文件。
- 单独编成一个程序时不是这个问题。手改过 `use text.text` 的那段里，`tools/check-driver-path.cx` 在已装产品上 rc=0（检查器自己不 `use text.text`）。`tools/check-driver-compat.cx` 自己 `use text.text`，已装产品报 `TX_ZCAP defined twice`；候选 `4ac3c2d4` 按 realpath 折成一个，rc=0，`CXC failed=0`。那是一个程序，不是 33 只对象。这两个源文件已经还原，现在仍是 `strcmp`。上面两次绿不属于现在的源。
- 单元缓存省的是重编，不把同一份代码从两只对象里拿掉。要让每只产品对象都能 `use` 同一份 L0，得先让每个模块只进一只对象。

## 这一刀实际改的

- `src/drive.cx` 本来就 `use text.text`。两处 `strcmp` 改成 `tx_eqz`：`gap` 对 `"e2"`，输出路径对 `"-"`。`-cx-plan` rc=0。
- 重编 `drive.o` 再链接。候选 `out/grk-iproto/candidate`，3025940 字节，sha256 `1026a5c5b31e76590c983264029aba80879d773eb3479acc2a5aee58cd2737ce`。没有安装。
- 间接复现仍 rc=1，点名 `user.cx` 和 `tx_snprintf`。直接 use 仍输出 `[ab-42] n=5 num=2.250000`。runcache 12/0（冷 556 ms，命中 41 ms，目录 `out/grk-iproto-rc6`）。这只候选是当时链出来的。后面的提交（例如 input 改走 `sy_fstat`）不在这只里面。
- 这一会话用已装产品再编了一次单文件：`sh ./unisacc.com -O1 -c -b lnx/x86_64 -funit -o /tmp/grk-drive-check.o src/drive.cx` 退出 0，对象 1326376 字节，导出 `tx_eqz`。这不是 33 只链接，也没有换掉上面那只候选。

## 没有 main 的 `-c`

已装产品 sha256 `79fb84e65a10b55ce40be5f68746748acbd38f441944046c0a5dfa5718bc0c32`。

- 不带 `-funit`：没有 main 就拒绝。`cx/driver/path.cx:55` 报 `not covered: no main`，退出 1。`apps/csih/tui.cx` 现在 28 条 use，`-cx-plan` 退出 0；同一条 `-c` 退出 1，错在 `apps/csih/sys/guard.cx:36` 同一句。和 use 条数无关。apps/csih 归 csih 车道（unisacc-00），这里不改那些文件。
- 带 `-funit`：`cx/driver/path.cx` 退出 0。对象里没有 `tx_eqz`，只有 `cxp_*`。
- sweep 规定的检查是不带 `-funit` 的 `-c`。改之前检查失败，refactor 把文件记成 SKIP，不改（`tools/refactor.cx`：失败的基线证明不了改动）。这条命令不会改 `path.cx`、`compat.cx`。没有跑 sweep。
- 产品清单里的 `path.o`、`compat.o` 就是 `cx/driver/path.cx`、`cx/driver/compat.cx`（`src/driver` 指到 `cx/driver`，同一 inode）。再 `use text.text`，`-funit` 会把 `tx_*` 再编进这两只对象，和入口 `drive.o` 里已有的 `tx_eqz` 重复。所以不把这两处改成 accept。
- 为了让模块找得到 `text.text` 而加的 `cx/driver/text` 软链会让已装产品把同一模块算两次。sweep 不做，这条软链删掉。

## cli.cx 维持 static（bdy 对齐，2026-10-30）

- `cx/driver/cli.cx` 继续用模块内 `cxc_eqz`、`cxc_prefixz`，不 `use text.text`。这是当前做法。等每模块只进一只对象之后，再改成 `tx_eqz`、`tx_prefixz`。
- 当场核对：`cx/driver` 旁边只有 `sys`、`mem`，没有 `text`。`cli.cx` 没有 `use text.text`。
- `plans/cx-layers.md` 的收敛清单由 bdy 改。这份计划不代改那个文件。
