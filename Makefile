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
#   make com        unisacc.com, all six targets, built here
#   make release    the pre-release gate
#   make linux      the whole suite inside the Linux VM
#   make bench      compile speed
#
PROBES = examples/*.c tests/c/*.c
UA     ?= /tmp/ua_ref
# Suites run concurrently; the default leaves cores free because this
# machine has overheated under full load.
JOBS   ?= 4

.PHONY: help quick test com release linux bench acc weights clean ref \
        c99 closure corpus

help:
	@sed -n '13,19p' $(MAKEFILE_LIST) | sed 's/^# \{0,1\}//'
	@echo "targets: $$(grep -E '^[a-z][a-z0-9]*:' $(MAKEFILE_LIST) | cut -d: -f1 | sort -u | tr '\n' ' ')"

.DEFAULT_GOAL := quick

# The reference build every suite uses.  -O2: it is run thousands of times.
ref:
	@./tests/build_ref.sh

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
com: ref
	@python3 -m unisa ape unisacc.c --via $(UA) -o unisacc.com
	@chmod +x unisacc.com
	@ls -l unisacc.com | awk '{printf "  unisacc.com  %s B\n", $$5}'

release: ref
	@./tests/release.sh --com

# The guest is emulated when its architecture differs from this host's;
# linux.sh notices and scales the watchdogs.
linux:
	@./tests/linux.sh

clean:
	@rm -f unisacc.com .release.log
	@rm -rf /tmp/ua_ref /tmp/ua_ref.c
	@echo "  removed the built artifacts (unisacc.c is generated and tracked)"
