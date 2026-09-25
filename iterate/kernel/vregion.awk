# The compared vocab / BF / BH region of a unisa_model.inc [J10 step 3, slice 2].
#   awk -v END_MODE=py|gm -v EXP=<expected symbol list> -f vregion.awk FILE
# START anchor: the "}" that closes "int model_dims(void) {"; that line must
#   occur exactly once in FILE, and the region is everything after its "}".
# END anchor (exclusive):
#   py  the first line starting "/* ENC_" (the first ENC comment, excluded);
#       no "char *ENC_" line may come before it;
#   gm  the line "/* genmodel: END of the vocab / BF / BH region ...", which
#       must occur exactly once.
# Removed: whole-line /* ... */ comment blocks (genmodel's comments name its
# own inputs), and ONLY two exact lines: 'char *TYPEV = "...";' and
# '#define NTYPEV <digits>' (TYPEV is not generated in this slice).
# Checked after that: the region is complete and non-empty, and its symbols
# (the name of every "char *X =" and "#define X" line) are EXACTLY the EXP
# list (one per line): none missing, none extra, none twice.  Any failure
# prints to stderr and exits 1; stdout is then not to be used.
BEGIN {
    GMEND = "/* genmodel: END of the vocab / BF / BH region (ENC_* is not written here) */"
    if (END_MODE != "py" && END_MODE != "gm") { print "vregion: END_MODE must be py or gm" > "/dev/stderr"; bad = 1; exit 1 }
    while ((getline l < EXP) > 0) if (l != "") { if (l in want) { print "vregion: EXP repeats " l > "/dev/stderr"; bad = 1; exit 1 }; want[l] = 1; nwant++ }
    close(EXP)
    if (nwant == 0) { print "vregion: empty EXP " EXP > "/dev/stderr"; bad = 1; exit 1 }
}
$0 == "int model_dims(void) {" { nmd++ }
$0 == GMEND { ngmend++ }
st == 0 { if ($0 == "int model_dims(void) {") inmd = 1; else if (inmd && $0 == "}") st = 1; next }
st == 2 { if (END_MODE == "py" && index($0, "char *ENC_") == 1 && !seenenc) seenenc = 1; next }
END_MODE == "py" && index($0, "/* ENC_") == 1 { st = 2; ended = 1; next }
END_MODE == "gm" && $0 == GMEND { st = 2; ended = 1; next }
END_MODE == "py" && index($0, "char *ENC_") == 1 { print "vregion: char *ENC_ before the first ENC comment" > "/dev/stderr"; bad = 1; exit 1 }
incmt { if (index($0, "*/")) incmt = 0; next }
/^\/\*/ { if (!index($0, "*/")) incmt = 1; next }
/^char \*TYPEV = "[^"]*";$/ { ntyv++; next }
/^#define NTYPEV [0-9]+$/ { nntyv++; next }
{
    out[++n] = $0
    sym = ""
    if (index($0, "char *") == 1) { sym = substr($0, 7); sub(/ .*/, "", sym) }
    else if (index($0, "#define ") == 1) { sym = substr($0, 9); sub(/ .*/, "", sym) }
    if (sym != "") { if (sym in got) dup = dup " " sym; got[sym] = 1; ngot++ }
    if ($0 != "") nonblank++
}
END {
    if (bad) exit 1
    e = ""
    if (nmd != 1) e = e " [start anchor 'int model_dims(void) {' hit " nmd + 0 " times]"
    if (st == 0) e = e " [start anchor not reached]"
    if (!ended) e = e " [end anchor not hit]"
    if (END_MODE == "gm" && ngmend != 1) e = e " [gm end anchor hit " ngmend + 0 " times]"
    if (END_MODE == "py" && !seenenc) e = e " [no char *ENC_ after the end anchor]"
    if (incmt) e = e " [unclosed comment]"
    if (ntyv > 1 || nntyv > 1 || ntyv != nntyv) e = e " [TYPEV lines " ntyv + 0 "/" nntyv + 0 "]"
    if (nonblank == 0) e = e " [region empty]"
    if (dup != "") e = e " [symbols twice:" dup "]"
    for (s in want) if (!(s in got)) e = e " [missing " s "]"
    for (s in got) if (!(s in want)) e = e " [unexpected " s "]"
    if (e != "") { print "vregion(" END_MODE "): " FILENAME ":" e > "/dev/stderr"; exit 1 }
    for (i = 1; i <= n; i++) print out[i]
    print "vregion(" END_MODE "): " n " lines, " ngot " symbols = EXP (" nwant "), TYPEV lines removed " ntyv + 0 "+" nntyv + 0 > "/dev/stderr"
}
