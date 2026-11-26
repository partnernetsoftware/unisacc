# 2026-10 以来 X 上 Orca vs Herdr 舆情调研

- 范围: 2026-10-01 至 2026-11-26, X 关键词/语义检索 + 关键帖线程
- 对象: Orca (`stablyai/orca`, `@orca_build`, orca.dev); Herdr (`herdr.dev`, `@herdrdev`)
- 方法: 只收录检索接口返回的原文。找不到的维度写样本不足。不编造引文。
- 链接一律 `https://x.com/<handle>/status/<id>`。时间按帖上 UTC。
- 标签: `已核实原文` = 本文件写调研时接口返回了全文; `仅转述` = 只见到别人引用或文章链,未展开原文。

## 1. 样本量与偏差

检索命中约 90+ 条相关帖,去重后可引用约 40 条。其中:

(1) 两边都用过、有取舍理由的实测,约 10 人: `@solanky` `@jilaiqian` `@laogui` `@NixFred` `@ohblue` `@charliesbot` `@kzkhykw` `@Suercha007` `@benatcortexai` `@ericjingyang`(Orca->T3,顺带评 Herdr 界面)。

(2) 只深度用一边的日常用户,约 12 人: Herdr 侧 `@axadrn` `@qus0in` `@chyuwz` `@aclempe` `@seapy` `@klieber` `@pveierland`; Orca 侧 `@eatmoreduck98` `@hx0000001` `@Stammy` `@arthuryeti` `@KasakiNeo` `@JudeTejada2`。

(3) 官方: `@orca_build` `@JinjingLiang`(Orca 创始) `@herdrdev` `@nwparker_`(Orca CTO,被用户点名回关,本调研未展开其长文)。

(4) 清单/平替文/Amb 带节奏: `@criscxuan`(Qwen/WorkBuddy Ambassador) `@Pluvio9yte` `@AIGrowWorks` `@reallychurjan` `@JayTL00` `@Voxyz_ai`(推 coordinator skill,主力改 Paseo) `@eatmoreduck98`(自称给 Orca 做过贡献,并庆祝 CTO 回关)。

时间偏差:

(1) 10 月头对头比较少。10-29 `@herdrdev` 推 herdr-core; 11-01 Herdr 桌面引发第一波卸载 Orca 帖; 11-07 `@jilaiqian` 长测; 11-09 Herdr 手机上架。之后才有「SSH vs 手机」对照。

(2) 11-24 起 T3 Code 遥测翻车(陈大黄帖浏览 228 万)把中文时间线灌成「T3 平替」。Orca 被当成干净替代集中安利,星数/「杀死比赛」话术重复出现。这一波不是 Orca vs Herdr 的自然比较,是 T3 的溢出流量。Herdr 在这波里常被写成「终端党选项」,不一定被实测。

(3) 11-23 `@JayTL00` 仍写 Herdr「没有独立手机 App」,与 11-09 官方上架矛盾,属过期清单,不能当现状。

谁在带节奏:

(1) Orca 官方大量转中文安利(`@voidJan`「ORCA 真的太爽了」196k 浏览被 `@orca_build` RT)。

(2) `@Pluvio9yte` 自称「今天在推友推荐下刚下载,直接就爱上了」同时宣布杀死比赛,属首日安利,不是长测。

(3) `@criscxuan` 同日长文给 Paseo/Orca/Herdr 分场景,带商务邮箱与 Amb 身份,属 KOL 综述,不是单人长测日志。

(4) Herdr 官方 11-09 手机发布 22 万浏览,是产品节奏,不是用户互评。

找不到就不写: 两边产品自身的订阅价/配额对比(都宣称复用用户已有 Claude/Codex 等订阅); Orca 公网端口直开的用户实测(官方/Grok 转述「默认拒绝公网端口」,用户帖几乎只谈 Relay/配对/Tailscale); 崩溃 dump/复现帖数量级不足,不能下「谁更稳」的统计结论。

## 2. 按维度

### 2.1 连接方式 (relay + 公网 vs 纯 SSH)

