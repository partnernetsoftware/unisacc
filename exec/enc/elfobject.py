"""ELF64 ET_REL writer delta, consuming a checked object plan.

install(E, regions, arch) creates EO.write, returning through RET. Caller supplies
registers eo_textlen, eo_nzend, eo_datalen, eo_nrel, eo_nsym, eo_nlocal,
eo_strsize and eo_tapelen. nsym counts named symbols, excluding four anchors.
Sparse byte regions: text/data/strings/tape. strings includes initial NUL.
Symbols are already in local/global/UND order; each 4-cell row is
(name string offset, ELF info, section index, section-relative value).
Relocations are 4-cell rows (offset, ELF type, ELF symbol index, signed addend).
All names, symbol/relocation decisions belong to the preceding plan delta.
No executor primitive or host writer is introduced here. Input extent/plan
validation is a caller precondition; this module guards nonnegative data
extents, nzend <= datalen, and relocation symbol indices < 4 + nsym.
"""


def install(E, regions, arch="x86_64"):
    assert arch in ("x86_64", "arm64")
    assert set(regions) == {"text", "data", "strings", "tape", "symbols", "relocs"}
    P = E.P
    def field(p, width, value):
        p.a(("LDI" if isinstance(value, int) else "COPYW", "eo_v", value))
        for _ in range(width):
            p.a(("A64I", "and", "eo_byte", "eo_v", 255),
                ("OUTW", "eo_byte"), ("A64I", "shr", "eo_v", "eo_v", 8))
        return p
    def pad(p, target):
        return p.a(("COPYW", "eo_padto", target)).call("EO.pad")
    def blob(p, region, length):
        return p.a(("LDI", "eo_base", regions[region]),
                   ("COPYW", "eo_count", length), ("LDI", "eo_i", 0)).call("EO.blob")
    def sh(p, name, ty, flags, off, size, link, info, align, entsize):
        for width, v in ((4,name),(4,ty),(8,flags),(8,0),(8,off),(8,size),
                         (4,link),(4,info),(8,align),(8,entsize)):
            field(p,width,v)
    p=P("EO.write").a(("LDI","eo_zero",0),("C64","eo_nzend","eo_zero"))
    p.branch({(1,2):"EO.extenttotal"},("rej","not covered: object data extent"))
    p=P("EO.extenttotal").a(("C64","eo_datalen","eo_zero"))
    p.branch({(1,2):"EO.extentorder"},("rej","not covered: object data extent"))
    p=P("EO.extentorder").a(("C64U","eo_nzend","eo_datalen"))
    p.branch({(0,1):"EO.layout"}, ("rej","not covered: object data extent"))
    p=P("EO.layout").a(
        ("A64I","add","eo_doff","eo_textlen",79),("A64I","and","eo_doff","eo_doff",-16),
        ("A64","add","eo_roff","eo_doff","eo_nzend"),("A64I","add","eo_roff","eo_roff",7),("A64I","and","eo_roff","eo_roff",-8),
        ("A64I","mul","eo_relsize","eo_nrel",24),("A64","add","eo_soff","eo_roff","eo_relsize"),
        ("A64I","add","eo_allsyms","eo_nsym",4),("A64I","mul","eo_symsize","eo_allsyms",24),
        ("A64","add","eo_stroff","eo_soff","eo_symsize"),("A64","add","eo_shstroff","eo_stroff","eo_strsize"),
        ("A64","sub","eo_bss","eo_datalen","eo_nzend"),("A64I","add","eo_bss_off","eo_doff",0),
        ("A64","add","eo_bss_off","eo_bss_off","eo_nzend"),("A64I","add","eo_firstglobal","eo_nlocal",4),
        ("CMPI","eo_tapelen",0))
    p.branch({1:"EO.notape"}, "EO.hastape")
    P("EO.notape").a(("LDI","eo_shstrsize",55),("LDI","eo_shnum",8)).goto("EO.offsets")
    P("EO.hastape").a(("LDI","eo_shstrsize",67),("LDI","eo_shnum",9)).goto("EO.offsets")
    p=P("EO.offsets").a(("A64","add","eo_tapeoff","eo_shstroff","eo_shstrsize"),
        ("A64","add","eo_shoff","eo_tapeoff","eo_tapelen"),("A64I","add","eo_shoff","eo_shoff",7),("A64I","and","eo_shoff","eo_shoff",-8))
    p.a(E.O("\x7fELF"))
    for b in [2,1,1,0]+[0]*8:p.a(("OUT",b))
    for w,v in [(2,1),(2,183 if arch=="arm64" else 62),(4,1),(8,0),(8,0),(8,"eo_shoff"),(4,0),(2,64),(2,0),(2,0),(2,64),(2,"eo_shnum"),(2,7)]:field(p,w,v)
    blob(p,"text","eo_textlen");pad(p,"eo_doff");blob(p,"data","eo_nzend");pad(p,"eo_roff")
    p.a(("LDI","eo_j",0)).goto("EO.relcheck")
    p=P("EO.relcheck").a(("C64U","eo_j","eo_nrel"));p.branch({0:"EO.rel"},"EO.symstart")
    p=P("EO.rel").a(("A64I","mul","eo_row","eo_j",4),
        ("A64I","add","eo_idx","eo_row",2),("LDX","eo_item","eo_idx",regions["relocs"]),
        ("C64U","eo_item","eo_allsyms"))
    p.branch({0:"EO.relfields"},("rej","not covered: object relocation symbol"))
    p=P("EO.relfields")
    for k,w in enumerate((8,4,4,8)):
        p.a(("A64I","add","eo_idx","eo_row",k),("LDX","eo_item","eo_idx",regions["relocs"]))
        field(p,w,"eo_item")
    p.a(("A64I","add","eo_j","eo_j",1)).goto("EO.relcheck")
    p=P("EO.symstart");pad(p,"eo_soff")
    for name,info,sec,val in [(0,0,0,0),(0,3,1,0),(0,3,2,0),(0,3,3,0)]:
        for w,v in [(4,name),(1,info),(1,0),(2,sec),(8,val),(8,0)]:field(p,w,v)
    p.a(("LDI","eo_j",0)).goto("EO.symcheck")
    p=P("EO.symcheck").a(("C64U","eo_j","eo_nsym"));p.branch({0:"EO.sym"},"EO.strings")
    p=P("EO.sym").a(("A64I","mul","eo_row","eo_j",4))
    for k,w in enumerate((4,1,2,8)):
        p.a(("A64I","add","eo_idx","eo_row",k),("LDX","eo_item","eo_idx",regions["symbols"]))
        field(p,w,"eo_item")
        if k==1:field(p,1,0)
    field(p,8,0);p.a(("A64I","add","eo_j","eo_j",1)).goto("EO.symcheck")
    p=P("EO.strings");blob(p,"strings","eo_strsize")
    for b in b"\0.text\0.data\0.bss\0.rela.text\0.symtab\0.strtab\0.shstrtab\0":p.a(("OUT",b))
    p.a(("CMPI","eo_tapelen",0)).branch({1:"EO.sections"},"EO.tape")
    p=P("EO.tape")
    for b in b".unisa.tape\0":p.a(("OUT",b))
    blob(p,"tape","eo_tapelen");p.goto("EO.sections")
    p=P("EO.sections");pad(p,"eo_shoff")
    for row in [(0,0,0,0,0,0,0,0,0),(1,1,6,64,"eo_textlen",0,0,16,0),
                (7,1,3,"eo_doff","eo_nzend",0,0,16,0),(13,8,3,"eo_bss_off","eo_bss",0,0,16,0),
                (18,4,64,"eo_roff","eo_relsize",5,1,8,24),(29,2,0,"eo_soff","eo_symsize",6,"eo_firstglobal",8,24),
                (37,3,0,"eo_stroff","eo_strsize",0,0,1,0),(45,3,0,"eo_shstroff","eo_shstrsize",0,0,1,0)]:sh(p,*row)
    p.a(("CMPI","eo_tapelen",0)).branch({1:"EO.done"},"EO.tapesh")
    p=P("EO.tapesh");sh(p,55,1,0,"eo_tapeoff","eo_tapelen",0,0,1,0);p.goto("EO.done")
    P("EO.done").ret()
    p=P("EO.pad").a(("OLEN","eo_pos"),("C64U","eo_pos","eo_padto"));p.branch({0:"EO.padbyte",1:"EO.padret"},("rej","not covered: object layout overlap"))
    P("EO.padbyte").a(("OUT",0)).goto("EO.pad")
    P("EO.padret").ret()
    p=P("EO.blob").a(("C64U","eo_i","eo_count"));p.branch({0:"EO.blobbyte"},"EO.blobret")
    P("EO.blobbyte").a(("A64","add","eo_idx","eo_base","eo_i"),("LDX","eo_byte","eo_idx",0),("OUTW","eo_byte"),("A64I","add","eo_i","eo_i",1)).goto("EO.blob")
    P("EO.blobret").ret()
    return "EO.write"
