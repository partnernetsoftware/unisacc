# CXIF cols 模块边界样例（可验收旁注材料）

- **授刀：** `cxif-cols-boundary-旁注-2145`
- **设计短页：** [`../csih-cx-modularity-2145.md`](../csih-cx-modularity-2145.md)
- **对照源（只读）：** `apps/csih/cols.cx`、`apps/csih/csih_cols.h`
- **口径：** 材料可审；**不是**产品 gate；**不是**「检查器已实现」证据

## 审阅清单

| 项 | 路径 | 应当 |
|---|---|---|
| 公开集 | `provider/cols.public.set` | 仅 `csih_next_cp`、`csih_cp_cols` |
| CXIF stub | `provider/cols.cxif.json` | `public[]` 与公开集一致；无函数体；无宏 |
| 公开头投影 | `provider/cols.public.h` | 与 `csih_cols.h` 同形（声明 only） |
| 正例消费者 | `consumer/use_cols_ok.c` | 只调公开二入口 |
| N1 | `neg/N1_include_provider.c` | 文本 include 提供者 → 设计拒 |
| N2 | `neg/N2_call_private.c` | 调未公开名 → 设计拒 |
| N3 | `neg/N3_body_in_projection.h` | 投影藏函数体 → 设计拒 |

## 绿 / 红（材料）

- **绿（正例）：** 人对照公开集，确认消费者未引用集合外符号；CXIF stub 无体、无私有完整类型。
- **红（反例）：** 上表 N1–N3 任一若被未来检查器放行，即接线回归失败；今日无执行器时，审阅标注「预期拒」即可。

## 分账

属 **0.0.41 旁注材料**；门闩候选 **0.0.42+ 另授**。勿挂 `tests/gate.sh`。勿挡 K6-9。

## 公开头对照

`provider/cols.public.h` 与 `apps/csih/csih_cols.h` **逐字节相同**（本刀复制作对照；不改 apps）。
