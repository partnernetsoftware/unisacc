# 第二轮预跑 DONE 后的证据整理清单(2026-11-25 ccogweco)

只写清单;命令与命名以轮内页面(round2-diffs、round2-sampling、f2-seal-checks)与 107 车树工具为准。
顺序按"先原始、再合并、后折叠"排;任何未跑的文件标 UNMEASURED,不预写内容。

## 0 顺序总表

| # | 文件/动作 | 内容预期 | 生成命令(入口) | 负责人 |
|---|---|---|---|---|
| 1 | 预跑原始 strict(逐窗 `strict-W*.tsv` + 合并前) | 241 窗的原始行,含 rc124/FAIL/UNKNOWN 原行 | 预跑自身产出;不手工改 | agy-flow |
| 2 | 合并 strict:`release/evidence/v0.0.107-strict-<DATE>.tsv` | 按批折叠:同批多行取最后一行(release-prep.cx:171-182 的 fold by batch id) | release-flow `prep` 内由 release-prep 读取 | agy-flow |
| 3 | 证据目录 strict:`release/evidence/0.0.107/strict.tsv` | 与 2 同一候选、同一折叠结果(scoped 到证据目录) | 同上,或按 round2 页的复制步骤 | agy-flow |
| 4 | `expected-fail.tsv` | 由合并 strict 与上一版预期生成;superseded 计数按原始 FAIL/INCOMPLETE 行 | release-prep `draft_expected`(release-prep.cx:188+) | agy-flow |
| 5 | `expected-unmeasured.tsv` | 只列无生产者的 UNKNOWN;本轮 #5/#6 已入车,应无 cx-booter-ape / tools-default-regression 两行 | 预跑输出/证据整理步骤 | agy-flow |
| 6 | `cx-booter-ape-deferred.tsv`、`tools-default-regression-deferred.tsv` | **应为空壳(仅表头或无数据行)**,因为两批本轮应 PASS 而非 deferred | 预跑输出;空壳头照 evidence-107-template.md | agy-flow |
| 7 | `fixedpoint-6targets.tsv` | 六目标 g2/g3 逐对 sha 与 compare 四同原行 | 107 车树的 fixedpoint-evidence 工具(链止后) | agy-flow |
| 8 | `ape-receipt.txt` | 链 ape-final 回执脱敏拷贝(六切片 path/sha/bytes) | 拷贝 + scrub_home | agy-flow |
| 9 | `old-product-assets.tsv` 追加 0.0.107 行 | 公开 APE 的 bytes 与 sha256(9a1e2630… 或第二轮新值) | 按 R33 登记行格式;值取公开资产 | cc/agy-flow |
| 10 | `release/evidence/0.0.107/INDEX.tsv` | 五列行:fixedpoint-6targets、ape-receipt、exec-1-X-commit、exec-2-draft-readback、exec-3-ci、exec-4-Y1、exec-5-publish、exec-6/7 等 | 各步骤执行器写;人不代写 | agy-flow |

## 1 strict 折叠预期(原始 FAIL/INCOMPLETE 行按窗计)

- 计数口径:superseded 按**原始 FAIL 或 INCOMPLETE 行**计,不是按批;折叠由 release-prep 按批 id 取最后一行。
- 预计折叠项(以第二轮 supervise.log 原行为准):
  - W0000:INCOMPLETE/rc124 冷窗(两次尝试)→ 以 `strict-W99xx` 复跑行折叠;
  - W0015:INCOMPLETE;
  - W0016:FAIL;
  - W0017:UNKNOWN;
  - W0088:若红,同法折叠。
- 以上窗号对应批名必须从第二轮 `windows.tsv` 第 1、5 列逐项取,不得沿用第一轮窗号;原行保留,折叠只换最终行。
- 期望最终 SUMMARY:FAIL=0(产品缺陷 W0201 已在第二轮修后重跑);UNKNOWN 只允许已登记无生产者项且不超 2;
  INVALID=0。未达即如实记,不调口径。

## 2 两 deferred 文件为空壳的判据

- 先核 `grep -c 'cx-booter-ape' <merged strict>`:有 PASS 行且无 KEEP_GOING/UNKNOWN;同理 tools-default-regression;
- 两者都 PASS 时,deferred 文件保留空壳头(照 evidence-107-template.md 的格式),不删文件、不写"已清";
- 若仍出现 deferred,说明 #5/#6 未在候选上生效,按 FAIL 处理并报 cc。

## 3 old-product 0.0.107 登记

- 行格式:`0.0.107<TAB>partnernetsoftware/unisacc<TAB>v0.0.107<TAB>unisacc.com<TAB><bytes><TAB><sha256>`;
- 值取公开 release 资产本身(第二轮 APE sha 与 bytes),不取本机安装件;登记后跑一次 predecessor 核验;
- 不做预登记;公开后再落,回滚为删该行。

## 4 命名与文件头核对(照 evidence-107-template.md)

- strict 双名:`release/evidence/v0.0.107-strict-<DATE>.tsv`(合并表)与 `release/evidence/0.0.107/strict.tsv`(证据目录);
  两者同候选、同折叠、同 SUMMARY,不得只给一个。
- 所有证据文件头保留模板给的空壳头(如 strict 的 `# source=` 与表头、INDEX 五列);不预填 sha。
- 家目录路径 0 命中;`~/` 简写可留。

## 5 失败处置

- 合并 strict 的 SUMMARY ape_sha256 与 layout 不一致:release-prep 会 `STRICT_CANDIDATE` 拒绝;核候选身份,不手改表;
- 折叠后仍有 FAIL:按 S1 分类;产品红不折叠成 PASS;
- deferred 非空:按 #5/#6 未生效处理,报 cc;
- old-product 登记值与公开资产不符:删行重登,不改 sha。
