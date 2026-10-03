"""Optional located-token reader and retained diagnostic map.
Parsing stays on the original token buffer, so rewinds and bounded input
views keep their existing offsets. source_pos is a preprocessed-text offset;
TOKEN_POS preserves it for a saved token-buffer position after later reads.
"""
TOKEN_POS=34<<40
SPLICES=35<<40
INCLUDE_LINE=36<<40
INCLUDE_LINES=37<<40
INCLUDE_NAME=38<<40
UNIT_MAP=48<<40
MAP_STRIDE=1<<26
MAP_FIELDS=('source','textlen','forced','auto','nsplice','ninclude','filename')


import json
from pathlib import Path


def rows(name):
    return [line.split("\t") for line in Path(__file__).with_name("tokenlocations-" + name + ".tsv").read_text().splitlines()[1:]]


def install(E,P,ordinal_table=None,token_record=None,ready="START",multi=True):
    from finite_rules import install as install_rules, load as load_rules, install_template
    g=E.g
    enabled={"main"} | ({"multi"} if multi else set()) | ({"record"} if token_record else set())
    bindings={name:globals()[name] for name in ("TOKEN_POS","SPLICES","INCLUDE_LINE","INCLUDE_LINES","INCLUDE_NAME","UNIT_MAP","MAP_STRIDE")}
    bindings.update(ready=ready, token_record=token_record or "RET", ordinal_table=ordinal_table or 0,
                    version2_target="DL.multizero" if multi else "DL.bad",
                    multi_context="DL.multiready" if multi else "DL.bad",
                    multi_prefix="DL.which" if multi else "DL.offset",
                    read_entry="DL.read" if token_record else "NEXT",
                    read_target="IX.have" if ordinal_table is not None else "NX")
    for part,prefix,kind,key in rows("fresh"):
        if part in enabled:
            owner=bindings[prefix[1:]] if prefix.startswith("@") else prefix
            bindings[key]=P(owner + ".tokenlocations_" + key).fresh(kind)
    sequences={name:E.rej(message) for name,message in rows("reject")}
    def actions(section,values):
        return load_rules(Path(__file__).with_name("tokenlocations-actions.tsv"),{},domain=[0],
                          bindings=values,section=section)
    for direction in ("store","load"):
        sequences["map_"+direction]=sum((actions(direction,dict(bindings,field_index=i,field_register="diag_"+field))["field"][0][1]
                                          for i,field in enumerate(MAP_FIELDS)),[])
    sequences["version2"]=actions("version2",bindings)["version2"][0][1] if multi else []
    sequences["ordinal"]=actions("ordinal",bindings)["ordinal"][0][1] if ordinal_table is not None else []
    def rules(section,extra=None,seq=None,classes=None):
        install_rules(g,Path(__file__).parent,"tokenlocations",section=section,
                      bindings=dict(bindings,**(extra or {})),sequences=dict(sequences,**(seq or {})),classes=classes)
    # Replace only the ordinary token entry; all reader consumers share this decoder.
    install_template(g,Path(__file__).parent,"tokenlocations",{},None)
    for part in ("main","multi","record"):
        if part in enabled:
            rules(part)
    for part,name,register,nxt in rows("words"):
        if part in enabled:
            rules("word",dict(word_entry=name,word_register=register,word_next=bindings.get(nxt[1:],nxt) if nxt.startswith("@") else nxt,
                              **{"word"+str(i):name+"."+str(i) for i in range(4)}))
    for part,name,encoded,nxt in rows("magic"):
        if part in enabled:
            value=json.loads(encoded)
            for i,byte in enumerate(value):
                rules("magic",dict(magic_state=name+str(i),magic_next=name+str(i+1) if i+1<len(value) else nxt),classes=dict(magic_byte=[byte]))
    for part,name,left,right,nxt,test,encoded in rows("comparisons"):
        if part in enabled:
            rules("le",dict(le_entry=name,left=left,right=right,le_next=nxt,le_test=bindings[test]),dict(le_prefix=json.loads(encoded)))
    return "DL.magic0"
