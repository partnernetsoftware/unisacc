# 审核戳 A 独立内容真审（cdx，07:05授权）

基准 e418bdcf→a7069c89。依据 stamp-A-materials-0646.md 全文、exec-delta.diff、固定提交源码及既有 run2/cold1 原账。当前81b86a47相对a706的六审核目录和gatequeue/gatedeps差集为空。仅只读git对象和仓外写本回执；未调用生成器、helper、门禁或复跑inv.py，未改仓。

- [x] 1. 差集：git对象与原JSON差集均只有新增100755 exec/parse2gen/gen-delta.sh、修改100644 exec/pipeline/prepare.sh；新增文件独立成组，已有文件mode未变、无删除。两文件SHA分别cacd2fbbefa84e939be565f97104a671c4527d1e3ddfa8a10852f27de40ca8d8和63474552642c3be637d6627c0d7068e812ec18951d2303791b08ea426e31724d，与材料和受控锁一致。
- [x] 2. prepare语义：唯一可执行变更为parse2的Python调用改成b env SEED_GEN_DIR="$OUT/.seed-gen-cache" sh "$R/exec/parse2gen/gen-delta.sh" "$OUT/e3.json"；其他差集为注释。set -eu下helper非零使准备停止，未引入回退；lex→parse2→opt→prune→pp顺序不变。真实间接路径核过exec/c/compilercheck.sh→exec/pipeline/elf.sh→models.prepare()→subprocess.run(...prepare.sh,cwd=ROOT,check=True)。STAGE_MODELS_READY=1跳过models；models valid缓存为真也跳过prepare。只有实际调用models且无有效缓存才执行此新行，模型closure收该helper和prepare内容。此窄语义可接受，不声称每次compilercheck都会冷准备。
- [x] 3. 新helper全文独立审核：恰一OUT参数，否则rc2；unset SEED_GEN默认1，0显式Python参考exec，空串/其他值rc2。显式非空BIN不可执行rc2；可执行BIN失败的真实rc通过bound/exec保留，未走Python回退。默认key单次Python失败显式exit2；绑定compiler realpath/字节/version stdout/flags及seed源内容，与其余三helper正文相同。缓存需可执行binary+sidecar且sha匹配，否则重建；编译/sha/两次安装失败返回非零，不默默转参考。最终exec bound55 G parse2 OUT，stage准确。prepare传非空私有DIR，覆盖继承DIR；默认TMPDIR尾斜杠归一不改本调用的私有路径。与既有helper同构的有限新增可接受。
- [x] 4. 放置/入口：parse2gen位于exec/parse2/之外；按提交字节及现有glob顺序已独核seedmemory键A=3d19910f75b9932c。exec/c/chain.sh第53行仍是Python parse2，未迁；本差集不改REFERENCE_KEYS或memory证据。键MATCH与seedparse2正式准入实测不混用。
- [x] 5. 接受已列窄限定，非证明其不存在：run2九片和cold1提供本轮参数/停点/字节对拍原账；pg枚举不能签完整清空、TERM未触发、run1超时原因UNKNOWN；RSS未测且无内存准入；默认缓存分支未直接运行；未跑seedparse2准入。并发与binary/sidecar两次mv及核sha→exec窗口未测，不宣称原子性或强工具身份；16位key、不绑cc1/as/ld、显式BIN不走键检验的既有边界保留。helper失败不保证删掉已有/部分OUT，prepare失败不得当成功产物。此审不称提速、六目标端到端、精确闭包/门绿或schema全齐。
- [x] 6. 只读独立重算候选：git ls-tree -r -z a7069c89 -- exec列不可变对象，以该提交compilercheck inventory_suffixes/excluded_dirs筛选、cat-file读取blob、按执行位归一100755/100644及SHA256，json.dumps(sort_keys=True)再SHA256。得到1412成员，成员字典与inv-a7069c89.json完全相同，候选全长1d0e0c5cf47ba3befd18d391491cb24d343dfc242aad7993a454ee57557f1825。只记候选；记录旧键仍fbb129700659fe8f225eb53a64fb384898f118b405d0a98f16ef2221bdd56336，未写gatedeps，未执行条件式单键。旧fbb认键CLOSED不重开，新键写入另裁。

本审勾选只落本回执，材料作者原清单未改。不存在本审授权刷戳/跑门/bump/Draft/扩产品切口的含义。

PASS 可进刷戳裁
