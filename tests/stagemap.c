/* stagemap.c -- the compiler-stage axis of the gate (run with: unisacc.com -run tests/stagemap.c).
 *
 * gatelayers says how broad a suite's claim is; this says which stage it tests, so a change to
 * one stage runs that stage's suites first and the whole chain after.  Stages follow prd.md
 * section 3: E2 pp, E1 lex, E3 parse/type/tape, E4 opt+prune, lower, E5 enc, E6 image/load/runtime.
 * Suites about no single stage go in: chain (whole route vs a reference), lib (shipped library and
 * host interop), build (seed, self-hosting, packaging), meta (ledgers, inventories, runner rules).
 * The table only labels; it never removes a suite.  A suite no rule matches is an error.  `also`
 * marks additional stages; primary-stage filtering remains unchanged.
 * --list wire: primary<TAB>name<TAB>also; also is a lexicographically sorted, unique
 * comma-separated set without whitespace; no values is "-"; one value is one item.
 *
 * Suite names come on stdin (tests/gate.sh --list --com).
 *   --check           every name has a stage; print counts
 *   --stage S [...]   print the names in these stages
 *   --list            print stage, name, also
 *   --overlaps        print names with also-stages
 *   --stages          print the registered stage vocabulary (no stdin needed)
 *   --unmapped        print the explicit unresolved whitelist (currently empty)
 * Patterns: '*' matches any run; a name is tried as is, without a trailing -N shard, and without
 * a -lnx-/-osx-/-win- target suffix; a com- prefix is the product route of the same suite.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char *rules[] = {
    "meta", "identity-product-negative identity-include-negative identity-include-consumer-negative identity-prep-negative c99-ledger decision-ledger ledgercheck finite-template dsl-ops manifest-entries-* script-inventory"
            " subtract-safety* freezecheck revivedscan facts-export publish-order gate-infra gate-layers"
            " tsv-build-account pipeline-cache docs kernel source-layout front-bounds bound boundreason boundstall-r7g boundnested-1 boundnested-2 boundnested-3 boundnested-4 boundnested-5 fresh-order*"
            " modelbenchcheck package-footer ape-version proc-enum qprefix full-signature referee-* tools"
            " lib-bindings-registry",
    "E2", "exec-pp-* exec-pploc exec-macros pptruth shared-e2-* rowcov-pp* prevheaders",
    "E1", "exec-lex* exec-f1-sidecar strconvert-* rowcov-lex*",
    "E3", "exec-parse* exec-r21-e3 exec-unit* exec-diag exec-errors* exec-f1-attributes exec-*warn*"
          " exec-neg exec-e3self exec-decimal tapebin* tape-reader rowcov-parse2* declmatrix declshape"
          " volatile-comma diag* c99 staticinit staticunits tagforward parserbounds hostile cli ccparity"
          " formatonce warn* seed-construct-parse2 seedparse2*",
    "E4", "opt-run exec-e4* exec-prune*",
    "lower", "exec-lower exec-armlower exec-winlower exec-lowdata exec-sparse rowcov-lower* forward*"
             " ccinterop ffi-* exec-bridge-linux windows-resolver-host exec-bind* libneed",
    "E5", "exec-e5 exec-arm exec-sha exec-asm* rowcov-enc* asmtext combo exec-memory-asm exec-memx86-asm",
    "E6", "exec-elf* exec-armelf exec-mach* exec-x86win exec-armwin exec-pe* exec-object* exec-modelobject"
          " elfobj linkunits run fat exec-memory-* exec-memx86-* exec-memwin* exec-binaryio exec-crcllp64*"
          " malloc memalign hosthdr syscall6 winposix auditnet exec-container exec-package exec-net"
          " exec-codec exec-core exec-embedded target-package lifecycle exec-srcelf",
    "chain", "difftest* csmithdiff* corpus* ccrun* closure* stages exec-chain exec-native-* exec-formats*"
             " exec-multi-* multi fb12-multi realprog luatests* sqlspeed* minicon apps-* bigclosure",
    "lib", "lib-*",
    "build", "host-triple-* seed* exec-*self* exec-selfprep-* exec-bootstrap-* nativeboot-* comboot-* exec-driver-*"
             " exec-tableself exec-prepare-*",
    0
};
/* Exact additions are the sole stage records for formerly unclassified suites.
 * Per-row basis is retained below; no side mapping file is consumed at runtime.
 * The 13 owner-reviewed cases are resolved (bits: C reference migration -> build).
 */
