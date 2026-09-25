# native/ — 兼容链接层

真源已按板块拆开（见 [`../ARCHITECTURE.md`](../ARCHITECTURE.md)）：

| 链接 | 指向 |
|---|---|
| `compiler_min.c` | `../seed/stage0/compiler_min.c` |
| `ujs_vm.c` · `uxe_asteroid.c` | `../seed/vm/` |
| `ujs_ic_net.c` | `../weights/ujs_ic_net.c` |

新代码请直接写板块路径。
