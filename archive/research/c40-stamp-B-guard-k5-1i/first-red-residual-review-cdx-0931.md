# 首红残差只读复核（cdx；0931）

实际只读核验时间：2026-10-11T09:34:00.694004+08:00。未写仓、未重跑夹具或门禁。

- identity 一出现即读取：标题及正文明确“事后采样”“不是跑 r3 或写材料时的瞬时身份”“不作为跑前身份证明”。所记写前 tip `a4d85631c47f881c76ed4ded0dbf9f0d18180c85` 与本刀父提交相同。所记当时 status/behind2 属作者事后记录，不冒充本哨独立采得跑前状态或远端三者一致。
- identity 的夹具、材料、两份 review、0902 回执、原件与副本 SHA256SUMS 总哈希均独立重算吻合；原件/副本13文件逐字节相等，SHA256SUMS全部条目重算通过。原件范围明确为 `/tmp/cc40-prep/first-red/r3/`，不包含r1/r2。
- §6 工作文件已对齐 r3：stdout含 chain: E3 gen failed，六类 tbl/net/check-net/probe-run/ua-ref/gen，及全部事件计数零；状态行“命令层限定闭合”存在。工作文件与本刀提交字节相同。页标题仍留“草案，未实跑”历史标签，正文已明确新状态，不影响日志判读，建议以后纯文档整理。
- 入仓前（本哨首读）：HEAD=a4d85631，材料 M，两份 review 均 ??，identity 不存在。入仓后：提交 `932d3ce4f4d0308054ab52ed2722a3a3395c1230`；完整差集恰材料 M、identity A、两份 review A（四路径，全部research）。两份review已被Git跟踪、工作文件与提交相同；tests/exec/prd/src本刀差集空，没有夹具或产品变更。当前 status：''。
- 独立重读r3：NEG E3记录rc3为第17行且最后一行，chain rc1；此前复核的正例可见性和红后零事件未变，原“命令层限定 PASS”仍有证据支持。

## 独立哈希

| 对象 | SHA256 |
|---|---|
| tests/chainfirstredcheck.sh | `864613567dacaa841620511e67b9c343d86f99cb14a243a7fc694ebc7b54a205` |
| research/c40-stamp-B-guard-k5-1i/first-red-zero-start-materials-0857.md | `b6e796e5d86cd18b94c554b7de240a77bc2f1d8f1599f5e029b341ed56b01663` |
| research/c40-stamp-B-guard-k5-1i/first-red-fixture-review-cdx-0902.md | `ef162acdce4ae3a17ce9f6717317c599948b3e09fe798b1df5b1d63a4a50fa59` |
| research/c40-stamp-B-guard-k5-1i/first-red-zero-start-review-cdx-0857.md | `93193f33ad79824e02d79d892f46801823a4b9c999693537d1abae9e9819b9fd` |
| research/c40-stamp-B-guard-k5-1i/first-red-receipt-cc-0902.md | `9f309525419f33b5ec3719b84b0258928772bd6e249c9a2c333eab5de22e57e4` |
| /tmp/cc40-prep/first-red/r3/SHA256SUMS | `71b3f97db6f1a12c7ca718dcb389f6dd15858218e2179ab25aa620cec1d56cbf` |
| research/c40-stamp-B-guard-k5-1i/first-red-run-0902/SHA256SUMS | `71b3f97db6f1a12c7ca718dcb389f6dd15858218e2179ab25aa620cec1d56cbf` |

仅核材料/身份措辞、哈希与入仓事实；本哨不代裁最终闭合，不将事后账升格为跑前身份。PGID/SID清空、TERM、UA本体、预算、门绿及净提速继续未证；没有bump/Draft/重跑或写仓。短记可供cc后续获授research-only副本带交，本轮未复制或发送工具消息。

只读核验吻合；命令层限定 PASS 保持，最终闭合待机房主任裁定
