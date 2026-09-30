# The commands this project is actually driven by.
#
# Every one of these was a line someone had to remember, and a few of them
# were remembered wrong: the suites take a probe list that is easy to omit
# (a suite with no probes checked nothing and still exited 0), and the
# release sequence lived in a person's head.  They are written down here.
#
# There is nothing to "build" in the usual sense -- `unisacc.c` is generated
# by concatenation, and the compiler builds ITSELF -- so this is a task
# runner, not a build system, and it deliberately has no dependency graph
# to go stale.
#
#   make            the fast checks, about a minute
#   make test       every suite (~10 min)
#   make com        model unisacc.com, bounded two-slot build
#   make model-com  one explicit bounded model construction step
#   make release    the pre-release gate
#   make linux      the whole suite inside the Linux VM
#   make bench      compile speed
#
PROBES = examples/*.c tests/c/*.c
UA     ?= /tmp/ua_ref
# Suites run concurrently; the default leaves cores free because this
# machine has overheated under full load.
JOBS   ?= 4
# The shipped compiler is built at -O2 [H1]; OPT=0 gives the walker's code.
OPT    ?= 2
# Explicit model build stages never overwrite the default shipped artifact.
MODEL_DIR  ?=
MODEL_STEP ?=
# N22: the seed and the bootstrap fixed point.  SEED_DIR is private, like
# MODEL_DIR -- a seed is a build input, not an artifact we ship.  COMB=1 makes
# `com` use unisacc-seed.com where it would have used $(UA), which is the
# whole of what bootstrapping means on this line: buildcompiler.sh's last step
# is `python3 -m unisa ape ... --via "$UA" ...`, and nothing else changes.
SEED_DIR  ?= /tmp/unisacc-seed-com
SEED_STEP ?=
COMB      ?= 0
COM_OUT   ?= unisacc.com

.PHONY: help quick test com release linux bench acc weights clean ref \
        c99 closure corpus classic-com model-com seed-com com3 comboot \
        stage2 stage3 comboot-seed-step comboot-stage2-step comboot-stage3-step

help:
	@sed -n '13,19p' $(MAKEFILE_LIST) | sed 's/^# \{0,1\}//'
	@echo "targets: $$(grep -E '^[a-z][a-z0-9-]*:' $(MAKEFILE_LIST) | cut -d: -f1 | sort -u | tr '\n' ' ')"

.DEFAULT_GOAL := quick

# The reference build every suite uses.  -O2: it is run thousands of times.
ref:
	@./tests/build_ref.sh "$(UA).c" "$(UA)"

# What to run before saying "it works" -- the checks that are fast enough
# that there is no excuse not to.
quick: ref
	@./tests/c99.sh
	@./tests/diag.sh
	@./tests/cli.sh
	@./tests/layout.sh
	@python3 tests/consts_check.py
	@python3 -m unisa acc

test: ref
	@JOBS=$(JOBS) ./tests/all.sh

c99: ref
	@./tests/c99.sh

closure: ref
	@./tests/closure.sh $(PROBES)

corpus: ref
	@for k in 1 2 3 4; do FETCH=0 SHARD=$$k/4 ./tests/corpus.sh || exit 1; done

bench: ref
	@./tests/bench.sh

acc:
	@python3 -m unisa acc

weights:
	@python3 -m unisa build-weights

# One file, every target, built in ONE environment -- that is the whole
# point of a compiler that writes all six itself.  CI tests; it does not
# build.
com:
	@UA="$(if $(filter 1,$(COMB)),$(SEED_DIR)/unisacc-seed.com,$(UA))" \
	 python3 tests/bound.py 60 sh -ec '\
	    out="$(if $(MODEL_DIR),$(MODEL_DIR),out/model-com)"; \
	    dst="$(COM_OUT)"; \
	    ./exec/c/buildcompiler.sh "$$out"; \
	    tmp=$$(mktemp ./"$$dst".XXXXXX); \
	    trap "rm -f \"$$tmp\"" EXIT HUP INT TERM; \
	    cp "$$out/unisacc-next.com" "$$tmp"; chmod +x "$$tmp"; \
	    mv -f "$$tmp" "$$dst"; \
	    cp "$$out/unisacc-next.com.build.json" "$$dst.build.json"; \
	    python3 exec/c/provenance.py check "$$dst"'

# N22 stage 1: ONE bounded build step of the seed, built by $(UA) exactly as
# `com` builds today.  The seed is never shipped; its only job is to be the
# compiler that builds stage 2.  Every step is its own invocation because the
# whole build does not fit the 55-second rule -- run
#   SEED_STEP=shared, then each of the six targets, then SEED_STEP=pack,
# and the last one seals the seed and records it.  `all` still works for a
# shell with no watchdog.
seed-com:
	@UA="$(UA)" python3 tests/bound.py 55 ./exec/c/buildcompiler.sh "$(SEED_DIR)" $(SEED_STEP)
	@case "$(SEED_STEP)" in pack|all) \
	    python3 tests/bound.py 55 sh -ec '\
	    set -e; \
	    [ -s "$(SEED_DIR)/unisacc-next.com" ] || { echo "seed-com: no artifact" >&2; exit 1; }; \
	    dst="$(SEED_DIR)/unisacc-seed.com"; \
	    tmp=$$(mktemp "$$dst".XXXXXX); \
	    trap "rm -f \"$$tmp\"" EXIT HUP INT TERM; \
	    cp "$(SEED_DIR)/unisacc-next.com" "$$tmp"; chmod +x "$$tmp"; \
	    mv -f "$$tmp" "$$dst"; \
	    cp "$(SEED_DIR)/unisacc-next.com.build.json" "$$dst.build.json"; \
	    python3 exec/c/provenance.py check "$$dst"; \
	    python3 exec/c/comboot.py stage 1 "$$dst"'; \
	    ;; *) echo "seed-com: $(SEED_STEP) done (not the last step; nothing sealed yet)";; esac

# N22 one bounded build step of stage 2, then of stage 3.  COMB=1 is the whole
# difference: it puts the previous stage where $(UA) was.
stage2:
	@$(MAKE) --no-print-directory com COMB=1 COM_OUT=unisacc.com
stage3:
	@$(MAKE) --no-print-directory com COMB=1 COM_OUT="$(SEED_DIR)/stage3/unisacc.com"'

# N22 stage 3: built BY stage 2, into SEED_DIR/stage3/.  Only its bytes matter;
# it is never shipped.  `make comboot` runs all three stages and the cmp.
com3:
	@$(MAKE) --no-print-directory com COMB=1 COM_OUT="$(SEED_DIR)/stage3/unisacc.com"
	@python3 tests/bound.py 55 exec/c/comboot.py cmp "$(COM_OUT)" "$(SEED_DIR)/stage3/unisacc.com"

# The gate: four shards, each bounded, each doing ONE stage's build and then
# its comparison.  The build steps inside are sharded by SEED_STEP, so the
# per-shard cost is one buildcompiler step -- see exec/c/comboot.py.
comboot:
	@python3 tests/bound.py 55 exec/c/comboot.py shard seed
	@python3 tests/bound.py 55 exec/c/comboot.py shard stage2
	@python3 tests/bound.py 55 exec/c/comboot.py shard stage3
	@python3 tests/bound.py 55 exec/c/comboot.py shard fixedpoint

# The build steps the shards call, as separate targets so `make -n` shows them.
COMBSTEP_DIR ?= /tmp/unisacc-comb-build
comboot-seed-step:
	@UA="$(UA)" python3 tests/bound.py 55 ./exec/c/buildcompiler.sh "$(SEED_DIR)" $(SEED_STEP)
comboot-stage2-step:
	@UA="$(if $(filter 1,$(COMB)),$(SEED_DIR)/unisacc-seed.com,$(UA))" \
	 python3 tests/bound.py 55 ./exec/c/buildcompiler.sh "$(COMBSTEP_DIR)" $(COMB_STEP)
comboot-stage3-step:
	@UA="$(if $(filter 1,$(COMB)),unisacc.com,$(UA))" \
	 python3 tests/bound.py 55 ./exec/c/buildcompiler.sh "$(COMBSTEP_DIR)" $(COMB_STEP)

classic-com: ref
	@mkdir -p out
	@python3 -m unisa ape unisacc.c --via "$(UA)" -O $(OPT) -o out/unisacc-classic.com
	@chmod +x out/unisacc-classic.com
	@ls -l out/unisacc-classic.com | awk '{printf "  out/unisacc-classic.com  %s B\n", $$5}'

# One explicit stage per invocation; completion/input checks belong to the
# existing builder. Run pack separately, after shared and all six targets.
model-com:
	@test -n "$(MODEL_DIR)" || { echo 'model-com: set MODEL_DIR to a private output directory' >&2; exit 2; }
	@case "$(MODEL_STEP)" in shared|lnx/arm64|lnx/x86_64|osx/arm64|osx/x86_64|win/arm64|win/x86_64|pack) ;; *) echo 'model-com: set MODEL_STEP=shared|OS/ARCH|pack (see exec/c/BUILDING.md)' >&2; exit 2;; esac
	@UA="$(UA)" python3 tests/bound.py 55 ./exec/c/buildcompiler.sh "$(MODEL_DIR)" "$(MODEL_STEP)"

release:
	@UA="$(UA)" ./tests/release.sh --com

# The guest is emulated when its architecture differs from this host's;
# linux.sh notices and scales the watchdogs.
linux:
	@./tests/linux.sh

clean:
	@rm -f unisacc.com unisacc.com.build.json .release.log
	@rm -rf /tmp/ua_ref /tmp/ua_ref.c
	@echo "  removed the built artifacts (unisacc.c is generated and tracked)"

# Independent export for VM transfer and single-file self-hosting.
.PHONY: export-ref
export-ref:
	@mkdir -p out
	@./tests/export_ref.sh out/unisacc-flat.c
