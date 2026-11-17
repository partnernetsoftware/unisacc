# ghibli-world(草稿,观感合同未达)

真源需求在 [`ujs/prd.md`](../../../prd.md) §0.3 D5。

## 已锁

(1) 页:`practice/uxe/demo/ghibli-world/`
(2) 核:`tests/examples/ghibli.ujs` 返回 `{d,wind}`;`d.length>=8`
(3) AUTO 标题:`GHIBLI_OK`
(4) `?seed=` 进核;`wind` 每步 +1
(5) Host 层:地、近丘、远山、云、草;无 three、不搬 GLB
(6) 盒树:轴对齐盒作树干/冠,不引入 mesh 资产
(7) WASD 高度:W/S 改观察高度,A/D 水平绕;不接 three 相机
(8) `?seed=` 为核输入唯一入口;缺省用页内默认种子,非法值回落默认
(9) ink / wave / 雾 / 昼夜随 `wind` 驱动(同核步进);Host 只读 `wind`,不另开时钟

## 未达

宫崎风水彩观感合同未达:纸感晕染、墨线与水色层次、人工 10 秒可辨风层。昼夜随 `wind` 的驱动已锁,观感验收未过。P2 未验收。不宣称可玩完成。
