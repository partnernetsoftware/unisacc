# 符号索引 tools/symindex.cx：设计（2026-10-30，cc）

## 要解决的问题

批量工具（refactor sites、find、rename，affected，以及接下来的 use-lint 和单元缓存规划）每次查询都要把整仓 .cx 重新词法扫描一遍：1030 个文件、12.8 MB，`sites --tracked` 一次大约 5 秒。多轮批量调整时，同一份没改过的文件会被反复扫描。

目标是给每个文件预先算好事实，并按内容寻址缓存。仓内没有改动时，查询只需 stat 一遍；改了几个文件，就只重新解析这几个。

## 身份与失效（主人提示：按时间戳预制，但不能拿时间戳冒充内容身份）

1. **清单 manifest**：每个被跟踪的 .cx 一行，记录 `path size mtime_ns ino dev sha256`。
2. **更新时逐个文件 lstat**：
   - (size, mtime_ns, ino, dev) 全部不变：沿用清单里的 sha256，不读文件。这一步是时间戳预筛。
   - 任何一项变了：读文件、重算 sha256。
3. **事实分片按内容 sha256 寻址**：`out/symindex/v1/<sha256>.tsv`，`v1` 是索引器的格式版本。内容相同，分片就相同，与路径无关。分片缺失时才解析并写入。
4. **`--verify`**：不信任时间戳，全部重算 sha256，和清单比对；不一致就报 `STALE` 并修复。AGENTS.md 要求"缓存命中仍核内容"，`--verify` 就是这一条的执行入口。日常 `update` 只做第 2 步的预筛，代价是理论上可能漏掉同大小、同 mtime 的改写。
5. **分片里只存与路径无关的事实**：use 只存原文字符串。解析到哪个文件取决于所在目录的链接，在查询时用 realpath 现算。
6. **符号链接**：git 模式为 120000 的跳过，与 cxraw 等棘轮一致，事实只记在真实路径下。

## 每个文件的事实（分片内容，TSV，每行一条）

| 记录 | 字段 |
|---|---|
| `M` 模块头 | name（无模块头时不写这一行） |
| `U` use | 原文，例如 `text.fmt` 或 `heap` |
| `D` 顶层函数定义 | name, line, static 0/1, variadic 0/1, 返回类型文本, 参数文本 |
| `P` 顶层原型声明（以 `;` 结束） | 同 D |
| `T` typedef、struct、union、enum 名 | name, line |
| `X` `#define` 宏 | name, line, 是否带参 |
| `C` 调用 | name, line（标识符后紧跟 `(`，不在注释、字符串、字符字面量里，也不是 D/P 本身） |

词法规则与 refactor.cx 的 find_file 相同：跳过注释和字面量，预处理行只取 `#define` 名和 `#pragma unisa`。

D/P 的判定：在花括号深度 0 处出现 `IDENT ( … )`，括号配平，后面是 `{` 记为 D，是 `;` 记为 P。`IDENT` 前面的词元串就是返回类型文本。if、while、for、switch、return、sizeof 这类关键字排除。参数里出现 `...` 记 variadic=1。

## 查询（全部只读索引，先自动做一次增量 update）

- `update [--verify]`：输出 `SYMINDEX files=N reparsed=K hashed=H elapsed_ms=T`。
- `def NAME`：列出定义处，`FILE:LINE` 加 static、variadic 和原型。
- `calls NAME[,NAME...]`：输出格式与 `refactor.cx sites` 一致，可以直接替换。
- `uses FILE` / `users FILE`：正向和反向的 use 闭包（按 realpath）。affected.cx 以后可以改用这个。
- `undeclared (FILE... | --tracked)`：use-lint。某个调用的函数在仓内某个模块里有非 static 的 D，但这个模块不在调用方的直接 use 集里，调用方自己也没有该函数的 D/P，就报一行。variadic 或返回 double/float 的函数标 `DANGER`（plans/indirect-proto-2026-10-30.md 里那种会被调错的情况），其余标 `INFO`。
- `dump FILE`：输出这个文件的全部事实，用于调试。

## 验收

1. **差分**：`calls NAME --tracked` 的结果集合，与 `refactor.cx sites NAME --tracked` 完全相同。至少比对 strcmp、tx_eqz、sy_spawn_pipe 三个名字。
2. **性能**，分别记录冷、热、改一个文件三种情况：
   - 冷建（清空 out/symindex），预计几秒；
   - 热查询（无改动）< 300 ms；
   - 改一个文件后的增量 update 只重解析这一个文件。