static char *extra[] = {
    /* Existing model-cache producer plus native UA artifacts; nativecheck.sh --prepare. */
    "build", "exec-native-resources-prep", "-",
    /* Same six-target closure assertion, first lane split by source workload; tests/gate.sh. */
    "chain", "stages-1a", "-",
    "chain", "stages-1b", "-",
    "chain", "stages-1c", "-",
    "chain", "stages-2a", "-",
    "chain", "stages-2b", "-",
    "chain", "stages-2c", "-",
    "chain", "stages-3a", "-",
    "chain", "stages-3b", "-",
    "chain", "stages-3c", "-",
    /* oracle-cache: 全阶段ask漏斗inf()（DENSE缓存）双向全域对拍infer()权重直算（A-49）; tests/gate.sh:352 */
    "chain", "oracle-cache", "meta",
    /* stages-back: 全程构建的六个后端网（opinfo/peep/abi/regmap/reloc/enc）「Python问过则C必问过」（A-33全程扩展）; tests/gate.sh:353 */
    "chain", "stages-back", "-",
    "build", "closure-c4a-prepare closure-c4b-prepare", "-",
    "build", "closure-c3a-prepare closure-c3b-prepare closure-c3c-prepare", "-",
    "build", "closure-c4c-prepare", "-",
    "chain", "closure-c1a", "build",
    "chain", "closure-c1b", "build",
    "chain", "closure-c1c", "build",
    /* hostcheck: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:218 */
    "meta", "hostcheck", "-",
    /* copy-contract: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:221 */
    "meta", "copy-contract", "-",
    /* ri-snapshot-overlay: E3封存输出+资源overlay/capture，跨阶段适配不是单个δ; tests/gate.sh:222 */
    "chain", "ri-snapshot-overlay", "E3,lower",
    /* ri-snapshot-core: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:223 */
    "meta", "ri-snapshot-core", "-",
    /* watchdog-orphan: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:225 */
    "meta", "watchdog-orphan", "-",
    /* watchdog-eof: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:226 */
    "meta", "watchdog-eof", "-",
    /* watchdog-success: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:227 */
    "meta", "watchdog-success", "-",
    /* stagelog: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:228 */
    "meta", "stagelog", "-",
    /* exittable: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:229 */
    "meta", "exittable", "-",
    /* gatequeue-symlink: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:230 */
    "meta", "gatequeue-symlink", "-",
    /* h1source: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:231 */
    "meta", "h1source", "-",
    /* attemptchain: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:232 */
    "meta", "attemptchain", "-",
    /* c99precheck: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:233 */
    "meta", "c99precheck", "-",
    /* rulingwait: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:234 */
    "meta", "rulingwait", "-",
    /* namedrefuse: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:235 */
    "meta", "namedrefuse", "-",
    /* checkrun: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:236 */
    "meta", "checkrun", "-",
    /* commitgate: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:237 */
    "meta", "commitgate", "-",
    /* k2b: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:238 */
    "meta", "k2b", "-",
    /* precommithook: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:239 */
    "meta", "precommithook", "-",
    /* queuestart: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:240 */
    "meta", "queuestart", "-",
    /* queuestart-strict: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:241 */
    "meta", "queuestart-strict", "-",
    /* envpin: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:242 */
    "meta", "envpin", "-",
    /* queuestart-launch: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:243 */
    "meta", "queuestart-launch", "-",
    /* queuetimeout: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:244 */
    "meta", "queuetimeout", "-",
    /* gatesummary: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:245 */
    "meta", "gatesummary", "-",
    /* observation: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:246 */
    "meta", "observation", "-",
    /* gqalive: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:247 */
    "meta", "gqalive", "-",
    /* queuetimer: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:248 */
    "meta", "queuetimer", "-",
    /* docedit: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:249 */
    "meta", "docedit", "-",
    /* whyrerun: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:250 */
    "meta", "whyrerun", "-",
    /* draftfetch: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:251 */
    "meta", "draftfetch", "-",
    /* draftguard: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:252 */
    "meta", "draftguard", "-",
    /* warmup: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:253 */
    "meta", "warmup", "-",
    /* installpair: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:254 */
    "meta", "installpair", "-",
    /* staticimage: 读产品ELF程序头、PT_INTERP/PT_DYNAMIC拒；不执行镜像; tests/gate.sh:355 */
    "E6", "staticimage", "meta",
    /* staticimage-selftest: ELF程序头正负例; tests/gate.sh:356 */
    "E6", "staticimage-selftest", "meta",
    /* abi-manifest: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:357 */
    "meta", "abi-manifest", "-",
    /* abi-manifest-selftest: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:358 */
    "meta", "abi-manifest-selftest", "-",
    /* early-stage-seal-selftest: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:359 */
    "meta", "early-stage-seal-selftest", "-",
    /* pp-leaf-seal: 封存PP流/构造依赖核验; tests/gate.sh:360 */
    "E2", "pp-leaf-seal", "meta",
    /* pp-leaf-seal-negative: PP封存与漂移负例; tests/gate.sh:361 */
    "E2", "pp-leaf-seal-negative", "meta",
    /* pp-leaf-capture-selftest: PP捕获格式自检，不是完整PP阶段; tests/gate.sh:362 */
    "E2", "pp-leaf-capture-selftest", "meta",
    /* lex-e1-seal: 封存lex流（e1，pp.text录制输入）回放与构造依赖核验，含负例; tests/gate.sh:453 */
    "E1", "lex-e1-seal", "meta",
    /* e3-seal: 封存e3测量叶（tokens.typed->tape.text，上游lex夹具）回放与构造依赖核验，含负例; tests/gate.sh */
    "E3", "e3-seal", "meta",
    /* opt-e4-seal: 封存opt流（e4，-O0 tape录制输入）回放与构造依赖核验，含负例; tests/gate.sh:454 */
    "E4", "opt-e4-seal", "meta",
    /* elf-seal: 封存elf流（enc --elf，lnx/x86_64，target.text录制输入）回放与构造依赖核验，含负例; tests/gate.sh:455 */
    "E6", "elf-seal", "meta",
    /* prune-seal: 封存prune流（gen.py prune，-O2 tape录制输入）回放与构造依赖核验，含负例; tests/gate.sh:456 */
    "E4", "prune-seal", "meta",
    /* lower-seal: 封存lower测量叶（tape.text->target.text）回放与构造依赖核验，含负例; tests/gate.sh */
    "lower", "lower-seal", "meta",
    /* seal-modules: manifest+xy/edges 结构（--check-only）；回放由 *-seal 单叶 job；tests/gate.sh */
    "chain", "seal-modules", "E1,E3,E4,E6,meta",
    /* seal-paradigm-audit: 流/网范式登记结构（manifest+xy+edges，--audit，.cx）；tests/gate.sh:461 */
    "chain", "seal-paradigm-audit", "E1,E3,E4,E6,meta",
    /* seal-paradigm-selftest: 范式登记审计器正负例自测; tests/gate.sh:462 */
    "meta", "seal-paradigm-selftest", "-",
    /* edge-e4-peep-seal: 边级冻结原件（e4->peep 规范化问答面回放），tests/edgeseal.py; tests/gate.sh:460 */
    "E4", "edge-e4-peep-seal", "meta",
    /* edge-e3-pfconv-witness: 产品侧边级见证（-nostdinc 内建 printf 路径），tests/edgewitness.sh */
    "E3", "edge-e3-pfconv-witness", "meta",
    /* edge-elf-enc-witness: 产品侧边级见证（elf 流 enc ask），tests/edgewitness.sh */
    "E6", "edge-elf-enc-witness", "meta",
    /* edge-e2-pp-seal: 边级冻结原件（e2->pp），同 edge-e4-peep-seal; tests/edgeseal.py */
    "E2", "edge-e2-pp-seal", "meta",
    /* edge-e1-lex-seal: 边级冻结原件（e1->lex），同 edge-e4-peep-seal; tests/edgeseal.py */
    "E1", "edge-e1-lex-seal", "meta",
    /* edge-e3-parse-seal: 边级冻结原件（e3->parse），同 edge-e4-peep-seal; tests/edgeseal.py */
    "E3", "edge-e3-parse-seal", "meta",
    /* edge-e3-type-seal: 边级冻结原件（e3->type），同 edge-e4-peep-seal; tests/edgeseal.py */
    "E3", "edge-e3-type-seal", "meta",
    /* edge-e3-scope-seal: 边级冻结原件（e3->scope），同 edge-e4-peep-seal; tests/edgeseal.py */
    "E3", "edge-e3-scope-seal", "meta",
    /* edge-e3-irsel-seal: 边级冻结原件（e3->irsel），同 edge-e4-peep-seal; tests/edgeseal.py */
    "E3", "edge-e3-irsel-seal", "meta",
    /* edge-e3-tyinfo-seal: 边级冻结原件（e3->tyinfo），同 edge-e4-peep-seal; tests/edgeseal.py */
    "E3", "edge-e3-tyinfo-seal", "meta",
    /* edge-e3-binsel-seal: 边级冻结原件（e3->binsel），同 edge-e4-peep-seal; tests/edgeseal.py */
    "E3", "edge-e3-binsel-seal", "meta",
    /* edge-e3-prec-seal: 边级冻结原件（e3->prec），同 edge-e4-peep-seal; tests/edgeseal.py */
    "E3", "edge-e3-prec-seal", "meta",
    /* edge-e4-opinfo-seal: 边级冻结原件（e4->opinfo），同 edge-e4-peep-seal; tests/edgeseal.py */
    "E4", "edge-e4-opinfo-seal", "meta",
    /* edge-lower-abi-seal: 边级冻结原件（lower->abi），同 edge-e4-peep-seal; tests/edgeseal.py */
    "lower", "edge-lower-abi-seal", "meta",
    /* edge-lower-regmap-seal: 边级冻结原件（lower->regmap），同 edge-e4-peep-seal; tests/edgeseal.py */
    "lower", "edge-lower-regmap-seal", "meta",
    /* edge-lower-reloc-seal: 边级冻结原件（lower->reloc），同 edge-e4-peep-seal; tests/edgeseal.py */
    "lower", "edge-lower-reloc-seal", "meta",
    /* edge-lower-enc-seal: 边级冻结原件（lower->enc），同 edge-e4-peep-seal; tests/edgeseal.py */
    "lower", "edge-lower-enc-seal", "meta",
    /* net-prec-seal: 封存单网prec（内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:457 */
    "E3", "net-prec-seal", "meta",
    /* net-binsel-seal: 封存单网binsel（C二元运算符+符号性→ALU flavour；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:458 */
    "E3", "net-binsel-seal", "meta",
    /* net-tyinfo-seal: 封存单网tyinfo（标量类型→size/uns/narrow三头；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:459 */
    "E3", "net-tyinfo-seal", "meta",
    /* net-pfconv-seal: 封存单网pfconv（printf转换字母→打印例程；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:460 */
    "E3", "net-pfconv-seal", "meta",
    /* net-regmap-seal: 封存单网regmap（tape寄存器+arch→机器寄存器；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:461 */
    "lower", "net-regmap-seal", "meta",
    /* net-lex-seal: 封存单网lex（字符类+前瞻类→词法动作；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:462 */
    "E1", "net-lex-seal", "meta",
    /* net-parse-seal: 封存单网parse（非终结符+前瞻token→产生式；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:463 */
    "E3", "net-parse-seal", "meta",
    /* net-type-seal: 封存单网type（t1 op t2→结果类型；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:464 */
    "E3", "net-type-seal", "meta",
    /* net-scope-seal: 封存单网scope（作用域上下文+声明种类→符号表动作；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:465 */
    "E3", "net-scope-seal", "meta",
    /* net-irsel-seal: 封存单网irsel（IR family+flavor→下降配方；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:466 */
    "E3", "net-irsel-seal", "meta",
    /* net-isel-seal: 封存单网isel（tape op+arch→指令form与symbol两头；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:467 */
    "lower", "net-isel-seal", "meta",
    /* net-enc-seal: 封存单网enc（tape op+os+arch→编码form；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:468 */
    "E5", "net-enc-seal", "meta",
    /* net-reloc-seal: 封存单网reloc（跳转种类+arch→重定位种类；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:469 */
    "lower", "net-reloc-seal", "meta",
    /* net-abi-seal: 封存单网abi（tape op+os+arch→ABI十三头；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:470 */
    "lower", "net-abi-seal", "meta",
    /* net-combo-seal: 封存单网combo（tape op+os+arch→isel+ABI十五头；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:471 */
    "lower", "net-combo-seal", "meta",
    /* net-opinfo-seal: 封存单网opinfo（tape op→simple/acls/bcls三头；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:472 */
    "E4", "net-opinfo-seal", "meta",
    /* net-peep-seal: 封存单网peep（指令类对+关系→窥孔动作；内核+该网权重字面量，金表全域回放）与构造依赖核验，含负例; tests/gate.sh:473 */
    "E4", "net-peep-seal", "meta",
    /* middle-flow-e3: --sealed-e3严格回放及拒绝; tests/gate.sh:363 */
    "E3", "middle-flow-e3", "meta",
    /* middle-flow-lower-accept: --sealed-lower接受投影回放; tests/gate.sh:364 */
    "lower", "middle-flow-lower-accept", "meta",
    /* middle-flow-ledger: middleflowcheck.cx ledger函数逐capture/capture-lower对sealed账本; tests/gate.sh:366 */
    "chain", "middle-flow-ledger", "E3,lower,meta",
    /* stage-contract-e1e3: 两fixture:stagecontract-e1-e3与stagecontract-tape-lower，SC1边界/资源/重置; tests/gate.sh:367 */
    "chain", "stage-contract-e1e3", "E1,E3,lower,meta",
    /* tapecheck-model-1: Tape验域模型边界；下游lower可继续拒目标能力; UNKNOWN:动态job需plan回查 */
    "E3", "tapecheck-model-1", "lower",
    /* tapecheck-model-2: Tape验域模型边界；下游lower可继续拒目标能力; UNKNOWN:动态job需plan回查 */
    "E3", "tapecheck-model-2", "lower",
    /* tapecheck-model-3: Tape验域模型边界；下游lower可继续拒目标能力; UNKNOWN:动态job需plan回查 */
    "E3", "tapecheck-model-3", "lower",
    /* exec-hostsymbols-model: 无地址hostsymbol合同模型，非新编译大阶段; tests/gate.sh:369 */
    "lib", "exec-hostsymbols-model", "meta",
    /* exec-librarycontract-model: check.sh:2明确2a model only，声明模型不是E3流水线; tests/gate.sh:370 */
    "lib", "exec-librarycontract-model", "meta",
    /* exec-librarypartition-model: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:371 */
    "lib", "exec-librarypartition-model", "meta",
    /* exec-librarytypedpartition-model-1: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:372 */
    "lib", "exec-librarytypedpartition-model-1", "meta",
    /* exec-librarytypedpartition-model-2: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:373 */
    "lib", "exec-librarytypedpartition-model-2", "meta",
    /* exec-librarytypedpartition-model-3: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:374 */
    "lib", "exec-librarytypedpartition-model-3", "meta",
    /* exec-librarytypedpartition-model-4: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:375 */
    "lib", "exec-librarytypedpartition-model-4", "meta",
    /* exec-librarycallsitepartition-model-1: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:376 */
    "lib", "exec-librarycallsitepartition-model-1", "meta",
    /* exec-librarycallsitepartition-model-2: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:377 */
    "lib", "exec-librarycallsitepartition-model-2", "meta",
    /* exec-librarycallsitepartition-model-3: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:378 */
    "lib", "exec-librarycallsitepartition-model-3", "meta",
    /* exec-librarycallsitepartition-model-4: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:379 */
    "lib", "exec-librarycallsitepartition-model-4", "meta",
    /* exec-librarycallsitepartition-model-5: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:380 */
    "lib", "exec-librarycallsitepartition-model-5", "meta",
    /* exec-librarycallsitepartition-model-6: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:381 */
    "lib", "exec-librarycallsitepartition-model-6", "meta",
    /* exec-librarycallsitepartition-model-7: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:382 */
    "lib", "exec-librarycallsitepartition-model-7", "meta",
    /* exec-librarycallsitepartition-model-8: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:383 */
    "lib", "exec-librarycallsitepartition-model-8", "meta",
    /* exec-librarycallbackpartition-model-1: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:384 */
    "lib", "exec-librarycallbackpartition-model-1", "meta",
    /* exec-librarycallbackpartition-model-2: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:385 */
    "lib", "exec-librarycallbackpartition-model-2", "meta",
    /* exec-librarycallbackpartition-model-3: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:386 */
    "lib", "exec-librarycallbackpartition-model-3", "meta",
    /* exec-librarycallbackpartition-model-4: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:387 */
    "lib", "exec-librarycallbackpartition-model-4", "meta",
    /* exec-librarycallbackpartition-model-5: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:388 */
    "lib", "exec-librarycallbackpartition-model-5", "meta",
    /* exec-librarycallbackpartition-model-6: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:389 */
    "lib", "exec-librarycallbackpartition-model-6", "meta",
    /* exec-librarycallbackpartition-model-7: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:390 */
    "lib", "exec-librarycallbackpartition-model-7", "meta",
    /* exec-librarycallbackpartition-model-8: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:391 */
    "lib", "exec-librarycallbackpartition-model-8", "meta",
    /* exec-librarycallbackpartition-model-9: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:392 */
    "lib", "exec-librarycallbackpartition-model-9", "meta",
    /* exec-librarycallbackpartition-model-10: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:393 */
    "lib", "exec-librarycallbackpartition-model-10", "meta",
    /* exec-librarycallbackpartition-model-11: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:394 */
    "lib", "exec-librarycallbackpartition-model-11", "meta",
    /* exec-librarycallbackpartition-model-12: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:395 */
    "lib", "exec-librarycallbackpartition-model-12", "meta",
    /* exec-librarycallbackpartition-model-13: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:396 */
    "lib", "exec-librarycallbackpartition-model-13", "meta",
    /* exec-librarycallbackpartition-model-14: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:397 */
    "lib", "exec-librarycallbackpartition-model-14", "meta",
    /* exec-librarycallbackpartition-model-15: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:398 */
    "lib", "exec-librarycallbackpartition-model-15", "meta",
    /* exec-librarycallbackpartition-model-16: gate入口独立partition/check.sh，构造对应/gen、C/P和分类穷举；归属选择不是新大阶段; tests/gate.sh:399 */
    "lib", "exec-librarycallbackpartition-model-16", "meta",
    /* exec-libraryfunctionimports-model: 入口exec/parse2/libraryfunctionimportscheck.sh; tests/gate.sh:400 */
    "E3", "exec-libraryfunctionimports-model", "lib",
    /* exec-librarytypedreader-model: 入口parse2/librarytypedreadercheck.sh构造parse2/librarytyped; tests/gate.sh:401 */
    "E3", "exec-librarytypedreader-model", "lib",
    /* exec-librarycallback-reader-model: librarycallbacke3check.sh构造parse2/librarycallbacke3并严格ledger回放; tests/gate.sh:402 */
    "E3", "exec-librarycallback-reader-model", "lib",
    /* exec-librarycallbacklower-model-1: gate入口lower专属check.sh或callback lower wrapper，构造lower模型并比较片段/拒绝; tests/gate.sh:403 */
    "lower", "exec-librarycallbacklower-model-1", "lib",
    /* exec-librarycallbacklower-model-2: gate入口lower专属check.sh或callback lower wrapper，构造lower模型并比较片段/拒绝; tests/gate.sh:404 */
    "lower", "exec-librarycallbacklower-model-2", "lib",
    /* exec-librarycallbacklower-model-3: gate入口lower专属check.sh或callback lower wrapper，构造lower模型并比较片段/拒绝; tests/gate.sh:405 */
    "lower", "exec-librarycallbacklower-model-3", "lib",
    /* exec-librarycallbacklower-model-4: gate入口lower专属check.sh或callback lower wrapper，构造lower模型并比较片段/拒绝; tests/gate.sh:406 */
    "lower", "exec-librarycallbacklower-model-4", "lib",
    /* exec-librarycallback-wiring-prepare: 真实API/W prepare/rc/释放计数；不是provider桥运行; tests/gate.sh:408 */
    "lib", "exec-librarycallback-wiring-prepare", "meta",
    /* exec-librarycallsite-reader-model: 入口parse2/librarycallsitecheck.sh; tests/gate.sh:409 */
    "E3", "exec-librarycallsite-reader-model", "lib",
    /* exec-librarycallsitelower-model-1: gate入口lower专属check.sh或callback lower wrapper，构造lower模型并比较片段/拒绝; tests/gate.sh:410 */
    "lower", "exec-librarycallsitelower-model-1", "lib",
    /* exec-librarycallsitelower-model-2: gate入口lower专属check.sh或callback lower wrapper，构造lower模型并比较片段/拒绝; tests/gate.sh:411 */
    "lower", "exec-librarycallsitelower-model-2", "lib",
    /* exec-librarycallsitelower-model-3: gate入口lower专属check.sh或callback lower wrapper，构造lower模型并比较片段/拒绝; tests/gate.sh:412 */
    "lower", "exec-librarycallsitelower-model-3", "lib",
    /* exec-librarycallsitelower-model-4: gate入口lower专属check.sh或callback lower wrapper，构造lower模型并比较片段/拒绝; tests/gate.sh:413 */
    "lower", "exec-librarycallsitelower-model-4", "lib",
    /* exec-librarytypedlower-model-1: gate入口lower专属check.sh或callback lower wrapper，构造lower模型并比较片段/拒绝; tests/gate.sh:414 */
    "lower", "exec-librarytypedlower-model-1", "lib",
    /* exec-librarytypedlower-model-2: gate入口lower专属check.sh或callback lower wrapper，构造lower模型并比较片段/拒绝; tests/gate.sh:415 */
    "lower", "exec-librarytypedlower-model-2", "lib",
    /* exec-librarytypedlower-model-3: gate入口lower专属check.sh或callback lower wrapper，构造lower模型并比较片段/拒绝; tests/gate.sh:416 */
    "lower", "exec-librarytypedlower-model-3", "lib",
    /* exec-librarytypedlower-model-4: gate入口lower专属check.sh或callback lower wrapper，构造lower模型并比较片段/拒绝; tests/gate.sh:417 */
    "lower", "exec-librarytypedlower-model-4", "lib",
    /* exec-libraryfn-model: 入口exec/lower/libraryfncheck.sh; tests/gate.sh:418 */
    "lower", "exec-libraryfn-model", "lib",
    /* exec-librarydata-model: 入口exec/lower/librarydatacheck.sh; tests/gate.sh:419 */
    "lower", "exec-librarydata-model", "lib",
    /* latebind-p6: 公共API未typed库模式26快照baseline; tests/gate.sh:649 */
    "lib", "latebind-p6", "meta",
    /* resourceflow: 真实产品C→Tape严格资源正负例; tests/gate.sh:650 */
    "chain", "resourceflow", "E3,meta",
    /* include-key: product capture与sim include-key对拍; tests/gate.sh:651 */
    "E2", "include-key", "meta",
    /* reviewed-trees: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:679 */
    "meta", "reviewed-trees", "-",
    /* script-inventory-self: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:689 */
    "meta", "script-inventory-self", "-",
    /* inventory-derived: 作业入口测登记/构造器平价/队列/证据/身份或工具合同，不测单个编译阶段; tests/gate.sh:690 */
    "meta", "inventory-derived", "-",
    /* unisa-peversion: gate注释具名image/PE/APE write/build参考点样本，不计目标执行; tests/gate.sh:724 */
    "E6", "unisa-peversion", "build",
    /* unisa-tape: gate注释具名Tape表示/编码/形状参考点样本，不计parse/type全阶段; tests/gate.sh:732 */
    "E3", "unisa-tape", "meta",
    /* unisa-tapebin: gate注释具名Tape表示/编码/形状参考点样本，不计parse/type全阶段; tests/gate.sh:733 */
    "E3", "unisa-tapebin", "meta",
    /* unisa-tapebin-shape: gate注释具名Tape表示/编码/形状参考点样本，不计parse/type全阶段; tests/gate.sh:734 */
    "E3", "unisa-tapebin-shape", "meta",
    /* unisa-image-fat: gate注释具名image/PE/APE write/build参考点样本，不计目标执行; tests/gate.sh:735 */
    "E6", "unisa-image-fat", "build",
    /* unisa-image-elf: gate注释具名image/PE/APE write/build参考点样本，不计目标执行; tests/gate.sh:736 */
    "E6", "unisa-image-elf", "build",
    /* unisa-image-macho: gate注释具名image/PE/APE write/build参考点样本，不计目标执行; tests/gate.sh:737 */
    "E6", "unisa-image-macho", "build",
    /* unisa-image-pe: gate注释具名image/PE/APE write/build参考点样本，不计目标执行; tests/gate.sh:738 */
    "E6", "unisa-image-pe", "build",
    /* unisa-ape: gate注释具名image/PE/APE write/build参考点样本，不计目标执行; tests/gate.sh:739 */
    "E6", "unisa-ape", "build",
    /* unisa-vm: gate注释vm run点程序执行参考，不计宿主文件/syscall闭包; tests/gate.sh:740 */
    "E6", "unisa-vm", "meta",
    /* unisa-gold-prec: gate注释prec/type/parse/scope label有限域源码提取，不计front全流水线; tests/gate.sh:741 */
    "E3", "unisa-gold-prec", "build",
    /* unisa-emit-arm-nwmem: gate注释emit_arm指令字段/N/movimm/mem有限域，不计完整encode/reloc; tests/gate.sh:742 */
    "E5", "unisa-emit-arm-nwmem", "lower",
    /* unisa-gold-lex-label: gate注释lex_label有限域源码提取，不计全lex; tests/gate.sh:743 */
    "E1", "unisa-gold-lex-label", "build",
    /* unisa-gold-type-label: gate注释prec/type/parse/scope label有限域源码提取，不计front全流水线; tests/gate.sh:744 */
    "E3", "unisa-gold-type-label", "build",
    /* unisa-gold-peep-label: gate注释peep_label有限域源码提取; tests/gate.sh:747 */
    "E4", "unisa-gold-peep-label", "build",
    /* unisa-gold-scope-label: gate注释prec/type/parse/scope label有限域源码提取，不计front全流水线; tests/gate.sh:748 */
    "E3", "unisa-gold-scope-label", "build",
    /* unisa-gold-pp-label: gate注释pp_label有限域源码提取，不计全pp; tests/gate.sh:749 */
    "E2", "unisa-gold-pp-label", "build",
    /* unisa-gold-parse-label: gate注释prec/type/parse/scope label有限域源码提取，不计front全流水线; tests/gate.sh:752 */
    "E3", "unisa-gold-parse-label", "build",
    /* com-tapedomain: raw Tape域前置拒绝，--S/-c不得写成功输出; tests/gate.sh:831 */
    "E3", "com-tapedomain", "lower",
    /* com-hostaddr-neg: .hostaddr索引0..3拒绝域（control与参考逐字节同，-1/4双方具名拒绝）; tests/gate.sh */
    "lower", "com-hostaddr-neg", "meta",
    /* com-extent-neg: tape数据extent拒绝域（稀疏control逐字节同；2^31单项/累计产品具名拒绝，参考接受已登记）; tests/gate.sh */
    "lower", "com-extent-neg", "meta",
    /* com-bindings-neg: .librarycall拒绝归属（缺bindings归绑定层、坏寄存器/操作数归tape域；plain与lnx .hostcall控制件逐字节同）; tests/gate.sh */
    "lower", "com-bindings-neg", "meta",
    /* com-hostenv: 宿主环境扰动不改产品输出，无法归单阶段; tests/gate.sh:832 */
    "chain", "com-hostenv", "meta",
    /* com-latebind: hostsymbol合同/绑定拒绝/普通-run产品路径; tests/gate.sh:833 */
    "chain", "com-latebind", "lib,lower",
    /* com-forceinclude: forced include/__FILE__/诊断产品路径; tests/gate.sh:835 */
    "E2", "com-forceinclude", "E3",
    /* com-iquote: quoted include搜索顺序产品路径; tests/gate.sh:836 */
    "E2", "com-iquote", "meta",
    /* unisa-bits: 共享MASK64/round_up叶子C参考对拍，未测单个流阶段; tests/unisabitscheck.sh:2–5; research/c41-k6-inventory-cut.md:146–152 */
    "build", "unisa-bits", "-",
    /* unisa-hostabi: 宿主桥指令常量TSV点采样的C参考迁移；未运行ABI桥; tests/unisahostabicheck.sh:2–5; unisa/emit_arm.py:130; unisa/emit_x86.py:472 */
    "build", "unisa-hostabi", "-",
    /* unisa-padded: Padded前缀/逻辑长度可观察值参考点采样; tests/unisapaddedcheck.sh:2–5; unisa/lower.py:156; unisa/image/__init__.py:55 */
    "build", "unisa-padded", "-",
    /* unisa-zdata: ZData段脚本操作/读写/copy_into参考迁移; tests/unisazdatacheck.sh:2–5; unisa/tape.py:13; unisa/lower.py:156 */
    "build", "unisa-zdata", "-",
    /* unisa-linalg: gemv/relu/argmax/margin有限样本C参考迁移; tests/unisalinalgcheck.sh:2–6; unisa/construct.py:26; unisa/oracle.py:16; unisa/control/net.py:14 */
    "build", "unisa-linalg", "-",
    /* unisa-libneed: scratch微incdir中guard/bodies/table/roots参考迁移; tests/unisalibneedcheck.sh:2–6; unisa/ckernel.py:172–189 */
    "build", "unisa-libneed", "-",
    /* unisa-uns1: 模型张量序列化dumps/loads/pad4参考迁移; tests/unisauns1check.sh:2–6; unisa/__main__.py:527–544; unisa/control/uns1.py:89,117 */
    "build", "unisa-uns1", "-",
    /* unisa-catalog: OS/ARCH/sysno/form/reg等目录事实C参考点采样; tests/unisacatalogcheck.sh:2–6; unisa/lower.py:11; unisa/emit_arm.py:40; unisa/gold.py:8 */
    "build", "unisa-catalog", "-",
    /* unisa-fp: op2/op3浮点运算位模式C参考迁移; tests/unisafpcheck.sh:2–6; unisa/front/lex.py:46; unisa/front/parse.py:1707; unisa/vm.py:8 */
    "build", "unisa-fp", "-",
    /* unisa-gold-binsel-label: C运算符/符号性→Tape ALU flavor；front parse调用Emitter; unisa/ir.py:206–214; unisa/front/parse.py:8; unisa/gold.py:440–454; tests/unisagoldbinsellabelcheck.sh:2–10 */
    "E3", "unisa-gold-binsel-label", "build",
    /* unisa-gold-opinfo-label: 优化器消费Tape op类别，opinfo oracle; unisa/opt.py:39–44; unisa/gold.py:391–408; tests/unisagoldopinfolabelcheck.sh:2–10 */
    "E4", "unisa-gold-opinfo-label", "build",
    /* unisa-gold-reloc-label: lower针对jump/jumpz/call查询reloc字段并交encoder; unisa/lower.py:437–443; unisa/emit_arm.py:328–335; tests/unisagoldreloclabelcheck.sh:2–8 */
    "lower", "unisa-gold-reloc-label", "build",
    /* unisa-gold-irsel-label: front Tape emitter查询family/flavor recipe; unisa/ir.py:1–12,78–87; unisa/front/parse.py:8; tests/unisagoldirsellabelcheck.sh:2–8 */
    "E3", "unisa-gold-irsel-label", "build",
    /* gate-select: selector mechanism invariant checker, next registration step; not product execution; cc step2 dispatch; tests/gateselectcheck.py warehouse draft */
    "meta", "gate-select", "-",
    0
};

