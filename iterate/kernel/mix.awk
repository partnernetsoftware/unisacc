# The scratch INTEGRATOR [J10 step 3]: a MIXED unisa_model.inc.
#   awk -v NEW=<genmodel output> -f mix.awk OLD > MIXED
# From OLD (the shipped file, Python-written) it keeps, located by explicit
# markers and never by line number:
#   - the provenance header: every line before "/* S_<stage>" (exactly once);
#   - everything from the first line starting "/* ENC_" to the end.
# Nothing else: TYPEV is generated (typekw slice), OLD's TYPEV is not read.
# From NEW (genmodel) it takes "/* S_<stage>" (exactly once) up to, not
# including, genmodel's END line (exactly once), verbatim; NEW must hold
# exactly one 'char *TYPEV = ' line and no TYPEV placeholder marker.
# Any marker miss: stderr, exit 1.
BEGIN {
    GMEND = "/* genmodel: END of the vocab / BF / BH region (ENC_* is not written here) */"
}
index($0, "/* S_<stage>") == 1 { nS++; ph = 1 }
ph == 0 { head[++nh] = $0 }
index($0, "/* ENC_") == 1 && !tl { tl = 1 }
tl { tail[++ntl] = $0 }
END {
    e = ""
    if (nS != 1) e = e " [OLD: '/* S_<stage>' " nS + 0 " times]"
    if (!tl) e = e " [OLD: no '/* ENC_' line]"
    while ((getline l < NEW) > 0) {
        if (index(l, "/* S_<stage>") == 1) { gS++; on = 1 }
        if (l == GMEND) { gE++; on = 0 }
        if (!on) continue
        if (index(l, "placeholder") && index(l, "TYPEV")) gB++
        if (index(l, "char *TYPEV = ") == 1) gT++
        body[++nb] = l
    }
    close(NEW)
    if (gS != 1 || gE != 1 || gB != 0 || gT != 1) e = e " [NEW: S " gS + 0 ", END " gE + 0 ", char *TYPEV " gT + 0 " (want 1 each), TYPEV placeholder lines " gB + 0 " (want 0)]"
    if (e != "") { print "mix.awk:" e > "/dev/stderr"; exit 1 }
    for (i = 1; i <= nh; i++) print head[i]
    for (i = 1; i <= nb; i++) print body[i]
    for (i = 1; i <= ntl; i++) print tail[i]
}
