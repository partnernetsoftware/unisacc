# Darwin B139（CI bootstrap B=139）材料索引

- 范围：run `38111789528` @ `82e1a9f06e9e39f805e25f0ad285b7306acb9937`；macos-14/15/intel 三红；linux×2 绿。
- 本刀裁定：`ruling-darwin-b139-next-1354.md`（终表刷新 + artifacts 扫 + 诊断规格 + **同 SHA flaky 探针一次**）。
- 前序：`ruling-darwin-b139-cc-lead-1349.md`；取证 `cc-darwin-b139-evidence-1349.md`；假设 `cc-darwin-b139-hypotheses-1349.md`。
- 原红日志（仓外 observe-10m，体积大未入本目录）：
  - macos-14 `cdx2-ci-macos14-firstred-1302.log` sha256 `0f361b4ebdec95c1a2542e109e0a282fe5829d74efbf533b892636ed6227ec1e`
  - macos-15 `cdx-ci-macos15-failure-1314.log` / `cdx2-ci-macos15-firstred-1314.log` sha256 `0b52165f04b4620edb6afac151e7eb66a03a1ef8e658a4c25904b62152bef617`
  - intel `cdx2-ci-intel-firstred-1350.log` sha256 `b2111630ca1ab003cf24522f36f4bcc6551e3d8a7045413ceb5a1bbe1725c409`
- 不构成：产品 PASS、根因闭合、Draft、bump、seal、放宽验收。
