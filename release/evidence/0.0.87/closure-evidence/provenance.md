# 0.0.87 构造闭包来源

整链输入提交 `ee595061e7d24ccebd034e22769b66e41d5e2c46`，会话为 `unisacc-session-v087-chain1`，快照标识 `ee595061e7-88f69af3`。六个归一化 TSV 每格记录 99 个仓内真实路径与其构造来源。归一化消解了当次快照的绝对路径和仓内符号链接。

Linux 两格从快照的 `build/chain-facts.tsv` 获取 44 个根，与会话的 `fixedpoint/g3/<目标>.closures.tsv` 合并，并逐项确认目标目录里的根对象和闭包对象存在。macOS 和 Windows 四格来自快照的 `out/chain/gen3/<目标>-front0/part.tsv`。这里记录 use 路径成员，当前表的字节数来自生成时工作树。

原始材料 SHA256

| 材料 | SHA256 |
|---|---|
| 快照根事实 | 8999f5696d71a58780ac4c8e1ea9de51601c9791222f42b739f476cb888f5cf5 |
| 两格 Linux 的 closures.tsv，各自 | 673a393cecc35cbd98543094e5401c54a1ee72482cccc2156671f18e55db3339 |
| 四格 macOS 和 Windows 的 part.tsv，各自 | 74409cd7da1476ff6bda652abc9f73633e6e4f45bdf45f08985ff145eb6b5c5d |

归一化材料 SHA256

| 目标 | SHA256 |
|---|---|
| lnx-arm64 | 78ef3e6bfd643932a559ae09a30c14d7dcd1557926bbb7c0920c687f12e64e2b |
| lnx-x86_64 | c9942b736d1b26528515cb47cbac336124616b8354c44fefcdb2576192600842 |
| osx-arm64 | 20fc53feb7055a4e768f2457cea15718880605b17746f06258aa10815fa0d450 |
| osx-x86_64 | e1025b20da24ae6dccd994dfc7b98e228ce01c4a90bb7cd3aeb47da785402432 |
| win-arm64 | 0de15f706c891e2453f559acc18d0c53a38f7d316704ef315d4cb36c133732b1 |
| win-x86_64 | 2992d65a944fce0037283f13d87680d49c4280454ec7e6e32540165e15b5b4de |
