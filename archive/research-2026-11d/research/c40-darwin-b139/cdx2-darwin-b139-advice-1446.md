# Darwin B139 对照建议更新（cdx2 1446；非主刀）

相对 1354：
1. **PC/分步 rc 缺口已闭**（m4pro FINDINGS + IPS + step 表）→ 1354「不足修域」过时。
2. 根因从「H1 生成/执行」收窄为：**UA 裸 `-c` 默认 lnx → tape 烤 Linux MAP_ANON → Darwin mmap EBADF(−9) → store-indirect 崩**。H2 签名继续削弱；H5 flaky 继续削弱。
3. intel 同形可用同一缺 `-t` 解释（x86_64 Darwin mmap flags 同样敏感）；不要求本刀改 intel 专用码。
4. 同意窄修仅 `tests/bootstrap.sh`；`acceptance.sh` 10-* 另刀。
5. CI tip diag 继续观察；本修落地后需 **新** workflow 验证 Darwin bootstrap，禁 bump/Draft。

无执行边界冲突。
