"""Model prepass for independently lexed translation units.
Input: repeated LE32 length + typed-token bytes. Output: one typed stream.
The model performs framing validation and file-static name isolation. C only
frames bytes. Declaration forms outside this scanner are named refusals.
"""
import importlib.util,json,pathlib,sys
s=importlib.util.spec_from_file_location('unit_grammar',pathlib.Path(__file__).parents[1]/'parse/gen.py')
E=importlib.util.module_from_spec(s);s.loader.exec_module(E)
P=E.P;g=E.g;TK_ID=E.TK_ID
STATIC=1<<40

from finite_rules import install as install_rules, load as load_rules


def rows(name):
    return [line.split("\t") for line in pathlib.Path(__file__).with_name("units-"+name+".tsv").read_text().splitlines()[1:]]


def build(locations=False):
    # Physical qualifiers are transport tokens here, not parser-skipped spans.
    qualifiers=[row[0] for row in rows("qualifiers")]
    for name in qualifiers:
        E.WORDS.append(name);E.TK[name]=max(E.TK.values())+1
    E.tokenizer();E.prn();E.fconv()
    for name in qualifiers:
        for state,row in load_rules(pathlib.Path(__file__).with_name("units-byte.tsv"),{},domain=[10],
                bindings=dict(qualifier_state="NX"+name,qualifier_token=E.TK[name]),section="qualifier").items():
            for key,(target,acts) in row.items():g.st[state][1][key]=(target,g.seq(acts))
    from strings import token_span
    token_span(E,P)
    selection=rows("builtin")
    builtin=[E.TK[word] for word in E.WORDS if any(word.startswith(prefix) and word!=exclude for prefix,exclude in selection)]
    tokens=dict(E.TK,identifier=TK_ID)
    classes={name:[tokens[token]] for name,token in rows("tokens")};classes["builtin"]=builtin
    sequences={name:E.O(json.loads(value)) for name,value in rows("text")}
    sequences.update((name,E.rej(message)) for name,message in rows("reject"))
    def rules(section,extra=None,extra_classes=None):
        bindings=dict(STATIC=STATIC,**(extra or {}))
        for part,prefix,kind,key in rows("fresh"):
            if part==section:bindings[key]=P(prefix+".units_"+key).fresh(kind)
        install_rules(g,pathlib.Path(__file__).parent,"units",section=section,bindings=bindings,
                      classes=dict(classes,**(extra_classes or {})),sequences=sequences)
    rules("main0")
    for i in range(4):
        rules("length",dict(length_byte="L"+str(i),length_step="L"+str(i)+"b",length_next="L"+str(i+1) if i<3 else "EXTENT",length_shift=8*i))
    rules("main2")
    for part,token,slot in rows("counters"):
        rules(part,dict(counter_entry="SD."+token,counter_ok="SD."+token+"ok",counter_slot=slot))
    rules("main5")
    for entry,nxt in rows("separators"):
        rules("separator",dict(separator_entry=entry,separator_next=nxt))
    rules("main7")
    for i,byte in enumerate(json.loads(rows("trailer")[0][0])):
        rules("trailer",dict(trailer_state="TRAIL"+str(i),trailer_next="TRAIL"+str(i+1)),dict(trailer_byte=[byte]))
    rules("main9")
    install_rules(g, pathlib.Path(__file__).parent, 'unit-labels', bindings=dict(ID=TK_ID, GOTO=E.TK['goto'], COLON=E.TK[':']), section='main')
    rules("main12")
    if locations:
        from unitlocations import install
        install(E,P)
    from layoutprovenance import units as source_provenance
    start=source_provenance(E,P,locations)
    g.finish()
    return {'start':start,'states':{n:[m,{str(k):v for k,v in r.items()}] for n,(m,r) in g.st.items()},'seqs':[list(map(list,s)) for s in g.seqs]}

if __name__=='__main__':
    d=build("--locations" in sys.argv);pathlib.Path(sys.argv[1]).write_text(json.dumps(d,separators=(',',':')))
    print('unit framing/static isolation:',len(d['states']),'states')
