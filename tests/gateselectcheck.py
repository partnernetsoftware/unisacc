#!/usr/bin/env python3
"""Synthetic small-graph tests, no real gatedeps or gatequeue execution."""
import copy
import importlib.util
import itertools
import json
from pathlib import Path
import unittest
import sys
sys.dont_write_bytecode = True

B = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("selector", B / "tests/gateselect.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
# Synthetic replacement is confined to fixtures; production table keeps its unresolved role.
POLICY_TEXT = (B / "tests/gateselect.tsv").read_text().replace("stage-contract-e1e3", "sc1-control").replace("com-iquote", "product-control").replace("staticimage", "abi").replace("com-cli", "image")
POLICY_TEXT = POLICY_TEXT.replace("suite.unisa-gold-opinfo-label", "suite.lex").replace("suite.unisa-gold-irsel-label", "suite.lower")
P = m.parse_policy(POLICY_TEXT)


def fixture():
    def job(name, layer="contract", primary="meta", also=(), after=()):
        return dict(name=name, layer=layer, primary=primary, also=list(also), after=list(after),
                    declaration="known", command=["fixture-only", name])
    return dict(mode="FIXTURE_ONLY", catalog={"layers": ["contract", "stage", "pipeline", "platform"],
                "stages": ["E2", "E1", "E3", "E4", "lower", "E5", "E6", "chain", "lib", "build", "meta"]}, jobs=[
        job("gate-layers"), job("script-inventory"), job("checkrun"),
        job("sc1-control", "stage", "E3", ["lower"]),
        job("product-control", "pipeline", "chain"),
        job("prep", "stage", "build"),
        job("lex", "stage", "E1", after=["prep"]),
        job("parse", "stage", "E3", ["E2"], ["lex"]),
        job("lower", "stage", "lower", ["E5"], ["parse"]),
        job("abi", "contract", "meta"),
        job("image", "pipeline", "E6"),
        job("platform", "platform", "lib")],
        changes=[dict(status="M", paths=["exec/parse2/rule.tsv"])],
        affected=["parse"], unclaimed=[])


class SelectorTests(unittest.TestCase):
    def plan(self, f=None, **kw):
        return m.select(f or fixture(), P, **kw)

    def rejects(self, thunk, word):
        with self.assertRaisesRegex(m.SelectionError, word):
            thunk()

    def test_single_file(self):
        f = fixture(); p = self.plan(f)
        self.assertEqual(set(p["suites"]), set(P["smokes"]) | {"prep", "lex", "parse"})
        self.assertEqual(m.verify(p, f, P)["status"], "VALID_PLAN_ONLY")

    def test_family_identity_replaced_normalized_fixture(self):
        # Simulate effective_entry identity output only, not selector's conservative union.
        f = fixture(); f["affected"] = ["lex"]
        f["adapter_evidence"] = {"kind": "synthetic", "family_owner": "family-header.h", "ignored_suite_file": "suite-only.h"}
        p = self.plan(f)
        self.assertIn("lex", p["suites"]); self.assertNotIn("parse", p["suites"])

    def test_family_selection_conservative_union_from_adapter(self):
        # declared_reasons on the RAW suite supplies both suite and family hits.
        # This test does not implement or certify the actual common declaration parser.
        f = fixture(); f["affected"] = ["lex", "parse"]
        f["adapter_evidence"] = {"kind": "synthetic", "suite_hit": "files", "family_hit": "guard"}
        p = self.plan(f); self.assertTrue({"lex", "parse"} <= set(p["suites"]))

    def test_shared_header_breadth_not_truncated(self):
        f = fixture(); f["affected"] = [j["name"] for j in f["jobs"]]
        p = self.plan(f); self.assertEqual(len(p["suites"]), len(f["jobs"]))
        self.assertEqual(p["breadth"], "ESCALATE_BREADTH")

    def test_deleted_path_no_stat(self):
        f = fixture(); f["changes"] = [{"status": "D", "paths": ["no-longer-existing.h"]}]
        self.assertIn("parse", self.plan(f)["suites"])

    def test_rename_preserves_old_and_new_identity(self):
        f = fixture(); f["changes"] = [{"status": "R", "paths": ["old name.h", "new\nname.h"]}]
        p = self.plan(f); m.verify(p, f, P)
        g = copy.deepcopy(f); g["changes"][0]["paths"].reverse()
        self.rejects(lambda: m.verify(p, g, P), "input-changed")

    def test_untracked(self):
        f = fixture(); f["changes"] = [{"status": "?", "paths": ["new.h"]}]
        self.assertIn("parse", self.plan(f)["suites"])

    def test_predecessor_chain(self):
        f = fixture(); f["affected"] = ["lower"]
        self.assertTrue({"prep", "lex", "parse", "lower"} <= set(self.plan(f)["suites"]))

    def test_cycle(self):
        f = fixture(); f["jobs"][5]["after"] = ["lower"]
        self.rejects(lambda: self.plan(f), "predecessor-cycle")

    def test_dangling_predecessor(self):
        f = fixture(); f["jobs"][5]["after"] = ["not-on-host"]
        self.rejects(lambda: self.plan(f), "dangling-predecessor")

    def test_primary_stage(self):
        f = fixture(); f["affected"] = []
        p = self.plan(f, stages=["E6"]); self.assertIn("image", p["suites"])
        self.assertIn("stage-primary", p["reasons"]["image"])

    def test_secondary_stage(self):
        f = fixture(); f["affected"] = []
        p = self.plan(f, stages=["E2"]); self.assertIn("parse", p["suites"])
        self.assertIn("stage-also", p["reasons"]["parse"])

    def test_fake_stage(self):
        self.rejects(lambda: self.plan(stages=["fake"]), "unknown-stage")

    def test_layer_stage_intersection_additive(self):
        p = self.plan(layers=["platform"], stages=["E6"])
        self.assertIn("parse", p["suites"]); self.assertIn("product-control", p["suites"])
        self.assertNotIn("image", p["suites"])

    def test_duplicate_filters_are_union(self):
        a = self.plan(stages=["E1", "E6", "E1"], layers=["stage", "pipeline", "stage"])
        b = self.plan(stages=["E1", "E6"], layers=["stage", "pipeline"])
        self.assertEqual(a, b)

    def test_smoke_deleted(self):
        f = fixture(); f["jobs"] = [j for j in f["jobs"] if j["name"] != "product-control"]
        self.rejects(lambda: self.plan(f), "smoke-not-in-universe")

    def test_empty_diff_still_smoke(self):
        f = fixture(); f.update(changes=[], affected=[])
        self.assertEqual(set(self.plan(f)["suites"]), set(P["smokes"]))

    def test_new_suite_full_tier(self):
        f = fixture(); f["jobs"].append(dict(name="new", layer="stage", primary="E4", command=["fixture"], declaration="known"))
        for tier in ["candidate", "release"]:
            self.assertIn("new", self.plan(f, tier=tier)["suites"])

    def test_metadata_changes_force_full_meta_check(self):
        f = fixture(); f["metadata_changed"] = True
        self.assertEqual(len(self.plan(f)["suites"]), len(f["jobs"]))

    def test_unknown_declaration_global(self):
        f = fixture(); f["jobs"][-1]["declaration"] = "unknown"
        self.assertIn("platform", self.plan(f)["suites"])

    def test_unclaimed_change_force_global(self):
        f = fixture(); f["affected"] = []; f["unclaimed"] = ["exec/parse2/rule.tsv"]
        self.assertEqual(len(self.plan(f)["suites"]), len(f["jobs"]))

    def test_remove_A_detected(self):
        f = fixture(); p = self.plan(f); p["suites"].remove("parse")
        self.rejects(lambda: m.verify(p, f, P), "missing-A-or-B")

    def test_remove_P_detected(self):
        f = fixture(); p = self.plan(f); p["suites"].remove("prep")
        self.rejects(lambda: m.verify(p, f, P), "missing-predecessor")

    def test_remove_B_detected(self):
        f = fixture(); p = self.plan(f); p["suites"].remove("product-control")
        self.rejects(lambda: m.verify(p, f, P), "missing-A-or-B")

    def test_candidate_remove_nonmandatory_detected(self):
        f = fixture(); p = self.plan(f, tier="candidate"); p["suites"].remove("platform")
        self.rejects(lambda: m.verify(p, f, P), "full-universe")

    def test_candidate_release_filter_forbidden(self):
        for t in ["candidate", "release"]:
            self.rejects(lambda: self.plan(tier=t, stages=["E3"]), "filter-forbidden")

    def test_merge_control_is_policy_addition_not_dev_smoke(self):
        f=fixture();f.update(changes=[],affected=[])
        self.assertNotIn("image",self.plan(f,tier="dev")["suites"])
        p=self.plan(f,tier="merge");self.assertIn("image",p["suites"])
        self.assertIn("merge-control",p["reasons"]["image"])

    def test_missing_merge_control_refuses(self):
        f=fixture();f["jobs"]=[j for j in f["jobs"] if j["name"]!="image"]
        self.rejects(lambda:self.plan(f,tier="merge"),"merge-control-not-in-universe")

    def test_merge_all_affected_layers(self):
        f = fixture(); f["affected"] = ["parse", "abi"]
        p = self.plan(f, tier="merge"); selected=set(p["suites"])
        self.assertTrue({j["name"] for j in f["jobs"] if j["layer"] in ["stage", "contract"]} <= selected)
        self.assertNotIn("platform", selected)

    def test_inspect_not_acceptance(self):
        f = fixture(); p = self.plan(f, tier="inspect", layers=["platform"])
        self.assertEqual(p["suites"], ["platform"]); self.assertEqual(p["status"], "INSPECT_ONLY")
        self.assertEqual(m.verify(p, f, P)["coverage_verdict"], "NOT_ASSESSED")

    def test_policy_unknown_record(self):
        self.rejects(lambda: m.parse_policy(POLICY_TEXT + "eval\tx\t__import__('os')\n"), "policy-unknown")

    def test_policy_duplicate(self):
        self.rejects(lambda: m.parse_policy(POLICY_TEXT + "policy\tbudget-mode\treport-only\n"), "policy-duplicate")

    def test_policy_missing_smoke_role(self):
        self.rejects(lambda: m.parse_policy(POLICY_TEXT.replace("smoke\tproduct-control\tproduct-control\n", "")), "missing-smoke")

    def test_policy_illegal_expression(self):
        self.rejects(lambda: m.parse_policy(POLICY_TEXT.replace("A+B+H", "fastest(5)")), "policy-value")

    def test_policy_budget_not_a_cap(self):
        q=m.parse_policy(POLICY_TEXT.replace("\t180\n", "\t1\n"));f=fixture();f["affected"]=[j["name"] for j in f["jobs"]]
        self.assertEqual(len(m.select(f,q)["suites"]),len(f["jobs"]))

    def test_policy_bad_shape(self):
        self.rejects(lambda: m.parse_policy(POLICY_TEXT + "bad\trow\n"), "policy-shape")

    def test_unclassified_suite(self):
        f=fixture();f["jobs"][-1]["layer"]="guess-by-name"
        self.rejects(lambda: self.plan(f), "unclassified-layer")

    def test_duplicate_universe(self):
        f=fixture();f["jobs"].append(copy.deepcopy(f["jobs"][0]))
        self.rejects(lambda: self.plan(f), "universe-duplicate")

    def test_real_mode_forbidden(self):
        f=fixture();f["mode"]="REAL"
        self.rejects(lambda: self.plan(f), "adapter-not-ready")

    def test_input_or_policy_identity_changes(self):
        f=fixture();p=self.plan(f);f["jobs"][-1]["command"]=["changed"]
        self.rejects(lambda: m.verify(p,f,P), "input-changed")
        q=m.parse_policy(POLICY_TEXT.replace("\t180\n", "\t181\n"))
        self.rejects(lambda: m.verify(self.plan(),fixture(),q), "policy-changed")

    def test_tampered_reasons_not_trusted(self):
        f=fixture();p=self.plan(f);p["reasons"]["parse"]=[]
        self.rejects(lambda: m.verify(p,f,P), "metadata-or-reason")

    def test_nul_path_refused(self):
        f=fixture();f["changes"][0]["paths"]=["bad\0path"]
        self.rejects(lambda: self.plan(f), "change-path")

    def test_order_preserved_not_topological_rescheduled(self):
        f=fixture();p=self.plan(f); self.assertEqual(p["suites"], [j["name"] for j in f["jobs"] if j["name"] in p["suites"]])
        p["suites"].reverse(); self.rejects(lambda: m.verify(p,f,P), "order-mismatch")

    def test_also_wire_set_from_single_owner(self):
        self.assertEqual(m.parse_stage_rows(b"E3\tparse\tE1,lower\nmeta\tabi\t-\n",["parse","abi"]),
                         {"parse":("E3",["E1","lower"]),"abi":("meta",[])})

    def test_also_wire_unsorted_duplicate_empty_space(self):
        for value,reason in [("lower,E1","unsorted"),("E1,E1","duplicate"),("E1,","empty"),("E1, lower","space")]:
            self.rejects(lambda:m.parse_stage_rows(("E3\tparse\t"+value+"\n").encode(),["parse"]),reason)

    def test_also_wire_missing_row_refuses(self):
        self.rejects(lambda:m.parse_stage_rows(b"E3\tparse\t-\n",["parse","abi"]),"incomplete")

    def test_nul_diff_real_parser(self):
        raw=b"R100\0old name.h\0new\nname.h\0D\0gone.h\0M\0keep.h\0"
        rows=m.parse_diff(raw)
        self.assertEqual(rows,[{"status":"R","paths":["old name.h","new\nname.h"]},
                               {"status":"D","paths":["gone.h"]},{"status":"M","paths":["keep.h"]}])

    def test_nul_diff_incomplete_rejected(self):
        self.rejects(lambda:m.parse_diff(b"R100\0old\0"),"path-count")

    def test_nul_gate_argv(self):
        rows=m.parse_gate_plan(b"suite\0"+b"3\0python3\0file name.py\0line\narg\0")
        self.assertEqual(rows,[{"name":"suite","command":["python3","file name.py","line\narg"]}])

    def test_nul_gate_truncated(self):
        self.rejects(lambda:m.parse_gate_plan(b"suite\0"+b"2\0only-one\0"),"argv-extent")

    def test_real_common_declaration_api_two_readings(self):
        decl=m.load_module(B/"tests/gatedecl.py","_gateselect_test_decl")
        manifest={"families":{"F":{"files":["family.h"],"guards":{"guard.h":"hash"}}},
                  "suites":{"s":{"files":["own.h"],"command":["check"],"family":"F"}}}
        effective=decl.effective_entry(manifest,"s")
        self.assertEqual(effective["files"],["family.h"])
        self.assertEqual(decl.declared_reasons(manifest["suites"]["s"],manifest["families"],"own.h"),["files"])
        self.assertEqual(decl.declared_reasons(manifest["suites"]["s"],manifest["families"],"guard.h"),["family"])

    def test_production_pending_smoke_refuses(self):
        policy=m.parse_policy((B/"tests/gateselect.tsv").read_text())
        if any(n.startswith("PENDING_") for n in policy["smokes"]):
            self.rejects(lambda:m.select(fixture(),policy),"smoke-role-unresolved")
        else:
            self.assertEqual(set(policy["smokes"]),{"gate-layers","script-inventory","checkrun","staticimage","stage-contract-e1e3","com-iquote"})
            self.assertEqual(policy["merge_suites"],["com-cli"])

    def test_candidate_without_com_rejected_before_tools(self):
        self.rejects(lambda:m.real_plan(B,{"com":False},B/"tests/gateselect.tsv","candidate",{}),"requires-com")

    def test_merge_no_unmeasured_budget_promise(self):
        self.assertIsNone(self.plan(tier="merge")["budget"]["hot_seconds"])

    def test_real_adapter_offline_fixture_repo(self):
        # End-to-end adapter exercise on a wholly synthetic repo; never live gatedeps/state.
        import os, shutil, subprocess, tempfile
        source_root = Path(__file__).resolve().parents[1]
        self.assertTrue((source_root / "tests/bound.py").is_file(), "test tool inputs missing")
        with tempfile.TemporaryDirectory(prefix="gateselect-adapter-test-") as tmp:
            root=Path(tmp);(root/"tests").mkdir();(root/"release/tools").mkdir(parents=True)
            for rel in ["tests/bound.py","tests/bound.c","tests/bound","tests/gatedecl.py","tests/gatelayers.py","release/tools/derivedcheck.py"]:
                shutil.copy2(source_root/rel,root/rel)
            names=["gate-layers","script-inventory","checkrun","stage-contract-e1e3","cli","exec-lower","abi-manifest","exec-elf"]
            raw=b"".join(n.encode()+b"\0"+b"2\0fixture-runner\0"+n.encode()+b"\0" for n in names)
            gate=root/"tests/gate.sh";gate.write_text("#!/bin/sh\nprintf '"+"".join("\\%03o"%v for v in raw)+"'\n")
            mapper = '#include <stdio.h>\n#include <string.h>\nint main(void){char n[256];while(fgets(n,sizeof n,stdin)){n[strcspn(n,"\\n")]=0;printf("meta\\t%s\\t-\\n",n);}return 0;}\n'
            (root/"tests/stagemap.c").write_text(mapper)
            decl={"families":{"F":{"files":["known.h"]}},"suites":{n:{"command":["fixture-runner",n],"files":[]} for n in names}}
            decl["suites"]["exec-lower"].update(family="F",files=["own.h"])
            (root/"tests/gatedeps.json").write_text(json.dumps(decl));(root/"tests/gateorder.json").write_text(json.dumps({"after":{"exec-lower":"checkrun"}}))
            (root/"release/derived-map.tsv").write_text("# fixture only\n")
            policy=root/"tests/gateselect.tsv";policy.write_text("\n".join(line for line in POLICY_TEXT.splitlines() if not line.startswith(("ask\t", "stage.alias\t"))).replace("sc1-control\tsc1-control","sc1-control\tstage-contract-e1e3").replace("product-control\tproduct-control","product-control\tcli").replace("checkrun,abi", "checkrun,abi-manifest").replace("merge.include\timage", "merge.include\texec-elf"))
            (root/"known.h").write_text("old\n");(root/"own.h").write_text("stable\n")
            env=dict(os.environ,GIT_AUTHOR_NAME="fixture",GIT_AUTHOR_EMAIL="f@local",GIT_COMMITTER_NAME="fixture",GIT_COMMITTER_EMAIL="f@local")
            def git(*a):return subprocess.check_output(["git",*a],cwd=root,env=env,stderr=subprocess.DEVNULL).decode().strip()
            git("init","-q");git("symbolic-ref","HEAD","refs/heads/main");git("add",".");git("commit","-qm","fixture base");base=git("rev-parse","HEAD")
            (root/"known.h").unlink();git("add","-u");git("commit","-qm","fixture deletion");head=git("rev-parse","HEAD")
            req={"base":base,"head":head,"worktree":False,"com":True,"paths":[]}
            plan=m.real_plan(root,req,policy,"dev",{})
            self.assertIn("exec-lower",plan["affected"])
            self.assertIn("changed:family:known.h",plan["reasons"]["exec-lower"])
            self.assertEqual(plan["coverage_verdict"],"NOT_ASSESSED")
            self.assertEqual(plan,m.real_plan(root,req,policy,"dev",{}))
            full=m.real_plan(root,req,policy,"candidate",{})
            self.assertEqual(full["suites"],names)

    def test_paper_alias_expansion(self):
        for alias, targets in P["aliases"].items():
            self.assertEqual(self.plan(stages=[alias])["filters"]["expanded_stages"], targets)
        self.assertEqual(self.plan(stages=["E5"])["filters"]["expanded_stages"], ["E5"])

    def test_paper_alias_wire_refusals(self):
        for old, new in [("enc\tE5,E6", "enc\tE6,E5"), ("pp\tE2", "pp\tpp"), ("stage.alias\tpp\tE2\n", "")]:
            self.rejects(lambda:m.parse_policy(POLICY_TEXT.replace(old,new)), "alias")
        self.rejects(lambda:m.parse_policy(POLICY_TEXT+"stage.alias\tpp\tE2\n"), "duplicate")

    def ask_fixture(self):
        f=fixture()
        f["ask_catalog_sha256"]=m.bind_ask_catalog(P,(B/"kernel/unisa_model.inc").read_bytes())
        return f

    def test_ask_additive_union(self):
        f=self.ask_fixture()
        plan=self.plan(f,stages=["E6"],asks=["opinfo","irsel","opinfo"])
        self.assertTrue({"lex","lower","parse","image","prep"}|set(P["smokes"]) <= set(plan["suites"]))
        self.assertIn("ask-label:opinfo",plan["reasons"]["lex"])
        self.assertEqual(m.verify(plan,f,P)["status"],"VALID_PLAN_ONLY")

    def test_ask_fail_closed(self):
        f=self.ask_fixture()
        for tier in ("candidate","release"):
            self.rejects(lambda:self.plan(f,tier=tier,asks=["opinfo"]),"full-tier")
        self.rejects(lambda:self.plan(f,asks=["lexcls"]),"unknown-ask")
        self.rejects(lambda:self.plan(f,asks=["abi"]),"ask-unmapped")
        q=copy.deepcopy(P);q["ask_suites"]["missing"]=["abi"]
        self.rejects(lambda:m.select(f,q),"ask-suite-not-in-universe")
        q=m.parse_policy("\n".join(l for l in POLICY_TEXT.splitlines() if not l.startswith("ask\t")))
        self.assertEqual(m.select(f,q)["suites"],self.plan(f)["suites"])
        self.rejects(lambda:m.select(f,q,asks=["opinfo"]),"ask-axis-disabled")

    def test_ask_catalog_wire(self):
        raw=(B/"kernel/unisa_model.inc").read_bytes()
        for bad in (raw.replace(b"S_PP 0",b"S_PP 1"),raw.replace(b"S_PP 0",b"S_LEXCLS 0"),b""):
            self.rejects(lambda:m.bind_ask_catalog(P,bad),"ask-generated")
        vocab=P["ask_vocabulary"]
        for tags in (vocab[:-1],vocab+["other"],vocab[:-1]+[vocab[0]],vocab[:-1]+[""]):
            self.rejects(lambda:m.parse_policy(POLICY_TEXT.replace(",".join(vocab),",".join(tags))),"ask")
        q=copy.deepcopy(P);q["ask_vocabulary"]=list(reversed(vocab))
        self.rejects(lambda:m.bind_ask_catalog(q,raw),"catalog-mismatch")

    def test_ask_plan_tamper(self):
        f=self.ask_fixture();p=self.plan(f,asks=["opinfo"])
        for field in ("ask_catalog_sha256","filters","reasons"):
            q=copy.deepcopy(p)
            if field=="filters":q[field]["expanded_stages"]=["E5"]
            elif field=="reasons":q[field]["lex"]=[]
            else:q[field]="forged"
            self.rejects(lambda:m.verify(q,f,P),"mismatch")

    def test_exhaustive_five_job_affected_masks(self):
        f=fixture(); candidates=["lex","parse","lower","abi","platform"]
        for bits in itertools.product([False,True],repeat=5):
            f["affected"]=[n for n,bit in zip(candidates,bits) if bit]
            for tier in ["dev","merge","candidate","release"]:
                p=self.plan(f,tier=tier); m.verify(p,f,P)
                self.assertTrue(set(f["affected"])|set(P["smokes"]) <= set(p["suites"]))
                self.assertEqual(p["coverage_verdict"],"NOT_ASSESSED")


if __name__ == "__main__":
    unittest.main(verbosity=2)
