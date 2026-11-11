# cdx2 首观察入仓漏证短记（1159）

采样墙钟：2026-10-11T12:02:41.936993+08:00。只读，仓外记；未写仓、未发送。

结论：research-only 入仓范围吻合；UNKNOWN 与原证局限保留，未见借入仓续 B.2/Draft。首观察范围闭合沿用1159裁定，非产品 PASS。

- tip：HEAD=origin/main=ls-remote refs/heads/main=`c1a76e0611ebf534e053eb37fc43059cf5551d5e`；净树。前tip bbd84a22，入仓commit仅24个 research/c40-first-observe/ 文件，无产品/version/gatedeps/.cx/prd改动。
- Latest实查仍 v0.0.39；release列表无v0.0.40/Draft；历史v0.0.31 Draft与本窗无关。
- HASHES.sha256 22项独立复算全吻合；九份迁入回执/短记/裁定与仓外同名原件逐字节相同；可对应state副本未见字节差异。UA二进制未入仓，仅哈希。
- UNKNOWN保留：t_end、1147 ps原表、identity-after ls-remote、失效键/seed/exittable、真make；1159裁定另明确接受“若干声明式rc0原证”。cdx2-1152短记仍具名UA/独立H1 rc0原证缺、archive前后完整SHA缺、运行时身份/argv/env/发行版/outer身份等缺口，没有被迁入时抹掉。
- 摘要缺口：README及cc入仓回执的UNKNOWN摘要未列“声明式rc0原证”；完整裁定与短记已保留，故不是原证闭合，也不应单凭索引宣称采集全集齐。H1缩写与事后ps局限原样保留。
- 入仓清单只保存观察与裁定；state/shim/make是样本副本，不是新执行证据。cc现场仅复制、写索引、路径提交与普通push；make原记录仍一行、cand仍仅shared日志，未见续真make/B.2、重跑首观察或公开/seal/bump。

提效：后续读索引连同1159具名接受裁定，勿把UNKNOWN摘要当原证齐全；B.2/Draft另授前只整理交接，不以入仓启动构造。
