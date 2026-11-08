# W2 准备:cx/sys → ext/sysx 的调用点清点与分批方案(只读,2026-11-02)

> 依据 `plans/cx-tiers-2026-11-02.md` 与 `archive/plans/2026-11-08b/layering-refactor-2026-11-02.md`。
> 范围:`net` `tty` `poll` `guard` `fd` `process` `process_group` `process_group_osx` `reaper` `childdepth`。
> **`ffi`、`dl`、`dlcall` 属 core,不迁**(cc 2026-11-02 更正:`ffi` 是 cx 对接外部世界的核心能力);
> `sig` 另议。实际清单是 **10 个模块**;cc 文中所说 11 个,差项待确认。

## 0. 清点口径(重要:不这么算会得到假数字)

仓里有三类会**放大**计数的结构,清点前必须排除:

1. **每目录一个 `sys` 符号链接**(14 个:`src/sys`、`tools/sys`、`tools/cxlib/sys`、`tests/sys`、`tests/cxlib/sys`、`cx/runtime/sys`、`cx/driver/sys`、`cx/workspace/sys`、`apps/csih/sys`、`release/ci/sys` 等)。
   于是同一个 `cx/sys/process.cx` 会以 `src/runtime/sys/process.cx`、`tools/sys/process.cx` … 的形式被重复命中。
   **按 realpath 去重**,并跳过路径中含 `/sys/` 与其等价的链接路径。
2. **`apps/csih/probes/**` 递归副本**(`probes/csih/probes/…` 多层),会把 net 的计数从 9 放大到 697。清点与改名都要排除。
3. `tests/preprocess/**` 是另一棵树,单独计数。

按此口径得到的结论:**11 个模块在产品里引用数全部为 0**,没有需要标红的项。
先前脚本报出的 `reaper: product=5` 是第 1 类假象(同一个 `cx/sys/process.cx` 经五个 `sys` 链接各命中一次)。

## 1. 清点结果(`#pragma unisa use` 行数;`ffi` 已按更正移出本表)

| 模块 | 产品 | tools | tests | apps/csih(不含递归副本) |
|---|---|---|---|---|
| net | 0 | 4 | 0 | 9 |
| tty | 0 | 1 | 0 | 1 |
| poll | 0 | 4 | 1 | 3 |
| guard | 0 | 1 | 0 | 1 |
| fd | 0 | 1 | 0 | 0 |
| process | 0 | **23** | 0 | 0 |
| reaper | 0 | 2 | 2 | 0 |
| childdepth | 0 | 5 | 0 | 0 |
| process_group | 0 | 0 | 0 | 0 |
| process_group_osx | 0 | 0 | 0 | 0 |

代表性调用点:

```text
net         tools/check-cx-sys-net.cx:17  tools/check-cx-sys-spawn.cx:9  tools/check-cx-sys.cx:12  tools/tls13-get.cx:20
tty         tools/check-cx-sys-net.cx:20
poll        tools/affected.cx:38  tools/check-cx-sys-net.cx:18  tools/entry-build.cx:29  tools/release-build.cx:42  tests/cx-host-run.cx:14
guard       tools/check-cx-sys-guard.cx:5
fd          tools/check-cx-sys-fd.cx:1
process     tools/affected.cx:44  tools/check-core-execute-real.cx:1  tools/check-cx-library-build.cx:4  …(23 处)
reaper      tools/cxlib/e2stream.cx:50  tools/cxlib/netpack.cx:43  tests/cxlib/netpack.cx:43  tests/preprocess/cxlib/e2stream.cx:50
childdepth  tools/check-childdepth.cx:32  tools/check-cx-fmt.cx:10  tools/check-cx-sys-net.cx:12  tools/check-cx-sys-sig.cx:10  tools/check-cx-sys.cx:7
```

## 2. 一处**设计冲突**(迁移前必须解决,不是改名问题)

`cx/sys/process.cx:7` **uses `reaper`**。process 属 P0 核心契约,reaper 要进扩展包——
那就成了"核心依赖扩展包",与 `cx-tiers` 的"core 不依赖扩展包"直接冲突。两条出路:

- a) `reaper` 不进 sysx,留在 `cx/sys`(它本质是子进程回收,属 OS 最小能力的一部分);
- b) 把 process.cx 对 reaper 的依赖拆掉(process 只提供原语,reaper 能力上移),成本更高。

**建议 a**,并请 cc 在 W2 开工前裁定;否则这一项会让整批改名返工。

## 3. 改名映射

| 现 | 后 |
|---|---|
| `#pragma unisa use sys.net` | `#pragma unisa use sysx.net` |
| 同理 `sys.tty` `sys.poll` `sys.guard` `sys.fd` `sys.process` `sys.childdepth` | `sysx.*` |
| `sys.process_group` / `sys.process_group_osx` | `sysx.*`(无 `use` 调用点,仅字符串路径,见 §5) |

文件移动:`cx/sys/<m>.cx` → `cx/ext/sysx/<m>.cx`(保持 `#pragma unisa module <m>` 不变,模块名不跟着路径变)。

## 4. 结构前提:14 个 `sys` 链接要有 `sysx` 兄弟

模块解析是**按使用者所在目录**找 `use` 的第一个分量:`tools/x.cx` 的 `use sys.net` 解析成 `tools/sys/net.cx`。
所以把模块搬到 `cx/ext/sysx/` 之后,**每个消费目录都要有 `sysx` 链接**,否则 `use sysx.net` 一律找不到:

```text
src/sysx  tools/sysx  tools/cxlib/sysx  tests/sysx  tests/cxlib/sysx
cx/runtime/sysx  cx/driver/sysx  cx/workspace/sysx  apps/csih/sysx  release/ci/sysx  …(共 14 处,与现有 sys 链接一一对应)
```

建链接前先跑 `readlink -f <dir>` 与 `ls -d <dir>/../cx/ext/sysx`,确认父目录不是链接、目标存在(自指环教训)。
链接用 `commit-own` 提交,提交后 `git ls-tree` 核对类型为 `120000`。

## 5. 不能机械替换的调用点

| 种类 | 位置 | 处理 |
|---|---|---|
| **字符串里的文件路径** | `tools/cx-driver-build.cx:48` 的 sources 数组含 `"cx/sys/process_group_osx.cx"` / `"cx/sys/process_group.cx"` | 手改路径,不能靠 rename |
| 同上 | `tools/check-osx-pgroup-link.cx:15` 的 names 数组 | 手改 |
| 同上 | `tools/check-process-group-osx.cx:8,17`(fixture 路径与错误串) | 手改 |
| **宏/字符串里的模块名** | `cx/sys/*.cx` 之间的 bare `use reaper` 等 | 只在 §2 决议后才动 |
| **递归副本** | `apps/csih/probes/**` | 一律排除,不参与改名 |
| **tsv 里的路径** | `tests/cxraw-layers.tsv`、`tests/cxapis.tsv`、`tests/cxlibc.tsv` 等按路径记录 L0 豁免/债务 | 移动后必须同步,否则棘轮会把新路径当"新增债务" |

## 6. 分批方案(每批 ≤40 个文件)

顺序按"依赖少→多",每批一次小提交:

| 批 | 内容 | 文件数(估) | 验收 |
|---|---|---|---|
| B0 | 建 `cx/ext/sysx/` + 14 个 `sysx` 链接(模块先不搬) | 链接 14 + 目录 1 | 全仓仍可编译(`-c -funit` 抽查 5 个消费文件)、棘轮 7/7 |
| B1 | `fd` `guard` `tty`(仅 tools 各 1 处) | 3 模块 + 3 文件 | affected 相关批 + 棘轮 + 字节等价 |
| B2 | `net`(tools 4 + csih 9) | 13 文件 | 同上;csih 车道需先发信封 |
| B3 | `poll`(tools 4 + tests 1 + csih 3) | 8 文件 | 同上 |
| B4 | `childdepth`(tools 5) | 5 文件 | `cx-childdepth` 批 + 棘轮 |
| B5 | `process`(tools 23 + 字符串路径 3 处) | 26 文件 | affected + 棘轮 + 字节等价 |
| B6 | `reaper`(tools 2 + tests 2)——**取决于 §2 决议** | 4 文件 | 同上 |
| B7 | `process_group*`:仅字符串路径 | 3 文件 | `check-osx-pgroup-link` / `check-process-group-osx` |

每批验收统一为:目标文件可编译 + `tools/affected.cx --files … --run` 相关批 + `tools/ratchets.cx` 7/7 + 同输入 `-o` 字节等价。

## 7. 风险

- **解析面最大的是 B0**:链接少一个,就有一整棵消费树编译失败。B0 必须单独一批、单独验收。
- `process` 的 23 处里有多处是**构造链工具**(entry-build、release-build、affected),它们本身也在被改;两者顺序要与 cc 对齐。
- 字符串路径(`process_group*`)改名工具看不见,漏改的表现是"文件找不到",不是编译错误。

## 8. B3 判定清单(cx/sys 剩余模块,实测引用数)

口径同上(realpath 去重,排除 `apps/csih/probes/**`)。`use sys.X` 的引用文件数:

| 模块 | 产品 | tools+tests | csih | 判定 |
|---|---|---|---|---|
| `dl` | 1 | 1 | 0 | **core 不动**(cc 裁定) |
| `dlcall` | 0 | 1 | 0 | **core 不动**(cc 裁定) |
| `ffi` | 0 | 1 | 2 | **core 不动**(cc 裁定,FFI 是核心能力) |
| `env` | 6 | 54 | 23 | P0,留 `cx/sys` |
| `fs` | 7 | 118 | 26 | P0,留 |
| `io` | 2 | 149 | 42 | P0,留 |
| `kernel` | 1 | 1 | 0 | P0,留 |
| `mem` | 1 | 1 | 0 | P0,留 |
| `posix` | 1 | 13 | 10 | P0,留 |
| `proc` | 1 | 81 | 12 | P0,留 |
| `sha` | 2 | 7 | 0 | P0,留 |
| `time` | 1 | 30 | 9 | P0,留 |
| `sig` | **0** | 51 | 2 | **待裁定**,见下 |

### `sig` 的两面

- 按"产品 0 引用即迁出"的规则,它符合条件;
- 但它被 `ext/sysx` 里已经搬过去的 `childdepth`(以及 `process`/`reaper` 一族)依赖,
  且信号处置与 io/fs/proc 同属"最小平台契约",**产品 0 引用只说明产品不装处理器,不说明它不是契约**。

**建议:留 `cx/sys`**,理由:①它是 `signalfd`/`sigaction` 这类最小平台能力的唯一出口;
②搬进 sysx 会让 sysx 内部出现"包依赖包"的自我闭包,而没有产品收益;
③51 个 tools/tests 调用点换来的只是路径变化。
若 cc 裁定迁出,则按 B1 的同法(先查豁免、计数逐项相等、csih 不动)执行。