共识(用户实测 + 官方,方向一致):

- Herdr: 手机/多机走 SSH(常叠 Tailscale/VPN)。官方手机「零账号」,用户复述「纯 SSH 直连不经过官方服务器」。
- Orca: 本机桌面 + 官方手机配对;出门用 Relay(需登录);远程工作区另说。有人用 Orca 的 remote SSH,再叠 Tailscale 才能从手机连。

| 谁 | 时间(UTC) | 链接 | 标签 | 原文要点 |
| --- | --- | --- | --- | --- |
| `@knowhynet` | 2026-11-26 02:11 | https://x.com/knowhynet/status/2125774532585677092 | 已核实原文 | 「电脑关掉的话直连会断,出远门要用 Relay,这个需要登录。」对照官方手机说明后写,属读文档+准备换用,不是长测。 |
| `@DmytroBavykin` | 2026-11-26 07:24 | https://x.com/DmytroBavykin/status/2125853251782222231 | 已核实原文 | 「i used remote SSH feature in Orca to connect it and i would probably need Tailscale on top of that to connect from my phone remotely」;自称 day 1 还没装完。 |
| `@qus0in` | 2026-11-26 07:06 | https://x.com/qus0in/status/2125848546272563289 | 已核实原文 | 「mac mini + tailscale + herdr + pi ... + discord로 정착」 |
| `@chyuwz` | 2026-11-26 04:15 | https://x.com/chyuwz/status/2125805640904003820 | 已核实原文 | 家里 Mac 常开 Tailscale,自建 derp;「herdr远程」。缺点:要服务器费;「herdr 手机端不够好用」。 |
| `@aclempe` | 2026-11-25 20:44 | https://x.com/aclempe/status/2125692012754264416 | 已核实原文 | 「works well if you have tailscale or VPN setup」 |
| `@kzkhykw` | 2026-11-02 02:26 | https://x.com/kzkhykw/status/2117080929726615753 | 已核实原文 | 选 Orca:「Orcaモバイルが使いやすくて、なんかあった時に常駐Macにモバイルからでも触れる」 |
| `@dbmikus` | 2026-11-24 21:04 | https://x.com/dbmikus/status/2125334790224388231 | 已核实原文 | 「Orca runs its own relay ... everything sent through it is encrypted so only those two can read it」。作者在做竞品 amika(YC),属知情者转述架构,不是抱怨。 |
| `@criscxuan` | 2026-11-26 07:01 | https://x.com/criscxuan/status/2125847327579820207 | 已核实原文 | KOL: Herdr「纯 SSH 直连不经过任何官方服务器」;Orca 有手机配套。非长测。 |

样本不足: 用户打开 Orca 公网端口的实测(成功/被扫/被打) 0 条。Relay 延迟对比数字 0 条。

### 2.2 延迟与手感

