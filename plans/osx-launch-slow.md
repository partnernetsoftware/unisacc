# OSX-LAUNCH-SLOW：0.0.48 wrapper 修复候选

原发布件为559705097427cf8f4e483bb6e5f702e0714623a81644991c087b17e3505d9032。新候选为01b16ae0a2938268a54fe89f3ec62692b774b391e4baba031401148065c9b15a，6427278字节，版本仍0.0.48；仅替换APE分发脚本，复用原六片、两manager与P4原文。产品源SID仍798321e8919815b8e4a6c5860be5210c8cd702a8cd7c8b280218acb539b011d4。它是下一wrapper候选，不冒称0.0.48.1，也不覆盖已发布v0.0.48。

管理器原始字节SHA前16作为私有.managers目录中的缓存键，临时文件写完后原子发布。只在新解压或marker异常时调用完整manager做摘要检查、最近四载荷清理、活跃lease处理；热路径不执行manager，核marker完整行的name/type/内置完整slice SHA，以noclobber原子新增本次PID lease，并touch现marker维护recency。marker损坏走原管理器验证/隔离fallback；不把同UID缓存当安全信任边界。管理器子目录不是载荷，不承诺总缓存只有四个文件；不对历史manager做自动删除。

首版只缓存manager inode的方案未达到性能门：manager每次重复摘要仍耗CPU。失败候选b2b3f0和日志保留，不迁PASS。最终还合并两次uname为一次，已有缓存目录跳过重复mkdir，保留chmod私有化。

## 实际证据

全部绑定新候选完整SHA。单次各warmup一次，随后version与run各测10次；run夹具为`int main(void){return 7;}`的.c文件。每次保留毫秒、退出、transport与残留；中位取排序后第5/6项均值，门为50/300ms，不删异常样本或放宽门。

| 上下文 | arm64 version/run | Rosetta x64 version/run | 结论 |
|---|---:|---:|---|
| Codex责任应用，无豁免 | 12.5/253.5ms | 48/512.5ms | Rosetta run失败，保留 |
| Terminal.app既有Developer Tools责任应用，正式批 | 15/101.5ms | 48/249ms | 两格通过 |

没有修改系统安全设置或给新产物真签名。Darwin现有`-run`仍每次落临时程序；非豁免环境对该程序首跑的评估是另一个既有开销。同入口旧f069 Rosetta run405.5ms，新裸x64切片388.5ms；本修复不掩盖这项限制，不宣称任意宿主上下文都低于300ms。

正式接口批15/0；冷清理/热复用/热毒化manager不得执行且新增PID lease/两种冷symlink隔离等缓存批两格各45/0。共五个新批实际PASS/rc0/transport0。Rosetta两批platform=osx/arm64，因为是ARM宿主上的强制x64探针；不改run-batch既有宿主门，不冒原生Intel资格。早期错误输出父目录与错误platform导致的REFUSE日志都保留。

日志位于out/osx-launch-slow：formal2-interface/cache-arm64/perf-arm64、formal4-cache-x64/perf-x64；独立测量cold-v3/perf-*与terminal-x64.log。十三棘轮13/13；完整工具库存重算为651源/82分片，仅库存核验通过，不宣称全651源或daily/release全批通过。

## 下一候选与发布边界

新tag使用v0.0.48-osx-launch-rc1，draft=true、prerelease=false。七资产由release-layout构造并严格核验；pkg.bin须保留UNIPKG1 footer，原完整SHAfffa2311…；winARM为41719a8a…不变。新paired record绑定新APE字节、当前SID、wrapper构造提交；原record中ce291旧源摘要不复制给新身份。源码测试提交、wrapper构造提交和原slice来源分账。

新同候选六目标CI必须绑定新APE SHA和精确测试提交，不能沿用559705的42/0。CI未实际完成前仅PREPARED/LOCAL_CHECKED。本轮不安装本机根、不改Latest、不签名或提交公证。若候选失败，保留草稿与证据，修复用新冻结身份；必要时`gh release delete v0.0.48-osx-launch-rc1 --cleanup-tag`撤回本次草稿，原v0.0.48资产保留。
