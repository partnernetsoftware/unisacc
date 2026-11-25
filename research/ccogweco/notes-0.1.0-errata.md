# 0.1.0 说明公开后更正稿(预备;2026-11-25 ccogweco)

状态:预备稿,等 Step 5 公开后立即应用到 research/ccogweco/notes-0.1.0-final.md(正文冻结版 sha 199742ba),
不改已发布草稿正文;公开后由 agy-flow 在 Y2 后用 gh release edit 更新公开正文并回读,同时随 release-post 提交。
理由:更正不触资产、SID 与候选字节;用户目标 ASAP,不阻塞 Step 5。

## 1 两处更正(逐行)

### (1) CI 耗时(第 46 行附近)

- 原文:约 **8.95 分钟**(0.0.107 为 9.06 分钟)。
- 更正:2026-11-25T03:41:01Z -> 03:50:30Z = **9 分 29 秒(9.48 分钟)**,0.0.107 为 9.06 分钟;
  本车比上一版多 **0.42 分钟(约 25 秒)**。

### (2) 准入条件 2 的 UNKNOWN 描述(第 66 行附近)

- 原文(模板式):"strict 以当轮 SUMMARY 为准,UNKNOWN 只允许已登记无生产者项"。
- 更正:"strict 以当轮 SUMMARY 为准;**UNKNOWN 仅限已登记且证据可核项:107 与 0.1.0 均为
  `precommit-list-cap` 的 window-timeout**(本车 S5 计数:520 PASS、0 FAIL、1 UNKNOWN、527 RAW、superseded=6、ratchets 19/0)。"

## 2 文末新增小节(公开后更正)

    公开后更正(2026-11-25)
    - CI 耗时原写 8.95 分钟,实为 9 分 29 秒(9.48 分钟);0.0.107 为 9.06 分钟,本车多约 0.42 分钟。
    - 准入条件 2 的 UNKNOWN 描述原为模板句,实为 precommit-list-cap 的 window-timeout(107 与 0.1.0 同);
      不改变 1 UNKNOWN 计数与披露口径。
    - 更正不触资产、SID、候选字节与 strict 计数。

## 3 应用与回执

- 公开后由 ccogweco 用 devsvc f edit 落两处 + 文末小节,报新 sha;
- agy-flow 以新 sha 更新公开正文并回读,Y2 后随 release-post 提交 release/notes-0.1.0.md;
- cdx 核更正后字节(正文两处 + 小节,资产与 INDEX 不变)。

## 4 边界

- 公开前不动冻结正文(sha 199742ba);本页只预备。
- 不把更正写成"未发生";保留原句与更正小节两处痕。