| 谁 | 时间 | 链接 | 标签 | 原文要点 |
| --- | --- | --- | --- | --- |
| `@jilaiqian` | 2026-11-07 17:22 | https://x.com/jilaiqian/status/2119118276089245782 | 已核实原文 | Herdr TUI「minimalista, muy rápida」; Orca「funciona muy pero que muy bien」但「demasiado tosco ... me llega a agobiar」; herdr-app「SSH están un poco buggy」。一周内 herdr->Orca->herdr-app->T3,最终留 T3(该结论已被后续 T3 遥测事件污染,只作当时手感)。 |
| `@arthuryeti` | 2026-11-24 23:39 | https://x.com/arthuryeti/status/2125373695380402488 | 已核实原文 | 「orca feels slow, t3code feels minimalist」 |
| `@hx0000001` | 2026-11-26 06:59 | https://x.com/hx0000001/status/2125846897336475791 | 已核实原文 | 「手机直接给电脑里的 agent 派活,彻底告别远程桌面那套卡顿操作了」-> 相对远控桌面,不是相对 Herdr。 |
| `@pveierland` | 2026-11-24 02:32 | https://x.com/pveierland/status/2125054864325816569 | 已核实原文 | 向 Mitchell 要 Rex 邀请:「Tired of Herdr lag。」未写测量,未对比 Orca。 |
| `@jayfx42` | 2026-11-26 06:26 | https://x.com/jayfx42/status/2125838639909449798 | 已核实原文 | Rex 加 SSH remote 后「从 Herdr 迁移到 Rex ... 流畅度非常高」。Orca 未出现。 |
| `@NathanFlurry` | 2026-11-12 00:10 | https://x.com/NathanFlurry/status/2120670510698869243 | 已核实原文 | 1 小时初印象: Herdr「more customizable (afaik its just electron)」「mobile basic but it works」; Rex「feels fast (no electron)」。Electron 判断可能不准(见下稳定性/形态)。未提 Orca。 |
| `@Suercha007` | 2026-11-02 06:10 | https://x.com/Suercha007/status/2117137278174728360 | 已核实原文 | 前帖推 Orca「被喷惨了,主要是因为 ... 一些人说占内存」,于是改推 Herdr。内存抱怨是转述评论区,本帖未给数字。 |
| `@NixFred` | 2026-11-11 23:03 | https://x.com/NixFred/status/2120653781415203015 | 已核实原文 | 9-30 起 agent 跑在 Orca,10-08「switched back away from orca」因为终端与语音旋钮控制器绑死 Orca,herdr 会话变成 watch-only;另:本机 12 核 load 43 时「I feel it lagging when I type」(多 worker+本地模型,不是 Orca 单独锅)。 |

样本不足: 同网络下 Orca Relay vs Herdr SSH 的往返时延对比。

### 2.3 多机切换

| 谁 | 时间 | 链接 | 标签 | 原文要点 |
| --- | --- | --- | --- | --- |
| `@laogui` | 2026-11-01 15:36 | https://x.com/laogui/status/2116917183888728145 | 已核实原文 | Herdr:「关闭 GUI 窗口或通过 SSH 接入远端服务器均可保持会话常驻,并在同一侧边栏集中管理。」「Orca 虽然基于终端但形态更重」。 |
| `@Suercha007` | 2026-11-02 06:10 | 同上 | 已核实原文 | Herdr「本地的和保存好的 SSH 机器放在一个窗口里,agent 列表是汇总的,每条连接独立重连。」「关掉客户端或者掉线,后台的终端继续跑。」机器重启「原来的进程留不住,这点它写得挺清楚。」 |
| `@benatcortexai` | 2026-11-08 00:26 | https://x.com/benatcortexai/status/2119225129108660604 | 已核实原文 | 回 `@jilaiqian`: Orca 编排「agents from different machines just messaging away」;「Love the mobile app」。T3 没调顺。 |
| `@0xTiby` | 2026-11-13 08:58 | https://x.com/0xTiby/status/2121165676736106519 | 已核实原文 | 已切 Herdr Desktop + iOS,仍要 web/终端进 herdr-core:「sometimes I still need web access」。 |
| `@ZumaniaX` | 2026-11-25 14:07 | https://x.com/ZumaniaX/status/2125592188205805955 | 已核实原文 | 问 Orca:「When will it be possible to access VPS workspace from mobile app ? Why is that not possible ?」说明当时手机到 VPS 工作区仍缺。 |

### 2.4 移动端 UI (收件箱 / 终端 / 截图 / 语音)

官方能力(产品声明,不是用户长测):

- `@herdrdev` 2026-11-09 22:32 https://x.com/herdrdev/status/2119921157801652245 已核实原文: iPhone/iPad/Android;「see which agents need you, open the real terminal and answer. type, paste a screenshot, or just talk to it。」依赖 herdr-core,当时 TUI 还不支持。同串官方自承 early,「there WILL be bugs」。
- `@orca_build` 多次演示手机上 Antigravity/Bots; 2026-11-20 称手机 OTA 从桌面拉版本。

用户:

