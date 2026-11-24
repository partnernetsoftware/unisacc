# 字面量长度修复交接

发布执行两处已独立提交 f5e00ce331865bce4c8608d19d6935f107fff3e4。旧有断言清单的 29 条漏登另提交 7382713af8e3f101e7f38a91cc84f73ea4654012，正式包装入口 na 与 bc 均通过。

## 应用顺序

(1) a-overread.patch 修七处越界读，四个改动文件均在已装产品实编成功。

(2) bc-tool-lengths.patch 修九处工具生产路径长度。

(3) c-check-lengths.patch 修二十三处检查长度。notcovered-census 完整自检零失败。net-deps 实际生成的汇总包含完整的“网=42”，全组自检留待集成。

(4) annotations.patch 只给合法长度加同行例外说明，共 138 个命中。包含 cx/runtime/source_frame.cx、cx/sys/sha.cx、src/drive.cx 三个产品输入路径，仅改注释也会改变源身份，须避开在途候选。

(5) litlen-gate.patch 在以上修复与注解到齐后启用零阈值棘轮，附自检批次、覆盖、清单及工具编译源行。集成时按最新登记与派生门重算产物，再实跑冷布局注册和全棘轮。

## 可复算证据

同一份冻结路径清单，tree-before-annotated.tsv 报 calls=4200 defects=41 need_annotation=0 annotated=138，tree-final.tsv 报 calls=4191 defects=0 need_annotation=0 annotated=138。42xx 是新检查器所支持的调用形状与 Git 普通文件口径，原普查的 4477 为旧工具口径。七处越界读和两处发布修复改成按对象求尺寸，因此移出手写数字的计数分母。

integration-litlen.log 在 0eec9156db 加全部补丁的私有索引上扫描，calls=4203 defects=0 need_annotation=0 annotated=138。formal-litlen.log 记录正式 run-batch 入口 PASS，total_ms=13024。scanner-selftest.log 保留四种调用形状各自的尺寸变异及转义、注解、两项发布回归夹具。ratchets-rows.log 记录棘轮编排与行级输出夹具零失败。

摘要在 SHA256SUMS。整链期间只做定点实跑，未执行全量 derive、全棘轮和其余改动批次，不能据此认定它们通过。
