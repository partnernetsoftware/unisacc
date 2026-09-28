"""Bind declared library-exit routing; no executor exit recognition.
The LE64 resource names a trusted, non-returning host callback: its first
argument is the script status, taken from the existing syscall spill area.
All six targets use the declared fixed six-GP ABI bridge. Windows long is
not the protocol word: callback arguments are fixed int64 values. Missing or zero bindings preserve ordinary syscalls.
"""
from pathlib import Path
from finite_rules import install as rules
from modelinput import u64

def install(E, os_, regmap, sysa):
    u64(E, 'LE.read', b'\0library/exit', 'library_exit', 'library_hasexit', 'C.fail')
    rules(E.g, Path(__file__).parent, 'libraryexit', section='route',
          bindings=dict(supported='LM.prelude' if os_ in ('osx','lnx','win') else 'C.fail', SYSA=sysa, DIG=112 << 40),
          sequences=dict(fn=E.O('setreg '+regmap['r1']+', imm '),
                         argv=E.O('setreg '+regmap['r0']+', addr '),
                         call=E.O('hostcall '+regmap['r1']+', '+regmap['r0']+'\n'),
                         retname=[('SBOUT',b) for b in regmap['r0'].encode()]))
    from librarymemory import install as memory
    memory(E, os_, regmap, sysa)
