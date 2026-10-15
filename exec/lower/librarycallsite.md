# 2c3 callsite lower：窄模型见证，未接运行时

共享挂点仅 lower/gen-manifest.tsv 在 typed 门控后追加一行 `call librarycallsite - fact:librarycallsite …`。原 LFN/MS 表、执行器及 W 不改；复用 parsebase、既有 LFN 的 symbol span/可消费调用授权，以及 MS 的 V3 结构/类型校验。callsite concrete 的 mode=1 是既有 libraryvariadic 的 stack transport；只有明确的 callsite 类别 hook 允许此形状，不放宽普通声明的 MS 规则。

输入为前置验域后的 Tape、USLCON1、USLSEL1 与显式 `NUL+library/callsites` 资源（完整 USLCSF1 兄弟帧）。没有 provider/宿主地址/路径/时间。span 仍指符号 ID，ordinal 留在调用帧，不冒充 plan handle 或符号属性。只消费 E3 已产出的窄整型模板，frame plan 仍为 0；不宣称 ABI、运行时接线或 V0–V7。

## 同一布局与同一记录流

E3 与 lower 共同读取 `exec/parse2/librarycallsite-layout.tsv`，通过通用 facts stem `../parse2/librarycallsite-layout` 消费九行。lower 的匹配、copy 源/目标槽位与帧长度计算都实际使用这些值。digest 是观测：按文件顺序把 plan/slots/result/count/ordinal/reserved/word_width/args_base/stack_args_base 的解析整数重序列化为 `=key<TAB>json<TAB>decimal<LF>`，不含注释。file 与九行摘要须两侧相同，alignment JSON 不进入出货构造。

同一类别有限匹配器在 C.output 前读取已解析 OP/KIND/AC/ARG 记录流，按原 record index 存帧事实，LFN 在消费时按 ci 取。这样不受目标即时数融合跳行影响，不加 ARM 特例，不另解析 Tape。类别内校验帧/签名/ordinal，成功才沿用 LFN 的片段提交；拒绝清输出。缺资源、EMPTY（畸形帧）分别具名，不能把缺 key 伪成空帧或退回旧路径。

冻结 27 例：实际 E3 wrapper 接受；其余帧头/长度/count/ordinal/symbol/签名/槽位负例同拒同因。动态 C 做判定，native runner 与 Python/C 参考只作观测。该有限见证不是任意 C 源、任意 Tape 或完整程序数据流证明。

## 有界准备与命名作业

观察产品通过 MODEL_COM 提供；它只作 -run 工具，不声称新鲜产品证据。缓存仍由既有 librarytypedcheck.sh 的内容键拥有，callsite 仅为其下按本组 fixture 摘要隔离的子目录；不新增缓存清理/生命周期。

先单独运行 `sh exec/lower/librarycallsitecheck.sh --prepare-tools`。
每个十二变体（full/object×lnx/osx/win×x86/arm）依序把下列六项各作一个有界命令：

```
sh exec/lower/librarycallsitecheck.sh --prepare-legacy VARIANT
sh exec/lower/librarycallsitecheck.sh --prepare-on-c VARIANT
sh exec/lower/librarycallsitecheck.sh --prepare-on-p VARIANT
sh exec/lower/librarycallsitecheck.sh --prepare-tables VARIANT
sh exec/lower/librarycallsitecheck.sh --prepare-net VARIANT
sh exec/lower/librarycallsitecheck.sh --prepare-check VARIANT
```

合法 VARIANT 与旧 typed lower 完全一致；每步经既有 bound 55，禁止合并长准备或放宽限时。legacy 步复用旧准备；on 两侧 C/P 构造逐字节相同，tbl/net 全观察域穷举。全部准备齐后 `1`、`2`、`3`、`4` 四个命名作业各自单跑，覆盖全夹具与各目标抽样。Windows 只按当前目标拒绝合同记账，不计 Windows 接线绿。

## 五问

复用 parsebase/MS/LFN 与通用 facts/执行器；预扫与现有解析记录流共机制、callsite 帧语义分账；不新建产品读写/资源/生命周期；MISSING 与 EMPTY 分开；范式进展是同一流式验证表可跨目标融合复用，重构优先于版本收口。
