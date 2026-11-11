# coreapi 重复构造优化

结论：最终源码真实默认-run，正常 --check 与原 --fixtures 均实际退出0。正常检查13.874秒，26模块、194个导出符号（基线194），new/missing/signature/alias全0；夹具14.322秒，原8项断言全部通过。两次transport=0、residue=0，计时含coreapi默认-run编译、本轮helper构造和全部查询，不是单纯热查询耗时。

仅修改 tests/coreapi.cx，22814B，SHA256 `1efed0e14b24f87b975d4104373064dfcf1be30343830aec06cf1f473099fa1d`。正常 --check/--write/--init 在原参数/基线验收后，用当前产品一次构造当前 tools/symindex.cx 到 mkdtemp 私有目录，此次运行的每次dump及alias callers查询复用该可执行文件，查询结果仍实时执行、不缓存结论。所有路径通过带真实cap的cx_fmt/join2检查；新增目录纳入原atexit安全删除。没有改动symindex、模块集合、全部exports、alias集合、基线、预算或核心stdout结构。

构造命令为 `sh ABS_PRODUCT -o PRIVATE_DIR/symindex ABS_SOURCE`，子进程上限50秒。查询仅替换原 `sh ABS_PRODUCT ABS_SOURCE --` 前缀为同一新构造native路径；原 --index、--in、dump/calls、--tracked 及其顺序全部保留。native实际执行成功由两次完整检查实证；构造本身不当作功能PASS。helper按运行结束清理，未保留其镜像SHA，材料身份绑定其真实源码与启动产品，不声称跨编译器二进制可复现。

全文件实读确认 prod 仅静态默认值与main内真实仓根绝对路径赋值，base仅由main解析；夹具只切换root来读取临时模块/基线/inlist，无mock产品或symindex。因此首次保守保留夹具重复编译后，按父明确授权将夹具分支也先构造一次同helper；8项断言和输入不变。

实际句柄与材料：

| 作业 | 句柄 | 真实终态 | 材料 |
|---|---:|---|---|
| 最终 --check | 5674 | exit0 / 13874ms | coreapi-final-check.log/.stderr |
| 最终 --fixtures | 22044 | exit0 / 14322ms | coreapi-final-fixtures.log/.stderr |
| 首版正常检查 | 65638 | exit0 / 14170ms | coreapi-check.log/.stderr |
| 保留旧夹具前缀，计时探针 | 53950 | exit142 / 外55秒超时 | coreapi-fixtures.log/.stderr |
| 保留旧夹具前缀，直接原入口控制 | 95217 | exit142 / 外55秒超时，前4断言PASS | coreapi-fixtures-direct.log/.stderr |

原父聚合门24902中coreapi ERROR、隔离67469实际142的两次55秒失败证据仍由父保留；没有用本次绿覆盖历史。上表的两个旧前缀夹具超时也保留。改源严格等待在途句柄终态，未在测试中修改输入。

输入身份见 coreapi-final-v2-identity.tsv：tools/symindex.cx SHA `5af3e15fb941ed68c36fed2090bb59acbff2745ef45f75c285a1b99240197c85`；产品SHA `f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82`；冻结模块表与接口基线SHA与本轮初始身份相同。未执行--write/--init，不改任何基线；它们共享同构造入口，功能本轮仅实际执行--check和--fixtures。

执行外bound55，计时探针的完整coreapi子进程上限50秒，coreapi每个构造/查询子仍50秒，未提高时限。未跑全13门，留父最终集成执行。未提交，无在途句柄。
