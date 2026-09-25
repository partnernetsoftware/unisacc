# The scratch INTEGRATOR [J10 step 3]: a MIXED unisa_model.inc.
#   awk -v NEW=<genmodel output> -f mix.awk OLD > MIXED
# From OLD (the shipped file, Python-written) it keeps, located by explicit
# markers and never by line number:
#   - the provenance header: every line before "/* S_<stage>" (exactly once);
#   - the TYPEV block: the line starting "/* TYPEV: " (exactly once) through
#     the next line starting "#define NTYPEV ", which must be 3 lines;
#   - everything from the first line starting "/* ENC_" to the end.
# From NEW (genmodel) it takes "/* S_<stage>" (exactly once) up to, not
# including, genmodel's END line (exactly once), with the TYPEV placeholder
# -- "/* TYPEV: BEGIN placeholder" through "/* TYPEV: END placeholder */",
# each exactly once -- replaced by OLD's TYPEV block.  Only this integrator
# reads OLD's TYPEV; genmodel never does.  Any marker miss: stderr, exit 1.
BEGIN {
    GMEND = "/* genmodel: END of the vocab / BF / BH region (ENC_* is not written here) */"
    TB = "/* TYPEV: BEGIN placeholder"; TE = "/* TYPEV: END placeholder */"
}
index($0, "/* S_<stage>") == 1 { nS++; ph = 1 }
ph == 0 { head[++nh] = $0 }
index($0, "/* TYPEV: ") == 1 { nty++; ty = 1 }
ty == 1 { tyl[++nt] = $0; if (index($0, "#define NTYPEV ") == 1) ty = 2; next }
index($0, "/* ENC_") == 1 && !tl { tl = 1 }
tl { tail[++ntl] = $0 }
END {
    e = ""
    if (nS != 1) e = e " [OLD: '/* S_<stage>' " nS + 0 " times]"
    if (nty != 1 || ty != 2 || nt != 3) e = e " [OLD: TYPEV block not found exactly once as 3 lines]"
    if (!tl) e = e " [OLD: no '/* ENC_' line]"
    while ((getline l < NEW) > 0) {
        if (index(l, "/* S_<stage>") == 1) { gS++; on = 1 }
        if (l == GMEND) { gE++; on = 0 }
        if (!on) continue
        if (index(l, TB) == 1) { gB++; skip = 1; for (i = 1; i <= nt; i++) body[++nb] = tyl[i]; continue }
        if (l == TE) { gT++; skip = 0; continue }
        if (!skip) body[++nb] = l
    }
    close(NEW)
    if (gS != 1 || gE != 1 || gB != 1 || gT != 1) e = e " [NEW: S " gS + 0 ", END " gE + 0 ", TYPEV BEGIN " gB + 0 ", TYPEV END " gT + 0 "; want 1 each]"
    if (e != "") { print "mix.awk:" e > "/dev/stderr"; exit 1 }
    for (i = 1; i <= nh; i++) print head[i]
    for (i = 1; i <= nb; i++) print body[i]
    for (i = 1; i <= ntl; i++) print tail[i]
}
