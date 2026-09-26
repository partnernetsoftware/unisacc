"""Optional native-memory binding, expressed as ordinary model actions.
The first pass without mapped bases supplies a size plan; the second pass
encodes at the actual bases. Neither pass asks a reference compiler.
"""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from modelinput import u64
from unisa.tape import DATA_BASE


def install(E, fail):
    P=E.P
    u64(E,'ML.mode',b'\0process/argc','ml_argc','memory_mode',fail)
    u64(E,'ML.argv',b'\0process/argv','ml_argv','ml_hasargv',fail)
    u64(E,'ML.text',b'\0memory/text','ml_text','ml_hast',fail)
    u64(E,'ML.data',b'\0memory/data','ml_data','ml_hasd',fail)
    P('LAYOUT').call('LAY.default').call('ML.mode').branch({1:'ML.file'},'ML.bind',[('CMPI','memory_mode',0)])
    P('ML.file').a(('ALU','or','ml_args','has_argc','has_argv')).branch({1:'RET'},fail,[('CMPI','ml_args',0)])
    P('ML.bind').branch({1:fail},'ML.bases',[('CMPI','target_os',3)]) # imports need a process resolver first
    P('ML.bases').call('ML.argv').branch({1:fail},'ML.addresses',[('CMPI','ml_hasargv',0)])
    P('ML.addresses').call('ML.text').call('ML.data').branch({1:'ML.no_text'},'ML.have_text',[('CMPI','ml_hast',0)])
    P('ML.no_text').branch({1:'RET'},fail,[('CMPI','ml_hasd',0)])
    P('ML.have_text').branch({1:fail},'ML.apply',[('CMPI','ml_hasd',0)])
    P('ML.apply').a(('COPYW','text_va','ml_text'),('COPYW','data_va','ml_data'),('A64I','sub','data_shift','data_va',DATA_BASE)).ret()