static char *also[] = {
    "build", "rowcov-*-build rowcov-*-union strconvert-seed seedparse2* seed-construct-parse2 closure-*"
             " bigclosure exec-e3self exec-e4self",
    "E6", "exec-bind* combo asmtext",
    "lib", "forward* ccinterop ffi-*",
    "meta", "tapebin*",
    "chain", "exec-memory-* exec-memx86-* exec-memwin*",
    "E3", "exec-multi-* multi staticunits linkunits",
    0
};

static int glob(char *p, char *s) {
    if (*p == 0) return *s == 0;
    if (*p == '*') { for (;; s++) { if (glob(p + 1, s)) return 1; if (*s == 0) return 0; } }
    return *p == *s && glob(p + 1, s + 1);
}

/* does any space-separated pattern in list match word? */
static int inlist(char *list, char *word) {
    char pat[128]; int i;
    while (*list) {
        while (*list == ' ') list++;
        i = 0; while (*list && *list != ' ' && i < 127) pat[i++] = *list++;
        pat[i] = 0;
        if (i && glob(pat, word)) return 1;
    }
    return 0;
}

static void forms(char *name, char f[3][256]) {
    char *b = strncmp(name, "com-", 4) == 0 ? name + 4 : name; int n, k;
    strncpy(f[0], b, 255); f[0][255] = 0;
    strcpy(f[1], f[0]); n = strlen(f[1]); k = n;
    while (k > 0 && f[1][k - 1] >= '0' && f[1][k - 1] <= '9') k--;
    if (k < n && k > 0 && f[1][k - 1] == '-') f[1][k - 1] = 0;
    strcpy(f[2], f[0]);
    { char *t = strstr(f[2], "-lnx-"); if (!t) t = strstr(f[2], "-osx-"); if (!t) t = strstr(f[2], "-win-"); if (t) *t = 0; }
}

