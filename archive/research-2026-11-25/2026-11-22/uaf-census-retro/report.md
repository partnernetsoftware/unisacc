# R79 uaf-census 历史真缺陷回测

- 工具: `tools/uaf-census.cx` / `tools/check-uaf-census.cx`(已落 main,报告模式)
- 日期: 2026-11-22
- 基线工具提交: fbe594b2a0; 回放核对 HEAD 见当时私有收据

## 样本量与检出率

在范围(同函数简单标识符 UAF,父提交文件,工具修订后):

| 样本 | 父提交 | 结果 |
| --- | --- | --- |
| chain-run old? | fcf9398dd8 父 | HIT kind=value |
| check-cand tb&& x2 | fcf9398dd8 父 | HIT kind=value |
| lower-row m? | fcf9398dd8 父 | HIT kind=value |
| p3-swap memcmp raw | fcf9398dd8 父 | HIT kind=arg |

在范围检出率: 5/5 (N=5,M=5)

范围外(预期漏报,计入全样本):

| 样本 | 提交 | 结果 | 原因 |
| --- | --- | --- | --- |
| free(t.b) 字段 | 8b4c5511af | MISS | LIMIT: struct-field |
| 下标越界 | 1520a08bfc | MISS | 非 UAF |
| litlen x2 | 0dff6077db | MISS | 非 UAF |
| .c ASan | 2ae13bad52 | MISS | 只扫 .cx |

全样本检出率: 5/9 (N=9,M=5)

## 规则修订(相对过严版全仓 0 候选)

(1) 值使用: free 后 `p?` / `p&&` / `p||` 报 kind=value
(2) 循环内 free 也种子
(3) 出块弱标记: 跨 if 块只保留 value,不报 arg/read(控假阳)

代价: 全仓(cx,ext,src,tools) candidates 0 -> 27; tools 0 -> 26。

## 合同

- 不写「无 UAF」。零候选只说明在范围规则下现存源码无「free 后同函数简单标识再用」形态;跨函数/字段/非.cx 仍盲。
- 先报告模式,不进 `chain-ready`;基线只降不升。
