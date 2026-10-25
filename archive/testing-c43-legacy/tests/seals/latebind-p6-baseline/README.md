# P6 基线（latebind-p6-baseline，仓外草稿）

登记前放在 /home/box/c43/P6/，未入仓。输出在 baseline/，由 collect.sh 与 run-all.sh 重新生成（collect.sh 会清空 baseline/，本 README 放在上一级）。

## 产品与来源
- 候选产品：`cand2/unisacc-next.com`，sha256 `83e6e7e4624c1a6d06771af3347f152187468a01893665e0972860961ca2dafb`。
- 源：git 克隆于 HEAD `4a842759`（`git checkout --detach 4a842759`），干净源树，由 `release/tools/build_candidate.sh` 构建（shared、六目标、pack-prep-1..3、pack-models、pack-driver 全部 rc 0）。
- 溯源：`exec/c/provenance.py check cand2/unisacc-next.com` 通过（product freshness verified 83e6e7e4624c1a6d06771af3347f152187468a01893665e0972860961ca2dafb），build.json 记录 commit `4a8427598e5d52f015c803bffba0c63cb19b5e5f`。
- 与 W 候选（fixture 所用）字节相同。4a842759 相对 45424c65 改了 8 个闭包文件（librarytypedpartition 等、libunisacc.c、librarylatebind.h），但没有改变产品字节（实测）。
- 自举器：`src/unisacc.com`（已安装的上一版，sha256 `ba11d636b54adcdc519a81af7553d15ab882ecb85622bdc16968d966acdb8f5f`，build.json 记录 commit 1fe598af），只用于 seed 工具，按 buildcompiler.sh 的设计。
- 同源参考 UA：`ua`（tests/build_ref.sh 从同一导出构建）。

## 采集的两部分（同 26 个源，lnx/x86_64，只用公共 API）
1. **未绑定侧**（collect.sh，p6probe.c）：不注册 provider，用 us_declare_injected_import / us_declare_injected_data；compile 后取 us_library_e3_fragment，relocate（有导入的案例会因 missing-provider 失败，属于设计），取 us_library_lower_unbound、us_library_data_fragment、us_library_data_lower_unbound。产物 `<case>/e3.bin`、`lower.bin`、`datae3.bin`、`datalower.bin`，每个案例的 status.txt 记录所有调用的 rc 与错误。
2. **行为侧**（run-all.sh，p6run.c）：提供 provider（hostadd 用 us_add_symbol，hostvalue 用 us_declare_injected_data + us_register_data_provider），compile、relocate、按 `cases/<case>.calls` 调用导出并记录返回值，调用后读回 hostvalue，最后 us_call_status。产物 `<case>/run.txt`，文件内不含地址。

## 复现
```
python3 -I gen-cases.py          # 26 个源 -> cases/
python3 -I calls.py              # 26 个调用规范 -> cases/*.calls
cd src && cc -std=c11 -O2 -w -I. ../p6probe.c exec/c/libunisacc.c exec/c/librarycall_x86_64.S -lffi -o ../p6probe
cd src && cc -std=c11 -O2 -w -I. ../p6run.c  exec/c/libunisacc.c exec/c/librarycall_x86_64.S -lffi -o ../p6run
cd .. && ./collect.sh && ./run-all.sh    # run-all 追加 run.txt 的清单行
./verify.sh                              # 只读：重算每个产物的 sha256 并比对 manifest
```
脚本哈希：
- fc03c313d78c2cac7f54325df9fce567e37935639c66328035739899318bf4f0  p6probe.c
- 9857434cedd97d347c58dca2f851ecaa01036606d2fc756b41801dad3857a603  p6run.c
- 901a4cd8ab25c59bbce56bd3948f451c22e1a3444a1bb3b1e056e8e0376fafb1  gen-cases.py
- c25a5b039dc0a9f52354ae7fa476bb9ab286fc547226b5099ce7608893eec0b8  calls.py
- 84614f1d912e9f3f61f7b060d5936154186c68b3e49c91ffb67b2424a97f2290  collect.sh
- 90accb718b55ad5229b68110f548bd27fb948e578283ea5f218253b628e78df7  run-all.sh
- 2c23082206b949735a8cd1c9c8b21b5bc916aa75628097d5765b5217d7fde31a  verify.sh

## 案例（26）
- 普通函数：plain-arith、plain-loop、plain-branch、plain-two、plain-const、plain-neg。
- 函数导入（hostadd）：add-const-1..4、add-nested、add-in-func、add-twice-func、add-loop、latebind-answer（源取自 tests/liblatebind2b.c）、mix-latebind-style。
- 数据导入（hostvalue）：data-read、data-write、data-read-twice、data-plus-add、data-in-func-cond、data-write-read、data-and-plain、mix-add-data-plain、mix-two-funcs-data、data-inj-unused。
- librarydata-cases.tsv 是数据类 tape 帧，不是源程序，未用作输入。

## 观测（实测）
- 未绑定侧：26/26 compile rc 0；有导入或数据的案例 relocate rc 1（missing-provider，符合设计）；普通函数 relocate rc 0。E3 片段在函数导入案例中 n=226，纯数据案例 n=0；未绑定 lower 在每个案例中都非空。
- 行为侧：26/26 provider、compile、relocate 与调用都 rc 0，run.txt 中无错误。返回值已逐条手算核对，例如 add-nested=6、add-loop=6、latebind-answer=27、plain-loop answer=45、plain-neg answer=-9、data-write 写回 hostvalue=11、mix-two-funcs-data 写 5 后读回 5、data-and-plain f=11。两次运行的 run.txt 哈希一致。

## 清单
- `baseline/manifest.tsv`：每个产物的 case、文件、字节数、sha256，共 130 行（26×4 个片段 + 26 个 run.txt）。`verify.sh` 只读重算核对，当前 VERIFY-OK 130。
- `baseline/rc.log`：未绑定侧 probe 的退出码（26 条，全为 0）；`baseline/run-rc.log`：行为侧（26 条，全为 0）。

## 限制
- 只有 lnx/x86_64 一个目标；没有 win/arm64、osx 等。
- 26 个案例，由模板生成（不是仓内的完整 latebind 套件）；两个源取自 liblatebind2b.c 的形式。
- 行为侧的 provider 是固定的 hostadd（加法）与 hostvalue（int 5）；不覆盖其它 provider 形状或错误路径（missing-provider 的具名诊断只在未绑定侧出现）。
- 探针的标准输出在 logs/，不在清单内（只是诊断）。
- 未登记：fixture 入仓、gatedeps、derived-map 等等 2c2 接线笔一起做。