| 谁 | 时间 | 链接 | 标签 | 原文要点 |
| --- | --- | --- | --- | --- |
| `@jilaiqian` | 2026-11-07 | 见上 | 已核实原文 | 当时 Herdr「no tiene features ... conectarse por el móvil」(手机发布前两天,当时为真)。Orca 有手机,但整体过重。 |
| `@eatmoreduck98` | 2026-11-26 07:05 | https://x.com/eatmoreduck98/status/2125848351329702189 | 已核实原文 | 录了手机处理开源项目:开 worktree、聊天、审 PR;「已经完全能把 UU 远程换掉」。贡献者+庆祝 CTO 回关,偏安利,但是带操作录像。 |
| `@hx0000001` | 2026-11-26 | 见上 | 已核实原文 | 手机派活,告别远控卡顿。短评,无对照 Herdr。 |
| `@chyuwz` `@aclempe` | 2026-11-26 / 25 | 见上 | 已核实原文 | Herdr 手机「不够好用」/「not like T3」。 |
| `@NathanFlurry` | 2026-11-12 | 见上 | 已核实原文 | Herdr mobile「basic but it works」。 |
| `@Stammy` | 2026-11-25 18:30 | https://x.com/Stammy/status/2125658278361522622 | 已核实原文 | 设计工具变成「@orca_build + iOS (and some Android) simulator」,一个月没用 Figma。是 Orca Design Mode 实测,不是 vs Herdr。 |

收件箱/语音: Herdr 官方写了 talk/screenshot; 用户侧「语音好用」的对照帖样本不足。Orca 手机通知+拍板有 `@eatmoreduck98` 录像,无独立「inbox 产品」对比帖。

### 2.5 多 agent 编排

| 谁 | 时间 | 链接 | 标签 | 原文要点 |
| --- | --- | --- | --- | --- |
| `@solanky` | 2026-11-01 04:10 | https://x.com/solanky/status/2116744695070236981 | 已核实原文 | 卸载 Orca 全转 Herdr 桌面。Orca「puts hooks into every agent's config」:清理时 Claude Code 13 条、Codex 9、Devin 8、Hermes 一个 plugin,「all running on every prompt and tool call」。Herdr「reads agent status from the terminal itself」,配置变干净。喜欢:agent 用小 CLI 开旁边 pane 把 sudo/prod 命令留在提示符让人按 Enter;少部件。240 赞,40 回复。本调研里权重最高的切换理由。 |
| `@JinjingLiang` | 2026-11-01 18:54 | https://x.com/JinjingLiang/status/2116967174741045518 | 已核实原文 | 官方答辩:终端读状态「still flaky even with OSC7501」; hooks 才是可靠状态,「first-party native features」「hook execution costs virtually zero」。 |
| `@ohblue` | 2026-11-01 12:29 | https://x.com/ohblue/status/2116870309030805680 | 已核实原文 | 「我也和你一样,刚刚把orca卸载回到herdr」 |
| `@charliesbot` | 2026-11-02 16:59 | https://x.com/charliesbot/status/2117300636584915351 | 已核实原文 | 「Herdr nailed the interface! ... Orca is so overwhelming」 |
| `@laogui` | 2026-11-01 | 见上 | 已核实原文 | Herdr: OSC 7501,「不会修改你的 Agent Hooks 文件」; `hc` 让 Claude 后台拉 Codex 审查,结果回主输入流。 |
| `@hexudong08` | 2026-11-24 01:29 | https://x.com/hexudong08/status/2125038950977044756 | 已核实原文 | 11 分半中文教程: herdr 主 agent 指挥三个 worker,真实终端。未比 Orca。 |
| `@benatcortexai` | 2026-11-08 | 见上 | 已核实原文 | Orca 编排子系统跨机统一规则/CI,「sooo good」。 |
| `@KasakiNeo` `@JudeTejada2` | 2026-11-24 | https://x.com/KasakiNeo/status/2125351795392381287 https://x.com/JudeTejada2/status/2125371026997649830 | 已核实原文 | 「I need a lite version of orca, the current one is so overwhelming」; Bots 加上后 threads 更挤。 |

### 2.6 配额与价格

两边公开叙事都是:编排器本身开源免费,算力走用户已有 Claude Code / Codex / 等订阅。

