# The identity check of the retired draftfetch.py check() (archive/reference-py/release/tools/), one reason per line;
# empty when the draft is the one named. Called by .github/workflows/defender-scan.yml.
[ (if ($r.id|tostring) != $rid then "API returned release \($r.id) for requested \($rid)" else empty end),
  (if ($a.id|tostring) != $aid then "API returned asset \($a.id) for requested \($aid)" else empty end),
  (if $r.tag_name != $tag then "release \($r.id) has tag '\($r.tag_name)', not '\($tag)'" else empty end),
  (if $r.draft != true then "release \($r.id) is not a draft (already published?)" else empty end),
  (if $r.target_commitish != $tc then "release targets '\($r.target_commitish)', not '\($tc)'" else empty end),
  (if ([$r.assets[]?.id] | index($a.id)) == null then "asset \($a.id) does not belong to release \($r.id)" else empty end),
  (if $a.name != "unisacc.com" then "asset \($a.id) is '\($a.name)', not unisacc.com" else empty end)
] | .[]
