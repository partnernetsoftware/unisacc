# CI 里的 unisacc.com：固定已发布版本（2026-10-30，cc 裁定）

## 裁定

`unisacc.com` 被 .gitignore 忽略，检出里没有，原先四个 workflow 的 CX 步骤因此都跑不起来：ci.yml、admission-capsule.yml、windows-signing.yml 的两个 job。

现在统一在 checkout 之后调用 `./.github/actions/bootstrap-unisacc`。它按 `release/ci/bootstrap-compiler.tsv` 里钉死的 tag，用 `gh release download` 取已发布的 unisacc.com，核对 sha256 后放到 `./unisacc.com`。

## 为什么不在 CI 里从源码构建

产品构建（tools/product-build.cx、hostseed-step）本身就要一个能运行的 unisacc.com，再加上宿主 C 引导，属于慢车道里的重活。用已发布的二进制当启动编译器，身份由 sha256 钉住，CI 里没有构建步骤。这和 AGENTS.md 的"当前产品可以作新工具的启动编译器"一致。

## 现状（实测）

- v0.0.42 是最新发布版（2026-10-14）。它比 `#pragma unisa module/use` 早：拿 tools/check-cx-text.cx 去跑，报 `undefined function 'tx_prefixz'` 等 11 个错误。所以钉选行现在是 `none`，这几个 CX 步骤会带明确的 `::error::` 失败退出，不会静默跳过。
- 本机演练了 action 的脚本，三条路径都按预期：
  - 钉 v0.0.42 及其真实 sha256：下载成功并加上可执行位；
  - `none`：报错，rc=1；
  - sha 不符：报错，rc=1。
- 没在 GitHub 上实跑过。

## 0.0.43 发布后要做的一步

把 v0.0.43 和它的 unisacc.com 资产 sha256 填进 `release/ci/bootstrap-compiler.tsv`。之后每次发版：确认新版本仍能运行仓内 CX 步骤，再更新这一行。
