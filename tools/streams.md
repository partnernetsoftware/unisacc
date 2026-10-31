# Single-stream development tools

The current route contains seven named streams plus a separate tapecheck step.
These developer artifacts isolate the seven streams; they do not replace the
complete route, its RI checks, target contracts or publication qualification.
The material packer selects the image/O2 route for any of the six targets.
The existing stream tests still cover Linux x86_64/O2 unless a test says otherwise.

Run each command separately from the repository root:

```sh
tools/bound 55 sh ./unisacc.com -O0 -DUNISA_CX_CODEC -I include -iquote exec/c -b lnx/x86_64 -c -funit tools/stream-runtime.c -o out/stream-runtime-unit.o
tools/bound 55 sh ./unisacc.com -O2 -I include -iquote exec/c -b lnx/x86_64 -c -funit cx/runtime/codec.cx -o out/stream-codec-unit.o
tools/bound 55 sh ./unisacc.com -O2 -I include -iquote exec/c -b lnx/x86_64 -c -funit cx/sys/process_group.cx -o out/stream-process-unit.o
tools/bound 55 sh ./unisacc.com -O2 -I include -iquote exec/c -b lnx/x86_64 -c -funit cx/runtime/stream_output.cx -o out/stream-output-unit.o
tools/bound 55 sh ./unisacc.com -O2 -I include -iquote exec/c -b lnx/x86_64 -c -funit cx/runtime/stream_resource.cx -o out/stream-resource-unit.o
tools/bound 55 sh ./unisacc.com -O2 -I include -iquote exec/c -b lnx/x86_64 -c -funit cx/runtime/stream_attributes.cx -o out/stream-attributes-unit.o
tools/bound 55 sh ./unisacc.com -O2 -I include -iquote exec/c -b lnx/x86_64 -c -funit cx/runtime/packagefooter.cx -o out/stream-footer-unit.o
tools/bound 55 sh ./unisacc.com -O2 -I include -iquote exec/c -b lnx/x86_64 -c -funit cx/runtime/attribute_frame.cx -o out/stream-attribute-frame-unit.o
tools/bound 55 sh ./unisacc.com -O2 -I include -iquote exec/c -b lnx/x86_64 -c -funit cx/runtime/model_number.cx -o out/stream-model-number-unit.o
tools/bound 55 sh ./unisacc.com -fno-trim-libc -o out/stream-runtime.com out/stream-runtime-unit.o out/stream-codec-unit.o out/stream-process-unit.o out/stream-output-unit.o out/stream-resource-unit.o out/stream-attributes-unit.o out/stream-footer-unit.o out/stream-attribute-frame-unit.o out/stream-model-number-unit.o
tools/bound 30 sh ./unisacc.com tools/stream-pack.cx -- out/stream-runtime.com ./unisacc.com out/streams-new
tools/bound 30 sh ./unisacc.com tools/stream-pack.cx -- out/stream-runtime.com ./unisacc.com out/streams-arm64-new lnx/arm64
make streams STREAM_DIR=out/streams-new STREAM_OUT=out/stream-check-new
```

Both output directories must be new. Material preparation refuses overwrites and
retains partial output on failure for inspection. The runtime binary is a shared
C substrate linked with separate CX codec, package footer, stream output, resource parser,
attribute validator, attribute-frame processor, model number reader and Linux x86-64 process provider
units. The large runtime unit uses O0 to fit the command bound; this development
construction is not a performance claim. The product builds these units because
executable-format and pipeline behavior are under test. All new CX source uses
modules and its implicit runtime, without
manual include directives. No Python or external C compiler is invoked by these
commands after the existing watchdog is available; the watchdog still has its
documented host C cold-bootstrap dependency.

The optional last argument is one of `lnx/x86_64`, `lnx/arm64`, `osx/x86_64`,
`osx/arm64`, `win/x86_64`, or `win/arm64`; the default is `lnx/x86_64`.
Each route needs its own new output directory. This selects the output target;
it does not execute on a foreign host.

`stream-pack.cx` imports `cxlib.streampack`. It validates the current P3 directory,
model and resource extents, preserves original compressed model records and all
resource records, and appends one package per runtime shell. It does not construct
weights or inflate networks. `streams.json` records input, runtime, model-record,
resource-section and artifact hashes, along with the selected route. This is
PREPARED material, not a PASS.
The extraction implementation was retained from the archived material tool;
no old test runner, assertions or PASS records are used by the new tests.

The seven artifacts are `unisacc-stream1.com` through `unisacc-stream7.com`, in
e2/e1/e3/e4/prune/lower/elf order. Each accepts `[INPUT|-] [SOURCE_LABEL]
[INCLUDE_DIR]`; `-` or absent INPUT reads stdin, stdout contains the primary result,
and stderr contains diagnostics. `UNISA_ATTRIBUTES` is the explicit optional
sidecar. E1 carries USLATTR1 in the pipeline; E3 imports it. Records are five bytes
in the E1 frame and nine bytes after unit tagging in the sidecar. The default
executor budget is 200,000,000 transitions, as in the production executor source.

`tests/streams.cx` is a new partial contract batch. Handwritten sources expect
exit 7 both directly and after a constructor. It checks stage acceptance,
attribute extents, ELF identity, exact comparison with a separately invoked full
product route, actual generated-program execution, a real seven-process pipeline,
a named relocation-overrun refusal, empty E4 identity and conservative prune
preservation. Every child has its own native watchdog. Outputs and diagnostics
are retained in the fresh output directory. Before/after hashes reject artifact
changes during the batch. Compilation, construction, loading, inference, output
and validation phase times remain UNMEASURED.

This batch passed 40 assertions locally. It is not complete independent semantic
coverage of every stream: positive chain inputs couple to upstream output, the
full-route comparison shares the models, tapecheck is outside the isolated chain,
and full RI/resource, eighteen-net, six-target and host contracts remain missing.
It does not run from `tests/run.cx --all`; that gate continues to report NOT_READY.
