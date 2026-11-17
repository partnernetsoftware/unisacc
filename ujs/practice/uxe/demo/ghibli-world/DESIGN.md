# ghibli-world(需求稿,未实现)

真源需求在 [`ujs/prd.md`](../../../prd.md) §0.3 D5。本文件只锁实现前未决项。

## 与 D1-D4 的关系

同一套:`.ujs` 世界+`step`,`host1Boot`,`host_gpu_submit`,无 three。复用 ink 分层/NPR、water 雾与 pal、fly 天空镜头、oneshot 实例云。不是新引擎。

## 锁

(1) 页路径:`practice/uxe/demo/ghibli-world/`
(2) 核:`tests/examples/ghibli.ujs`(未写)
(3) AUTO 标题待锁(暂不占 `GHIBLI_OK`)
(4) 种子:`seed` 整数全局;同种子同布局
(5) Host 不另写噪声当真值;不搬 GLB;不升 `ujs.com`;不改 host1-pass 28

## 未写

`.ujs`、wasm、index.html、表长数字。P2 未验收前本页不宣称可玩。