| 谁 | 时间 | 链接 | 标签 | 原文要点 |
| --- | --- | --- | --- | --- |
| `@ryanhn717` | 2026-11-25 14:25 | https://x.com/ryanhn717/status/2125596752367538461 | 已核实原文 | 读官网后写: Orca MIT 免费;干活的是接入的 agent,「要自备相应订阅」。 |
| `@knowhynet` | 2026-11-26 | 见上 | 已核实原文 | 「还是用自己已有的套餐,不会再单独扣一份。」 |
| `@RRweb3` | 2026-11-26 06:13 | https://x.com/RRweb3/status/2125835221400801574 | 已核实原文 | 清单文:「Open-source doesn't always mean free model usage」 |

样本不足: Herdr/Orca 自己向用户收费的档位对比;谁更省 token;Orca Bots/云 VM 是否另计费的用户账单截图。`@criscxuan` 提到 Paradev 远程/手机有付费档,那是第三方,不是这两家。

### 2.7 安全面 (遥测 / hooks / relay)

| 谁 | 时间 | 链接 | 标签 | 原文要点 |
| --- | --- | --- | --- | --- |
| `@solanky` + `@stretchcloud` | 2026-11-01 / 11-02 | https://x.com/solanky/status/2116744695070236981 https://x.com/stretchcloud/status/2117047033601097890 | 已核实原文 | 核心争议是 Orca 往各 harness 写 hooks,每 prompt/tool call 都跑。stretchcloud 升成「权限与信任」:hook 也可能默许 sudo/prod。Herdr 读终端、不改配置。 |
| `@JinjingLiang` | 2026-11-01 / 11-24 | https://x.com/JinjingLiang/status/2116967174741045518 https://x.com/JinjingLiang/status/2125307925120053652 | 已核实原文 | 为 hooks 辩护;遥测指向 https://orca.dev/docs/telemetry 与源码。 |
| `@ZryMiller` | 2026-11-25 07:53 | https://x.com/ZryMiller/status/2125498086541885654 | 已核实原文 | 审计 25 个应用: Orca「The ONLY app out of 25 that cleanly stops both analytics and diagnostics when DO_NOT_TRACK is set, and turns off IP/location tracking。」扣分:「silently opting you in if you click the X on its banner。」 |
| `@ilseccoz` | 2026-11-25 07:03 | https://x.com/ilseccoz/status/2125485432209129506 | 已核实原文 | 对照读 T3 与 Orca 代码。关 Orca: Settings → Privacy 或 `DO_NOT_TRACK=1`。长文链 https://stocasticparrot.ai/en/numbers/telemetry-orca-t3-code/ 本调研未逐行核该站,属已核实推文、文章细节仅转述。 |
| `@Pluvio9yte` | 2026-11-25 07:04 | https://x.com/Pluvio9yte/status/2125485780072350040 | 已核实原文 | 「遥测也能够自己选择关闭」;当天刚下载。被 `@orca_build` 转发过同类安利。 |
| `@criscxuan` | 2026-11-26 | 见上 | 已核实原文 | 「选 Orca(记得先关遥测)」; Herdr「手机 App 零账号、零遥测」。 |
| `@seree` | 2026-11-24 13:12 | https://x.com/seree/status/2125215983757599193 | 已核实原文 | 直接问 `@orca_build` 是否像 T3 一样激进遥测。 |
| `@VillaHasNoGift` | 2026-11-26 06:52 | https://x.com/VillaHasNoGift/status/2125845093181726803 | 已核实原文 | 长文锤 T3;对比句「Orca充分告知,GUI设计关闭按钮。」这是相对 T3,不是相对 Herdr。 |
| `@eatmoreduck98` | 2026-11-24 13:31 | https://x.com/eatmoreduck98/status/2125220862462984391 | 已核实原文 | 「遥测和安全问题得看证据,不能因为另一家有争议,就说 Orca 一定更安全。我推荐它,单纯是因为自己用着确实不错。」 |
| `@Kosumi1989` | 2026-11-25 17:58 | https://x.com/Kosumi1989/status/2125650378138300880 | 已核实原文 | 自称没用过 Orca 这类 GUI:「relays and telemetry」「buggy, slow and bloated」是对品类的偏见,自研 SSH/Herdr 方案。不能当 Orca 实测。 |

