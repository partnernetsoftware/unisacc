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
