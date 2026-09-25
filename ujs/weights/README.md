# weights/ — 可复用权重（数据）

只放**产物**，不放生成脚本（脚本在 `../seed/construct/`）。

| 文件 | 说明 |
|---|---|
| `ujs_ic_net.c` | IC 表嵌入；由 `seed/construct/build/ic_c.py` 生成 |

重建：`python3 -c 'from ujs.construct.build.ic_c import emit; print(emit())'`
