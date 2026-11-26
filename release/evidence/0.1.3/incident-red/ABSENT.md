# 红停原件缺失与证据丢失事件(归档侧记录)

## 1. 缺什么

计划中的三份红停原件:

- `~/.cache/unisacc/prerun/v0.1.3-bfdd1448ba.superseded-073824/evidence-red-064216`
- `~/.cache/unisacc/prerun/v0.1.3-bfdd1448ba.superseded-073824/evidence-red-072250`
- `~/.cache/unisacc/prerun/v0.1.3-bfdd1448ba.superseded-073824/evidence-red-073114`

均不在。相关三个目录也不在:`chains/bg-b1.bak-pre-C0c`(含 `bfdd1448b`、`d07d9640c` 两条旧链)、`prerun/v0.1.3-bfdd1448ba.superseded-073824`、`prerun/v0.1.3-d07d9640ca.superseded-092413`;`chains` 与 `prerun` 两个父目录 mtime 均为 `2026-11-26 13:00:11` 本地时间。

## 2. 原因(与现有证据吻合的归因)

- 用户 journal 显示 `system-cleanup.service` 在 `13:00:03` 到 `13:00:13` 运行:
  - `13:00:03` `before: 250G 190G 61G 76%`;
  - `13:00:11` `/tmp and 2h caches cleaned`,同批报 `unisacc stale artifacts cleaned`;
  - `13:00:13` `after: 186G`。
- 该 unit 的 `ExecStart` 指向 moltbaby 的用户清理脚本;脚本第 60-67、79-80 行对 `~/.cache/unisacc` 的 `prerun` 与 `chains` 有同阶段递归清理规则。
- 规则原文要点:对 `~/.cache/unisacc/{prerun,chains/snap,chains,顶层}` 与 `~/.claude/jobs/*/tmp` 的子目录,若自身 `atime` 与 `mtime` 均超过 120 分钟,且内部无 120 分钟内被访问或改动的条目,则 `rm -rf`;排除 `.*`、`*.lock`、`*.sock`。
- 归因限制:`journalctl --user -u system-cleanup.service` 无逐目录删除原行;逐目录原行不可得,据目录 mtime 与源码规则推定。
- 事后处置:已加有界 keepalive `~/.local/share/unisacc/keepalive/keepalive-0.1.3.sh`,每 25 分钟 touch 一次,最多 12 轮;台账在 `15:15:34` 记下该 keepalive 启动。
- 责任面:六窗询问中五窗已声明不是自己;ccogweco 明确回不是我,13:00 前后只做文字稿与私有研究页,未对该缓存执行 `rm`/`mv`/`unlink`/清理脚本,首次读取这两个缓存目录是 14:46 之后。归因对象是用户定时器服务,不是任一协作窗口。

## 3. 剩余可重建的证据(非原件)

- 台账(当前保护库):`~/.cache/unisacc/evidence-vault-0.1.3/chain-ledger-0.1.3.txt`,sha256 `5c3482f8f25d1597235c87456ac3363e049649a8e94fc7bb327bce0c845716be`,2263 字节;比早先摘录多出 `15:15:34` 的 keepalive 一条记录。
- 台账(早先摘录,本次登记先前引用):sha256 `5b05efde3bd0627a8284f52885e7815b9dc0b914b88ddc8b189eaadaba42870b`,2066 字节;与当前版本差异即 keepalive 一行。两版都含 `prerun resume#1/#2/#3` 三行(红停时刻与 `evidence-red-*` 名称)。
- 监督输出:`~/.cache/unisacc/evidence-vault-0.1.3/jobs-outs/prerun4.out`,sha256 `f6eaf3eba4528869accaf38c696df81cd330bb50116103c294d2b6085db14100`;该文件是最终正式轮(预跑 #4)的输出,`PRERUN_STOP` 原始行数为 0,不能用于重建三次红停的 `raw=` 数据行。
- 保护库其余现存旧目录:`~/.cache/unisacc/evidence-vault-0.1.3/`(含 `chains/a0c4fc8285…`、`v0.1.3-a0c4fc8285.bak-pre-C0f-120836`、`v0.1.3-a0c4fc8285.explore`);`bfdd1448b` 与 `d07d9640c` 两链仍无原件。
- 归档侧复制件(明确非原件,仅供重建引用;复制后按包内规则脱敏,家路径替换为 `~/`):
  - `reconstruct-chain-ledger-0.1.3.txt`:早先台账副本;原始 sha256 `5b05efde3bd0627a8284f52885e7815b9dc0b914b88ddc8b189eaadaba42870b`,脱敏后 sha256 `f830aa2ddef5a3ec5666a7756e1d3ab57675785880eec652739985263bc43d4f`,2000 字节;
  - `reconstruct-chain-ledger-0.1.3-current.txt`:当前台账副本;原始 sha256 `5c3482f8f25d1597235c87456ac3363e049649a8e94fc7bb327bce0c845716be`,脱敏后 sha256 `27ff201c5a3754af2451fa0a687af09d7d09ea10302806b9b4ea6636b46d3352`,2197 字节;
  - `reconstruct-prerun4-supervise.out`:监督输出副本;原始 sha256 `f6eaf3eba4528869accaf38c696df81cd330bb50116103c294d2b6085db14100`,脱敏后 sha256 `5bd93fcc5a4b367ce009f55a533798665e647bb93e7cc939d34933fe719a9ec3`,45106 字节。

## 4. 不能声称的事

- 不能声称原件内容或校验和已验证;原件缺失前无 sha 记录。
- 不能声称 journal 有逐目录删除原行;归因是"与现有证据吻合"的推定,不是逐条命令证据。
- 不能声称清理动作的逐目录执行细节、动机或责任窗口已查明。
- 红停原因与次数只按台账与 known-gaps 记录,不据此推断任何 PASS。