3. **`--verify`**：先人为制造一次同大小、同 mtime 的改写（写入后用 touch -d 还原 mtime），再跑 `--verify`，必须报 STALE，并且修复后查询结果正确。
4. **夹具**：tools/symindex/fixture/ 下放几个 .cx，覆盖 static、variadic、double 返回、宏、注释里的假调用、字符串里的假调用、函数指针参数、多行原型。tools/check-symindex.cx 对这些做精确断言，用 tools/register.cx 登记成批次。
5. **`undeclared`**：对 out/indirect-proto 的复现报 DANGER（tx_snprintf、tx_strtod）；对全仓 `--tracked` 的结果如实记录。

## 不做的事

- 不做类型检查，不展开宏，不跟踪 `#include`。L0 的 `#include` 由 cxinclude 棘轮管。
- 不缓存任何测试结论，只缓存词法事实。

## 实现结果（2026-10-30，cc）

实现在 tools/symindex.cx，检查是 tools/check-symindex.cx（batch `symindex-tool`，19 项全过），夹具在 tools/symindex/fixture/。

与设计的差异：

- **mtime 只有秒**：sy_stat_t 只带秒，清单列名是 `mtime_s`。同一秒内同大小的改写，或者改写后把 mtime 拨回去，都能骗过预筛，只有 `--verify` 能发现。如果以后要纳秒，得改 cx/sys/fs.cx，这不归本工具。
- **分片字段**：D/P 是 `name line site static variadic ret tail`，C 是 `name line site tail`。`tail` 是名字之后到配对 `)` 的原文，带转义。`site=1` 表示 refactor sites 也会列出这一处：括号里没有注释，参数不超过 9 个。`calls` 输出 C、D、P 中 `site=1` 的行，所以和 sites 完全相同。sites 也会把定义和原型列出来。
- **C 关键字不记**：if、while、sizeof、int、double 等后面跟 `(` 时不写 C 行。sites 会列这些，但不会有人按关键字查调用。`#define F(x)` 的 `F(` 记为 C，与 sites 一致；undeclared 遇到调用方自己定义的宏会跳过。
- **行号**：照搬 find_file 的规则，字符串里反斜杠续行不计行。这是为了和 sites 逐行相同；两者在这一点上同样偏一行。
- **测试用的选项**：`--index DIR` 换索引目录，`--in LIST` 用列表文件代替 git ls-files。命令行点名的文件会补进集合，比如 out/ 下的复现。
- **undeclared 的 reach 字段**：`transitive` 表示定义模块在调用方的 use 闭包里，就是 indirect-proto 那种会被调错的情况；`none` 表示经 use 根本看不到，通常靠 libc、`#include` 或别的链接集合拿到。每个调用方加名字只报一行，带 sites 计数。有 DANGER 时 rc 为 1。
- **期限**：update 到 45 秒就停，打印 `DEFERRED n`，已做的部分写进清单，查询返回 3。实测冷建 2.4 秒，用不上。

实测（本机，1000 个跟踪 .cx；含 unisacc.com 启动约 110 ms）：

| 情形 | SYMINDEX 行 | 总耗时 |
|---|---|---|
| 冷建（rm -rf out/symindex） | files=1000 reparsed=994 hashed=1000 elapsed_ms=2421 | 2529 ms |
| 热，无改动 | reparsed=0 hashed=0 elapsed_ms=21 | 124–132 ms |
| 热查询 `calls NAME --tracked` | 同上 | 244–264 ms，读约 9.6 MB 分片 |
| 改了一个文件（tools/symindex.cx 本身） | reparsed=1 hashed=1 elapsed_ms=32 | 工具本身要重编，另计 |
| 只 touch 一个文件 | reparsed=0 hashed=1 | 内容没变，分片照用 |

差分（排序后逐行 diff 为空）：strcmp 4354 = 4354，tx_eqz 1920 = 1920，sy_spawn_pipe 36 = 36。另外 9 个名字合查 16755 = 16755。

`--verify`：check 在 out/ 的副本上用同一 inode 重写（大小不变，mtime 用 sy_set_mtime 拨回）。普通 update 得到 hashed=0，分片还是旧事实；`update --verify` 打印 `STALE`，统计为 reparsed=1 stale=1，dump 出新事实。用 sed -i 改写会换 inode，普通 update 就能发现。

undeclared：

- out/indirect-proto/user.indirect.cx 报 DANGER tx_snprintf（variadic）和 tx_strtod（double），reach=transitive。
- 全仓 `--tracked` 扣掉本工具夹具后：DANGER 1 条，是 apps/csih/fuzz/fuzz_json.cx 调 jnum（double），reach=none，属 csih 车道；INFO 1037 条，其中 reach=transitive 67 条：apps/ 39、archive/ 15、tools/ 8、tests/ 3、src/ 2。INFO 里 reach=none 有 970 条，主要是 setpgid、clock_gettime 这类 libc 同名函数，以及 archive/ 的旧代码。这些都没有改。