归纳(有原文支撑的部分): Orca 产品遥测默认存在、可关,审计者给过相对高分但仍有 banner 点 X 即 opt-in 的扣分;账号/手机配对/通知另有数据处理(`@ryanhn717` 读官方文档)。Herdr 叙事是不改 hooks、手机不经官方服务器;用户 `@aclempe` 承认没读过代码,「potentially they are collecting just metrics」。Herdr 遥测源码级用户审计帖,样本不足。

### 2.8 崩溃与稳定性

| 谁 | 时间 | 链接 | 标签 | 原文要点 |
| --- | --- | --- | --- | --- |
| `@herdrdev` | 2026-11-09 22:32 | https://x.com/herdrdev/status/2119921161379459367 | 已核实原文 | 手机「early」「there WILL be bugs」;11-10 回复「fix for android coming!」 |
| `@jilaiqian` | 2026-11-07 | 见上 | 已核实原文 | herdr-app 远程 SSH「un poco buggy」;「aún está muy verde」。 |
| `@JinJieBeWater` | 2026-11-01 17:14 | https://x.com/JinJieBeWater/status/2116941995767898344 | 已核实原文 | 「Herdr App 出来那个默认UI一堆Bug我对他还挺失望的」,后来认可 GPUI 布局引擎。 |
| `@jilaiqian` 对 Orca | 2026-11-07 | 见上 | 已核实原文 | 「funciona muy pero que muy bien」-> 功能能跑,抱怨的是重/压人,不是崩。 |
| `@seapy` | 2026-11-26 01:38 | https://x.com/seapy/status/2125766134960120199 | 已核实原文 | 「Really happy with Herdr lately. Not viral, just quietly getting stronger。」Karrot CTO,偏正面稳态,无崩。 |

样本不足: Orca/Herdr 桌面崩溃率、数据丢失、会话损坏的连续用户报告。不能写谁更稳。

## 3. 官方口径(便于对照,不当作用户实测)

- Orca `@orca_build` bio: Desktop + mobile + VPS; MIT; 自有订阅跑 bot 舰队。11 月密集 RT 中文安利与 Antigravity/mobile 演示。
- Herdr `@herdrdev` bio: 「the runtime your coding agents live on」。10-29 herdr-core; 11 月桌面; 11-09 手机; 11-25 与 `@mitchellh`/`@superlogical` 合作 OSC 7577 重启恢复(Herdr 下个版本才接,属预告)。

## 4. 可执行对照(仅基于上面有原文的点)

终端原住民、多机 SSH、不想改 Claude/Codex hooks: 用户把票投给 Herdr(`@solanky` `@ohblue` `@laogui` `@qus0in`)。

要手机收件箱式拍板、Design Mode/浏览器点选、Linear/GitHub/Bots、跨机编排 GUI: 用户把票投给 Orca(`@kzkhykw` `@benatcortexai` `@eatmoreduck98` `@Stammy`)。

两边都试过的人里,「Orca 能跑但太重」重复出现(`@jilaiqian` `@charliesbot` `@KasakiNeo`);「Herdr 手机/SSH 桌面还绿」也重复(`@jilaiqian` `@chyuwz` `@herdrdev` 自承)。

11 月下旬中文区 Orca 声量被 T3 翻车放大,星数安利 != 长测。若只看 11-01 至 11-12 的切换帖,Herdr 桌面是从 Orca 迁出的主因(hooks + 形态轻),不是功能清单输赢。

## 5. 检索缺口(下次若要加严)

(1) 10 月(T3 翻车前)英文长测对照帖偏少,可能被 11 月新帖挤出 Latest 前页。
(2) Discord/GitHub issue 不在本次范围。
(3) `@voidJan` 高浏览安利帖未逐条核评论区反对意见。
(4) DEX 同名 Orca(Solana)噪声已手工剔除。
