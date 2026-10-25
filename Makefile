# Native CX iteration. The historical gate and its Makefile are archived.
.DEFAULT_GOAL := smoke
UNISACC ?= ./unisacc.com
UA ?= $(UNISACC)
MODEL_DIR ?=
MODEL_STEP ?=
SEED_DIR ?= out/bootstrap/stage1
SEED_STEP ?=
COMBSTEP_DIR ?= out/bootstrap/next
COMB_STEP ?=
COMB ?= 0
OUT ?= out/unisacc-flat.c
STREAM_DIR ?=
STREAM_OUT ?=
IDENT_BINARY ?=
IDENT_OUT ?=

.PHONY: identity streams owned help smoke quick test list export-ref ref com model-com release comboot-seed-step comboot-stage2-step comboot-stage3-step
help:
	@echo 'smoke / quick: native product batch, partial coverage only'
	@echo 'test: complete contract gate; NOT_READY is a failure'
	@echo 'list: implemented cases and explicit gaps'
	@echo 'export-ref OUT=...: native complete source export'
	@echo 'streams STREAM_DIR=... STREAM_OUT=...: partial seven-stream contracts, fresh output required'
	@echo 'model-com MODEL_DIR=... MODEL_STEP=...: one bounded construction step'
identity:
	@test -n "$(IDENT_BINARY)" -a -n "$(IDENT_OUT)" || { echo 'set IDENT_BINARY and a new IDENT_OUT' >&2; exit 2; }
	@tools/bound 40 sh "$(UNISACC)" tests/identity.cx -- "$(CURDIR)" "$(abspath $(IDENT_BINARY))" "$(abspath $(IDENT_OUT))"
streams:
	@test -n "$(STREAM_DIR)" -a -n "$(STREAM_OUT)" || { echo 'set STREAM_DIR and a new STREAM_OUT' >&2; exit 2; }
	@tools/bound 55 sh "$(UNISACC)" tests/streams.cx -- "$(CURDIR)" "$(abspath $(STREAM_DIR))" "$(abspath $(STREAM_OUT))"
owned:
	@sh "$(UNISACC)" cx/check_owned.cx
smoke quick:
	@sh "$(UNISACC)" tests/run.cx -- --smoke
test:
	@sh "$(UNISACC)" tests/run.cx -- --all
list:
	@sh "$(UNISACC)" tests/run.cx -- --list
export-ref:
	@mkdir -p out
	@sh "$(UNISACC)" tools/sourceflat.cx -- "$(OUT)"
ref:
	@tools/build_ref.sh "$(UA).c" "$(UA)"
com: model-com
model-com:
	@test -n "$(MODEL_DIR)" -a -n "$(MODEL_STEP)" || { echo 'set MODEL_DIR and MODEL_STEP; one step per invocation' >&2; exit 2; }
	@UA="$(UA)" tools/bound 55 ./exec/c/buildcompiler.sh "$(MODEL_DIR)" "$(MODEL_STEP)"
comboot-seed-step:
	@UA="$(UA)" tools/bound 55 ./exec/c/buildcompiler.sh "$(SEED_DIR)" "$(SEED_STEP)"
comboot-stage2-step:
	@UA="$(if $(filter 1,$(COMB)),$(SEED_DIR)/unisacc-seed.com,$(UA))" tools/bound 55 ./exec/c/buildcompiler.sh "$(COMBSTEP_DIR)" "$(COMB_STEP)"
comboot-stage3-step:
	@UA="$(if $(filter 1,$(COMB)),unisacc.com,$(UA))" tools/bound 55 ./exec/c/buildcompiler.sh "$(COMBSTEP_DIR)" "$(COMB_STEP)"
release: test
	@echo 'Release collection and matching native-platform evidence remain NOT_READY.' >&2
	@exit 3
