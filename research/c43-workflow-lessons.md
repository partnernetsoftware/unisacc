# 0.0.43 多代理工作流的经验（2026-10-23，cc）

> 给本仓所有代理（cc、cc2、grok、hermes…）读的运行纪律。来源：同日两次清空事故 + 一次险情 + 一天的并发波次实测。

## 1. 安全（硬规则）

| 教训 | 规则 |
|---|---|
| 05:22 代理在活检出里对 `exec/parse2/warnprep.py` 做参数模糊测试，`--tools=` 变成 `Path('')`=`.`，`shutil.rmtree` 清空了整个检出（含 `.git`）；约 30 个未推送提交只存在于那里 | 预言机、移植、模糊、变异只在 scratch 导出（`git -c tar.umask=022 archive HEAD \| tar -x`）里跑；任何工具都不得收到 `''`、`.`、`/`、仓库根、`$HOME` 作路径参数 |
| ~10:00 `~/.claude/projects/-home-devali-repos`（记忆、代理日志）被清空，原因未证实，当时正在跑删除守卫的竞态/攻击测试 | 一切破坏性测试（攻击、竞态、会删文件的变异、写/删型工具的参数模糊）只在隔离命名空间里跑：`unshare -rm` + 用空 tmpfs 遮住 `~/.claude ~/repos ~/backups` 及其他代理状态 |
| 08:05 代理把 scratch 的整份 `tests/gate.sh` 覆盖到活检出，抹掉别人未提交的行 | 共享文件（gate.sh、gatelayers.py、stagemap.c、INVENTORY.md、gatedeps.json、zeropython.tsv）只用 `git apply` 打自己的补丁；有别人未提交的块就等 |
| 未推送的提交和记忆都只有一份 | 每批落地后 `git bundle --all` 到本机备份目录；记忆目录同样备份；push 仍归主人 |
| 删除调用散落各处 | 唯一守卫 `include/unisa_safedel.h` + 台账 `tests/deletes.tsv` + `deletecheck` 闸（政委裁定） |

## 2. 过程

| 教训 | 做法 |
|---|---|
| 代理"等后台任务"时结束回合、再也不回来（出现两次），落地做一半 | 简报写明：自己起的后台任务没完不得结束回合；真发生时，新代理带"防重复落地"检查（先看提交、再拿锁目录）从 scratch 状态接手 |
| 验证者一驳回，整个工作流就停 | 每一步允许 3 轮"实现→独立验证"，验证意见回灌下一轮 |
| 文本扫描式判据会被变形数据绕过（W6 B1 三轮被 X-macro、enum 矩阵、`A3->F`、网格绕过） | 以行为与钉住判定：扰动注入证明 + 文件清单哈希；文本扫描只作启发 |
| 多个落地者 `sleep` 轮询共享文件、互相等待 | 已做：单一落地锁（`flock` 于 `.git/` 下的锁文件），按序排队，见本表末行 |
| 套件在 scratch 跑一遍、commitgate 再跑一遍 | 下一步：输入字节同一性可证时复用 scratch 结果；与 `tests/cxbin` 编译缓存一起做 |
| commitgate 的单批 60 s 上限常被大套件集撑爆（rc 142） | 每次提交只点 5–7 个关键套件，其余在 scratch 跑绿并写进提交信息 |
| 代理在 scratch 里仍用 Python 改文件 | 禁止；改文件用 Edit、awk/sed 或小 `.cx` |
| 工作流脚本只存在 `~/.claude` 下，随清空丢失 | 脚本同时存为 scratch 文件，按路径启动 |
| 落地者等别人未提交的行：共享文件里有他人的块时，`commitgate -- 路径` 会把他人的行一起提交，只好 `sleep` 轮询 | 已做（0.0.43）：`release/tools/commitgate.sh --index` 只提交索引里的内容（落地者只放自己的块：`git apply --cached P` + `git apply P`，新文件 `git add`）；索引里有声明路径之外的改动即拒（rc 2，逐个列名），闸门在索引树的导出工作树（`tests/landlock --export`）里跑，派生检查读索引。所有 commitgate 共用一把落地锁 `tests/landlock`：`flock` 于 `$(git rev-parse --git-common-dir)/unisacc-landing.lock`，最多等 `COMMITGATE_LOCK_WAIT` 分钟（默认 30，超时 rc 6，提示持锁者），在 `tests/landlock` 之下可重入。派生文件在锁内从索引树重算：`tests/landlock --derive` = 在 `X=$(tests/landlock --export)` 里依次 `sh ./unisacc.com -run tests/inventory.cx -- --write`、`sh ./unisacc.com -run tests/refreshgatedeps.cx -- --index`、`sh tests/cxbin tests/zeropython.cx --write`，再 `tests/landlock --add tests/INVENTORY.md tests/gatedeps.json tests/zeropython.tsv`（导出字节进索引，同一改动三方合并进工作树，他人未提交的行保留）。整套一条：`tests/landlock sh -c 'git apply --cached P && git apply P && tests/landlock --derive && release/tools/commitgate.sh --index -m MSG --suite S -- 路径… tests/INVENTORY.md tests/gatedeps.json tests/zeropython.tsv'` |

## 3. 吞吐

- 重活错峰：几个工作流同时跑预言机和重建时负载到 33，门禁会假超时；产品重建与全量差分不要叠加。
- 产品字节改动合批：同一重建周期里落多个 `fix:`（重建、重封、身份核验只做一次）。
- 并发上限 3 个工作流；同一共享文件同一时刻只能有一个落地者在改。
