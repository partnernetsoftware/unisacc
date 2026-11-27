本表一行一个 plans/slice-units.tsv 单元, 共 99 行.
unit 是单元名. dest_class 只取 plans/migration-order.tsv 已有值, 没有的行记 UNMEASURED.
path: plans/startup-path-top15.tsv 里 startup=yes 的记 startup, 本任务规则下其余记 hot. 该表只测了 top 15, 只有 m0.o 为 yes.
live_bytes 是 v0.1.7 链 kept-bytes 的 kept_bytes. obj_bytes 是同表 obj_text. 99 个单元都有这两列.
size_delta 与 source_kind 在现有事实表里没有对应列, 记 UNMEASURED, 不从字节差或 override 推.
golden_kind, golden_cases, golden_pass, calls_per_run, steps_per_call, budget_us, bridge_shape, blockers_n 未测, 一律 UNMEASURED.
cost_us_per_run 由检查规格从调用次数推导, 不手填, 留空.
verdict 全部 PENDING. veto_reason 不填, 不发明否决原因.
尺寸只记录, 不作准入否决. 单单元净降不足 2,000 B 就回退的旧规则已撤回.
未测项保持 UNMEASURED, 直到有同候选身份的实测再改.
