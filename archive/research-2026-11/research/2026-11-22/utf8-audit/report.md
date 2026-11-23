# tools 字节级切分的 UTF-8 边界审计(omp-sonnet,私有补丁,未提交)

范围:tools/*.cx 与 tools/cxlib/*.cx(已排除 archive/、research/、历史)。方法:git grep 定位固定长度截断(`%.Ns`、`%.*s`、容量钳位、尾部偏移裁剪),读源码判定切点来源,对危险点做真实运行或库级复现。

## 0. 结论

(1) prd.md 第 220 行那个坏字节**不是**"在多字节字符中间切开"。证据:坏字节是 0xDE(2 字节序列的首字节),后面紧跟 ASCII `vsvc`,周围汉字完整。三字节汉字被切断时留下的是 0xE0-0xEF 首字节加至多 1 个续字节,不会是 0xDE。该行引入提交 750e63b581(2026-11-18,wanjo)的 diff 里,同一句其它两处写的是 `` `；devsvc product-map ``,此处文本应为 `` `devsvc ``:字节 `60 64 65`(反引号 d e)被压成了单字节 0xDE。形态是三字节变一字节的替换,属于写入这行文字的那条路径(模型输出或其编辑通道)的瑕疵,不是 tools/ 下按偏移切片的工具造成的。本审计没有找到能产出这个形态的工具。[INFERENCE:未能复现生成该行的原始写入路径]

(2) 但 tools/ 里确有**真实**的字节数截断会切开汉字,且已复现(见 3、4)。edit-span / own-diff / commit-own / refactor / commit-msg 这几个"编辑类"工具本身是安全的(见 2.1)。

(3) 一个通用防线比逐点修更有效:edit-span 已补输出校验补丁(原文件是合法 UTF-8 而产出不是则拒绝),建议同样放进预提交门(agy-flow 正在做的防线)。

## 1. 安全性判据

UTF-8 自同步:合法 UTF-8 的精确子串匹配只会起止于字符边界;0x0A 以及任何 ASCII 字节永远不在多字节序列内部。因此:
- 切点来自 tx_find / tx_chr / tx_prefix 的匹配起止、行边界(0x0A)、ASCII 标记 -> 安全。
- 切点来自固定字节数(`%.Ns`、容量钳位 `cap-1`、`k < 120`、尾部偏移 `size-N`)-> 可能落在续字节(0x80-0xBF)上,危险。
- 坏输入透传(参数里带坏字节)不属于"切",但没有输出校验时工具会原样写入。

## 2. 清单

### 2.1 编辑/改写类工具(判定:切点安全,无需改)

| 文件 | 函数 | 切点来源 | 判定 |
|---|---|---|---|
| tools/edit-span.cx | es_text | OLD 精确匹配起止(tx_same 全长比较) | 安全,但无输出校验(见 E3) |
| tools/edit-span.cx | main(行段模式) | tx_chr 找 `\n` 得行起止 | 安全 |
| tools/own-diff.cx | main/行比较 | 整行(`\n` 分隔)加 KEY 子串匹配 | 安全 |
| tools/commit-own.cx | --keyed 行选择 | 整行加 KEY 子串 | 安全 |
| tools/commit-msg.cx + cxlib/trailer.cx | tr_strip | 整行丢弃 | 安全 |
| tools/refactor.cx | find/apply/rename/inject | 标识符起止(ASCII)、行首插入 | 安全(ASCII 字节永不在多字节内部) |
| tools/notes-fill.cx | 占位替换 | 匹配"待填"的位置,整串替换 | 安全 |

### 2.2 危险点(固定字节数截断;已复现的标 R)

| 编号 | 文件:行 | 切点来自 | 落地 | 判定 |
|---|---|---|---|---|
| D1 | tools/auto-continue.cx:176 `judge` | 动作句复制循环 `k < 120`、`k < ncap-1` 逐字节 | 钩子 stdout 的 JSON(decision=block 的 reason)与 log.tsv | 危险,R1 已复现 |
| D2 | tools/auto-continue.cx:46 `log_line` | 日志只留末尾 65536 字节:`old += on - 65536` | 持久化 log.tsv(首行从字符中间开始,此后每次写入都不再修复) | 危险,R2 已复现 |
| D3 | tools/auto-continue.cx:94 `last_text` | `m = cap - 1 - o` 钳位(cap=8000) | 喂给判断器的提示词 | 危险(未单独复现,同 D1 机制) |
| D4 | tools/auto-continue.cx:58 `jstr` | `n = cap - 1` 钳位 | session_id/transcript_path | 危险但低影响(ASCII 为主) |
| D5 | tools/devsvc-core.cx:297-298 事件环 | `snprintf(e, EVW=512, ...)` 超长后 `e[EVW-2]='~'` | 事件环加持久化 events.tsv | 危险,R3-A 已复现(库级) |
| D6 | tools/devsvc-core.cx:4242 `events emit` | `%.*s` 长度 `EVW-80` | 事件文本(命令行 TEXT,常为中文) | 危险,R3-B 已复现(库级,是否切坏取决于前缀字节数,偶然对齐时有效) |
| D7 | tools/devsvc-core.cx:1116 on-change | `%.*s` 长度 `EVW-40` | 事件文本 | 危险,R3-C 已复现(库级) |
| D8 | 其它 `%.Ns`/`%.*s`(全仓约 595 处,tools/affected.cx、chain-ready.cx、check-*.cx 等) | 固定字节数 | 绝大多数是十六进制 sha 前缀(`%.16s`、`%.40s`)、ASCII 标识、仅向 stdout/stderr 的诊断显示 | 显示用,不落库,按"显示"类记录,不在本批改。含汉字的显示截断例:tools/archive-mv.cx:169(160 字节)、tools/batch-packages-gen.cx:317-318(160 字节)、tools/chain-ready.cx:184-223(`%.60s`/`%.90s`/`%.120s`/`%.140s`)、tools/pp-differential.cx:46(`%.40s` 源码片段) |
| D9 | `tx_snprintf` 小缓冲区(check-trainjob、check-uxe-*、commit-own 等) | 缓冲区容量,静默截断 | 多为 ASCII 路径和标识 | 未发现含汉字的落库用法;`cx_fmt` 溢出返回错误,是安全的,建议此类处优先 cx_fmt |

未列入:`tools/auto-continue.cx last_text` 的尾部偏移读取(`off = sz - TAIL`)从半个字符开始,但随后按行(`\n`)回溯切行,首个残缺行 JSON 解析失败即被跳过,安全。

## 3. 复现(repros/)

repros/ 里放了 cxroot.tsv 与 `r -> ~/repos/unisacc` 链接,使仓外脚本能解析模块(prd_cx_spec 3.3)。自仓根运行。

| 编号 | 文件 | 命令 | 修前结果 | 修后结果(补丁叠加到 work/o 覆盖树) |
|---|---|---|---|---|
| R1 | repro-auto-continue.cx | `sh ./unisacc.com ~/.claude/jobs/omp1/tmp/utf8-audit/repros/repro-auto-continue.cx` | `R1_stdout=INVALID_UTF8`、`R1_log=INVALID_UTF8`(假判断器答 `a`加 50 个"中"共 151 字节,120 字节封顶落在第 40 个汉字中间) | 两者 VALID |
| R2 | 同上 | 同上 | `R2_log=INVALID_UTF8`(种子 65537 字节,裁剪起点落在第一个汉字的续字节上) | VALID(按整行保留,种子无换行故被整体丢弃,after=29 字节) |
| R3 | repro-devsvc-ring.cx | `sh ./unisacc.com .../repro-devsvc-ring.cx` | A ring-body INVALID(first_bad=509)、B 加一个前导 ASCII 字节 INVALID(440)、C on-change INVALID(486);B 无前导字节时因 `10+432` 恰好对齐而 VALID | 补丁用 u8c_floor 后同样三处都落在字符边界(逻辑与 R1 同一 helper,helper 本身经 check-u8cut 10 项 PASS) |
| E1 E2 | repro-edit-span.cx | `sh ./unisacc.com .../repro-edit-span.cx` | 对照:汉字文件 `--text` 与行段替换后均 VALID,说明 edit-span 自己不切 | 同 |
| E3 | 同上 | 同上 | NEW 带 0xDE 时工具打印 REPLACED 并写出,文件 INVALID_UTF8(只透传,不是切) | 补丁后 `REFUSE output would not be valid UTF-8, first bad byte offset: 1`,文件保持 VALID |

R3 说明:repro-devsvc-ring.cx 是三处截断代码的库级拷贝,没有对在跑的 devsvc 实例发 `events emit`(会写入共享事件流)。补丁后的 devsvc-core.cx 已用 `-c` 编译通过(18 秒,3461600 字节目标文件,已删),未跑 check-devsvc。

## 4. 补丁(patches/,均 `git apply --check -p1` 通过)

| 补丁 | 内容 |
|---|---|
| u8cut-helper-and-check.patch | 新增 tools/cxlib/u8cut.cx(`u8c_floor`:切点回退到字符起点;`u8c_ceil`:尾部起点前进到字符起点;只移动切点,不校验)和 tools/check-u8cut.cx(10 项 PASS,含"每个位置回退后前缀合法、前进后尾部合法"的全位置穷举) |
| auto-continue-utf8-cuts.patch | D1 动作句 120 字节封顶回退到字符起点;D2 日志裁剪起点前进到下一整行;D3、D4 钳位用 u8c_floor;`use cxlib.u8cut` |
| devsvc-core-utf8-cuts.patch | D5 `~` 标记替换整字符而不是第 510 字节;D6、D7 `%.*s` 长度先 u8c_floor;`use cxlib.u8cut` |
| edit-span-utf8-gate.patch | `es_u8gate`:原文件合法而产出不合法即拒绝(两种模式都走);`use text.utf8` |

关于 helper 放哪:查过 cx/text/utf8.cx(已有 tx_utf8_valid、tx_utf8_next,没有回退/前进切点帮手)。我先试过把 floor/ceil 加到 cx/text/utf8.cx,编译报 `tx_utf8_floor` 未声明:cx.text 的公开 API 由 build/std-surface.tsv 与 build/layer-api/cx.text.tsv 冻结,新增公开函数要走 API 表、api_sha 与版本流程,代价大。所以改放 tools/cxlib/(不冻结 API),这与你的建议一致。以后若要升格到 cx.text,u8c_floor/u8c_ceil 可原样搬过去并改名 tx_utf8_floor/tx_utf8_ceil。

## 5. 落库前还需要(补丁未包含)

- tools/check-u8cut.cx 要按仓规登记批次(tools/register.cx),tools/README.md 与 prd.md 能力速查补一行。
- auto-continue 与 edit-span 各有 check-auto-continue.cx、check-edit-span.cx,补丁后需各跑一遍(我只跑了自己的复现,没有跑这两个既有检查)。
- 建议给 check-auto-continue 增一个汉字封顶用例(R1 夹具可直接移植),给 check-edit-span 增 E3 用例。
- 预提交门加"已暂存文本文件必须是合法 UTF-8"的检查,是真正的防线(对应 prd.md 事故):任何路径写进来的坏字节,在此被拦。
- D8、D9 如要治理,建议用 `cx_fmt`+`u8c_floor` 的统一小函数替换含汉字显示的 `%.Ns`,不建议全仓盲改 595 处。

## 6. 约束与未做

- 全部为只读加私有补丁,未提交;工作树与覆盖树在 work/(o 是链接加覆盖,仅用于验证,可删)。
- 一次性生成 repros/cxroot.tsv 用了 awk(只作用于临时目录配置,非仓内文件)。
- 未审计:cx/、ext/、src/ 下的库内截断(范围限于 tools/);tools/ 之外的 apps/csih 属其它车道,未碰。
