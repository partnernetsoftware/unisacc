# B.9b publish.sh 运行日志摘要（重建）

> 原 stdout 日志未保留于 `/tmp`；本文件由 observe + `gh release view` + `/tmp/b9b-publish` 公开字节重建，**不是**重跑 `publish.sh`。
> 重建时刻（箱钟 CST）：2026-10-10T18:53:00+08:00
> 回执指针见 `research/r39-release-acceptance.json`（`publish_run.log_path`）。

## 命令（授权原文，已执行过一次）
```
./release/tools/publish.sh v0.0.39 6b2b96803b23fc19c578bb511cddd9434b0c64d7743adc99fa2f6e174eb6c57a research/r39-release-acceptance.json
```
- **rc**：0（observe / 回执 `published.publish_sh`）
- **执行人**：机房主任 grk（cc Stop-hook 队列未即时开干 → 主任直接执行）
- **授权人**：机房主任（B.9b 2026-10-10 18:15；政委 18:05 自决公开）
- **执行人与授权人是否同一人**：**是**（同一角色：机房主任 18:15 授权 + grk/机房主任侧执行）
- **四眼**：缺失（授权人=执行人）；**非阻塞**（本刀事实声明，不重发）

## 脚本侧可观测步骤（来自 observe，非本机重放）
1. 开跑前回执已具备 `release_eligible=true`、courts 绿、owner-promotion authority、`published.public_assets=["unisacc.com"]`、pending B.9b→RESOLVED
2. draft 下载 `unisacc.com` 校验 = 签后 sha `6b2b9680…`
3. 删除非回执资产：`unisacc-unsigned.zip`、`unsigned-receipt.json`
4. `gh release edit v0.0.39 --draft=false --latest`
5. 公开下载再校验 sha = 签后

## 现网 gh 事实（只读核，未改 release）
| 项 | 值 |
|---|---|
| tag | `v0.0.39` |
| isDraft | `false` |
| publishedAt | `2026-10-10T10:16:17Z`（CST 2026-10-10T18:16:17+08:00） |
| URL | https://github.com/partnernetsoftware/unisacc/releases/tag/v0.0.39 |
| targetCommitish | `e31eaddaf99513eb2b7f707f45b1168068098268` |
| assets | unisacc.com |

### assets 明细
- unisacc.com id=RA_kwDOUhBsCM4laK4c size=2259392 createdAt=2026-10-10T10:04:00Z

## 公开字节核验（仓外证据，未改 tag）
- `/tmp/b9b-publish/public-unisacc.com` sha256 = `6b2b96803b23fc19c578bb511cddd9434b0c64d7743adc99fa2f6e174eb6c57a`
- want（签后） = `6b2b96803b23fc19c578bb511cddd9434b0c64d7743adc99fa2f6e174eb6c57a`
- match = **YES**

## 重建依据（仓外）
- `/tmp/unisacc-cdx2/observe-10m/b9b-publish-20261010T101758Z.md` sha256=305ce31ddb838e12497532f0d70a24cf65527c76b0b2ae9e39c42749143102a0
- `/tmp/unisacc-cdx2/observe-10m/b9b-publish-20261010T101839Z.md` sha256=d2c0d945a1c267f1a31a90755dc7180275ba48b8838a768ad996c4bdb106a193
- `/tmp/unisacc-cdx2/workflow-efficiency/b9b-authorize-1815.md` sha256=34166188aaf4ae29da46bcda413edf1a03a9402a1df6856293d2db3db8c49c5a
- 仓内授权：`research/c39-b9b-authorize.md`
- 公开回执祖先：`4c3f25db`（其后误覆盖已 Revert；`published.*` 保持）

## 边界
- **未**重跑 `publish.sh`；**未**改 tag / Draft / Latest；**未**改回执 `published.*` 内容字段
