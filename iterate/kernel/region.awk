# The compared region of a unisa_model.inc [J10 step 3]: from the first line
# that starts "#define S_" through the "}" line that closes
# "int model_dims(void) {", with every /* ... */ comment block removed (a
# comment is whole lines here: it starts a line with "/*" and ends at the
# first line containing "*/").  Everything else in the span -- S_* defines,
# MODEL, NSTAGE, the STAGE_* / act / z declarations, DENSE, DENSE_LEN,
# STAGE_DOFF/NH, model_dims() and blank lines -- is kept byte for byte.
done { next }
!on && /^#define S_/ { on = 1 }
!on { next }
incmt { if (index($0, "*/")) incmt = 0; next }
/^\/\*/ { if (!index($0, "*/")) incmt = 1; next }
{ print }
inmd && $0 == "}" { done = 1 }
$0 == "int model_dims(void) {" { inmd = 1 }
END { if (!done) { print "region.awk: no complete region" > "/dev/stderr"; exit 1 } }