static char *table(char **tab, char *name, char *skip) {
    char f[3][256]; int r, j;
    forms(name, f);
    for (r = 0; tab[r]; r += 2)
        for (j = 0; j < 3; j++)
            if ((!skip || strcmp(skip, tab[r])) && inlist(tab[r + 1], f[j])) return tab[r];
    return 0;
}

static char **extra_row(char *name) {
    int i; for (i = 0; extra[i]; i += 3) if (!strcmp(extra[i + 1], name)) return extra + i;
    return 0;
}
static char *primary_stage(char *name) {
    char **row = extra_row(name);
    return row ? row[0] : table(rules, name, 0);
}
static int known_stage(char *name) {
    int i; for (i = 0; rules[i]; i += 2) if (!strcmp(rules[i], name)) return 1;
    return 0;
}
static int csv_has(char *csv, char *name) {
    int n = strlen(name); char *p = csv;
    while (*p) {
        char *q = strchr(p, ','); int len = q ? (int)(q - p) : (int)strlen(p);
        if (len == n && !strncmp(p, name, n)) return 1;
        if (!q) break;
        p = q + 1;
    }
    return 0;
}
static int schema_ok(void) {
    int i, j;
    for (i = 0; extra[i]; i += 3) {
        char current[64], previous[64], *p = extra[i + 2]; int have = 0;
        if (!known_stage(extra[i])) { fprintf(stderr, "stagemap: invalid primary %s\n", extra[i + 1]); return 0; }
        for (j = 0; j < i; j += 3) if (!strcmp(extra[j + 1], extra[i + 1])) {
            fprintf(stderr, "stagemap: duplicate exact suite %s\n", extra[i + 1]); return 0;
        }
        if (!strcmp(p, "-")) continue;
        if (!*p) { fprintf(stderr, "stagemap: empty also %s\n", extra[i + 1]); return 0; }
        while (*p) {
            char *q = strchr(p, ','); int len = q ? (int)(q - p) : (int)strlen(p);
            if (len <= 0 || len >= 64) { fprintf(stderr, "stagemap: empty also %s\n", extra[i + 1]); return 0; }
            memcpy(current, p, len); current[len] = 0;
            if (!known_stage(current) || !strcmp(current, extra[i]) || (have && strcmp(previous, current) >= 0)) {
                fprintf(stderr, "stagemap: invalid/duplicate/unsorted also %s\n", extra[i + 1]); return 0;
            }
            strcpy(previous, current); have = 1;
            if (!q) break;
            p = q + 1;
            if (!*p) { fprintf(stderr, "stagemap: empty also %s\n", extra[i + 1]); return 0; }
        }
    }
    return 1;
}
static int also_stages(char *name, char *primary, char *out) {
    char *values[16], forms_[3][256], **row = extra_row(name), *swap;
    int i, j, k, count = 0, found;
    forms(name, forms_);
    for (i = 0; rules[i]; i += 2) {
        char *tag = rules[i];
        if (!strcmp(tag, primary)) continue;
        found = row && csv_has(row[2], tag);
        for (j = 0; also[j]; j += 2) if (!strcmp(tag, also[j]))
            for (k = 0; k < 3; k++) if (inlist(also[j + 1], forms_[k])) found = 1;
        if (found) {
            if (count == 16) return -1;
            values[count++] = tag;
        }
    }
    for (i = 0; i < count; i++) for (j = i + 1; j < count; j++) if (strcmp(values[i], values[j]) > 0) {
        swap = values[i]; values[i] = values[j]; values[j] = swap;
    }
    *out = 0;
    for (i = 0; i < count; i++) { if (i) strcat(out, ","); strcat(out, values[i]); }
    if (!count) strcpy(out, "-");
    return count;
}
int main(int argc, char **argv) {
    static char names[2048][128]; int n = 0, i, j, bad = 0, nover = 0;
    char line[256], tags[256], *mode = argc > 1 ? argv[1] : "--check";
    if (!schema_ok()) return 1;
    if (!strcmp(mode, "--stages")) { for (i = 0; rules[i]; i += 2) printf("%s\n", rules[i]); return 0; }
    /* No unresolved names remain after the 13 owner decisions. Unknown additions
       still fail closed rather than inheriting an implicit meta/UNMAPPED stage. */
    if (!strcmp(mode, "--unmapped")) return 0;
    if (strcmp(mode, "--check") && strcmp(mode, "--list") && strcmp(mode, "--overlaps") && strcmp(mode, "--stage")) {
        fprintf(stderr, "stagemap: unknown mode %s\n", mode); return 1;
    }
    if (!strcmp(mode, "--stage")) {
        if (argc < 3) { fprintf(stderr, "stagemap: missing stage filter\n"); return 1; }
        for (i = 2; i < argc; i++) if (!known_stage(argv[i])) { fprintf(stderr, "stagemap: unknown stage %s\n", argv[i]); return 1; }
    }
    while (fgets(line, sizeof line, stdin)) {
        if (!strchr(line, '\n') && !feof(stdin)) { fprintf(stderr, "stagemap: input name too long\n"); return 1; }
        line[strcspn(line, "\r\n")] = 0;
        if (*line) {
            if (n == 2048) { fprintf(stderr, "stagemap: input list overflow\n"); return 1; }
            if (strlen(line) >= 128) { fprintf(stderr, "stagemap: input name too long\n"); return 1; }
            strcpy(names[n++], line);
        }
    }
    if (!n) { fprintf(stderr, "stagemap: no suite names on stdin\n"); return 1; }
    if (primary_stage("new-suite-without-a-stage")) { fprintf(stderr, "stagemap: unknown names must fail\n"); return 1; }
    for (i = 0; i < n; i++) {
        char *st = primary_stage(names[i]);
        if (!st) { fprintf(stderr, "stagemap: suite without a stage (add it to tests/stagemap.c): %s\n", names[i]); bad = 1; }
    }
    if (bad) return 1;  /* No partially classified list is ever submitted. */
    for (i = 0; i < n; i++) {
        char *st = primary_stage(names[i]); int count = also_stages(names[i], st, tags);
        if (count < 0) { fprintf(stderr, "stagemap: also capacity\n"); return 1; }
        if (count) nover++;
        if (!strcmp(mode, "--list")) printf("%s\t%s\t%s\n", st, names[i], tags);
        else if (!strcmp(mode, "--overlaps")) { if (count) printf("%s\t%s\talso %s\n", st, names[i], tags); }
        else if (!strcmp(mode, "--stage")) { for (j = 2; j < argc; j++) if (!strcmp(argv[j], st)) printf("%s\n", names[i]); }
    }
    if (!strcmp(mode, "--check")) {
        int r, c;
        printf("gate stages: %d suites;", n);
        for (r = 0; rules[r]; r += 2) {
            c = 0; for (i = 0; i < n; i++) if (!strcmp(primary_stage(names[i]), rules[r])) c++;
            printf(" %s %d", rules[r], c);
        }
        printf("; overlaps %d\n", nover);
    }
    return 0;
}
