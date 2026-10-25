# native/ — 兼容链接层

真源已按板块拆开（见 [`../ARCHITECTURE.md`](../ARCHITECTURE.md)）：

| 链接 | 指向 |
|---|---|
| `compiler_min.cx` | `../seed/stage0/compiler_min.cx`（**ujs.com 产品 host**） |
| `ujs_vm.c` · `uxe_asteroid.c` | `../seed/vm/`（实践脊；待迁 `.cx`） |
| `ujs_ic_net.c` | `../weights/ujs_ic_net.c`（生成表；待迁） |

产品构建只走 `unisacc.com` + `.cx`（见 [`../product/`](../product/)）。新代码请直接写板块路径。
