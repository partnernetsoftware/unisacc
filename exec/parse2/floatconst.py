"""Decimal and hexadecimal literal conversion in delta actions.

32-bit limbs keep M and powers of ten exact. Normalise their ratio, take
only the destination's significant bits, and round once (nearest/even),
including subnormals. The product's 160-limb bound is checked, never wrapped.
Integer and character token paths remain in the existing reader.
"""


def install(E, P):
    g = E.g
    A, D = 30 << 40, 31 << 40
    bad = 'DEAD.decimal'
    g.on(bad, range(257), 'DEAD', E.rej('not covered: decimal floating constant'), 'r')
    # Classify the complete token first: 009.5 is decimal, not an octal
    # integer, and a hex digit 'e' must not be mistaken for an exponent.
    g.st['SPANNUM.integer'] = g.st.pop('SPANNUM')
    P('SPANNUM').a(('INPUSHX', 'ps')).goto('DF.scan')
    g.on('DF.scan', b'.eE', 'DF.scanned')
    g.on('DF.scan', b'xX', 'HF.scan', [('ADV',)])
    g.on('DF.scan', b"'\n", 'DF.integer')
    g.on('HF.scan', b'pP', 'HF.scanned')
    g.on('HF.scan', [10, 256], 'DF.integer')
    g.els('HF.scan', 'HF.scan', [('ADV',)])
    P('HF.scanned').a(('INPOP',)).goto('HF')
    g.on('DF.scan', [256], 'DF.integer')
    g.els('DF.scan', 'DF.scan', [('ADV',)])
    P('DF.scanned').a(('INPOP',)).goto('DF')
    P('DF.integer').a(('INPOP',)).goto('SPANNUM.integer')
    # Old reader float entries remain valid, but use the same conversion.
    del g.st['NUMF']
    P('NUMF').goto('DF')
    del g.st['NUMFL']
    P('NUMFL').goto('DF')
    P('DF').a(('LDI', 'df_hex', 0)).goto('DF.init')
    P('HF').a(('LDI', 'df_hex', 1)).goto('DF.init')
    P('DF.init').a(('JUMP', 'ps'), ('LDI', 'df_offset', 0), ('LDI', 'df_n', 1), ('LDI', 'df_z', 0),
              ('STX', 'df_z', A, 'df_z'), ('LDI', 'df_frac', 0),
              ('LDI', 'df_exp', 0), ('LDI', 'df_seen', 0),
              ('LDI', 'df_mbits', 52), ('LDI', 'df_bias', 1023),
              ('LDI', 'df_emin', -1022)).branch({1: 'HF.prefix'}, 'DF.digits', [('CMPI', 'df_hex', 1)])
    g.on('HF.prefix', [48], 'HF.x', [('ADV',)])
    g.els('HF.prefix', bad)
    g.on('HF.x', b'xX', 'HF.digits', [('ADV',)])
    g.els('HF.x', bad)
    for digit, chars in enumerate(['0','1','2','3','4','5','6','7','8','9','aA','bB','cC','dD','eE','fF']):
        g.on('HF.digits', chars.encode(), 'HF.digit', [('ADV',), ('LDI', 'df_digit', digit)])
    P('HF.digit').a(('LDI', 'df_mul', 16), ('COPYW', 'df_carry', 'df_digit')).call('DF.mulA').a(
        ('ALU', 'sub', 'df_exp', 'df_exp', 'df_frac'), ('LDI', 'df_seen', 1)).goto('HF.digits')
    g.on('HF.digits', [46], 'HF.dot', [('ADV',)])
    P('HF.dot').branch({1: 'HF.dotok'}, bad, [('CMPI', 'df_frac', 0)])
    P('HF.dotok').a(('LDI', 'df_frac', 4)).goto('HF.digits')
    g.on('HF.digits', b'pP', 'DF.exponent', [('ADV',)])
    g.els('HF.digits', bad)
    for d in range(10):
        g.on('DF.digits', [48+d], 'DF.digit', [('ADV',), ('LDI', 'df_digit', d)])
    P('DF.digit').a(('LDI', 'df_mul', 10), ('COPYW', 'df_carry', 'df_digit')).call('DF.mulA').a(
        ('ALU', 'sub', 'df_exp', 'df_exp', 'df_frac'),
        ('LDI', 'df_seen', 1)).goto('DF.digits')
    g.on('DF.digits', [46], 'DF.dot', [('ADV',)])
    P('DF.dot').branch({1: bad}, 'DF.dotok', [('CMPI', 'df_frac', 1)])
    P('DF.dotok').a(('LDI', 'df_frac', 1)).goto('DF.digits')
    g.on('DF.digits', b'eE', 'DF.exponent', [('ADV',)])
    P('DF.exponent').a(('LDI', 'df_ev', 0), ('LDI', 'df_sign', 1), ('LDI', 'df_edigits', 0)).goto('DF.sign')
    g.on('DF.sign', [45], 'DF.edigits', [('ADV',), ('LDI', 'df_sign', -1)])
    g.on('DF.sign', [43], 'DF.edigits', [('ADV',)])
    g.els('DF.sign', 'DF.edigits')
    for d in range(10):
        g.on('DF.edigits', [48+d], 'DF.edigit', [('ADV',), ('LDI', 'df_digit', d), ('LDI', 'df_edigits', 1)])
    P('DF.edigit').branch({0: 'DF.eadd'}, 'DF.edigits', [('CMPI', 'df_ev', 100000)])
    P('DF.eadd').a(('ALUI', 'mul', 'df_ev', 'df_ev', 10), ('ALU', 'add', 'df_ev', 'df_ev', 'df_digit')).goto('DF.edigits')
    g.els('DF.edigits', 'DF.eend')
    P('DF.eend').branch({1: bad}, 'DF.eok', [('CMPI', 'df_edigits', 0)])
    P('DF.eok').a(('ALU', 'mul', 'df_ev', 'df_ev', 'df_sign'), ('ALU', 'add', 'df_exp', 'df_exp', 'df_ev')).goto('DF.suffix')
    g.els('DF.digits', 'DF.suffix')
    g.on('DF.suffix', b'fF', 'DF.end', [('ADV',), ('LDI', 'df_mbits', 23), ('LDI', 'df_bias', 127), ('LDI', 'df_emin', -126)])
    g.on('DF.suffix', b'lL', 'DF.end', [('ADV',)])
    g.on('DF.suffix', [10], 'DF.end')
    g.els('DF.suffix', 'DEAD', E.rej('not covered: decimal floating suffix'))
    g.on('DF.end', [10], 'DF.convert', [('MARK', 'pe'), ('ADV',)])
    g.els('DF.end', 'DEAD', E.rej('not covered: decimal floating suffix'))
    # Nonnegative little-endian limb multiply with initial carry (also add).
    for tag, base, size in (('A', A, 'df_n'), ('D', D, 'df_dn')):
        P('DF.mul'+tag).a(('LDI', 'df_j', 0)).label('DF.ml'+tag).branch({0: 'DF.mm'+tag}, 'DF.mc'+tag, [('CMP', 'df_j', size)])
        P('DF.mm'+tag).a(('LDX', 'df_t', 'df_j', base), ('A64', 'mul', 'df_t', 'df_t', 'df_mul'),
            ('A64', 'add', 'df_t', 'df_t', 'df_carry'), ('A64I', 'shr', 'df_carry', 'df_t', 32),
            ('A64I', 'and', 'df_t', 'df_t', 4294967295), ('STX', 'df_j', base, 'df_t'),
            ('ALUI', 'add', 'df_j', 'df_j', 1)).goto('DF.ml'+tag)
        P('DF.mc'+tag).branch({1: 'RET'}, 'DF.cap'+tag, [('CMPI', 'df_carry', 0)])
        P('DF.cap'+tag).branch({0: 'DF.append'+tag}, bad, [('CMPI', size, 160)])
        P('DF.append'+tag).a(('STX', size, base, 'df_carry'), ('ALUI', 'add', size, size, 1)).ret()
        # Trim high zeros and measure the leading limb.
        P('DF.bits'+tag).a(('COPYW', 'df_j', size)).label('DF.bt'+tag).branch({1: 'DF.bzero'+tag}, 'DF.btest'+tag, [('CMPI', 'df_j', 0)])
        P('DF.btest'+tag).a(('ALUI', 'sub', 'df_j', 'df_j', 1), ('LDX', 'df_t', 'df_j', base)).branch({1: 'DF.bt'+tag}, 'DF.bnonzero'+tag, [('CMPI', 'df_t', 0)])
        P('DF.bzero'+tag).a(('LDI', 'df_bits', 0)).ret()
        P('DF.bnonzero'+tag).a(('ALUI', 'mul', 'df_bits', 'df_j', 32)).label('DF.bl'+tag).a(
            ('ALUI', 'add', 'df_bits', 'df_bits', 1), ('A64I', 'shr', 'df_t', 'df_t', 1)).branch({1: 'RET'}, 'DF.bl'+tag, [('CMPI', 'df_t', 0)])
    # A and D may have zero high limbs after subtraction: compare padded words.
    P('DF.cmp').a(('COPYW', 'df_j', 'df_n')).branch({0: 'DF.cmpdn'}, 'DF.cl', [('CMP', 'df_n', 'df_dn')])
    P('DF.cmpdn').a(('COPYW', 'df_j', 'df_dn')).goto('DF.cl')
    P('DF.cl').branch({1: 'DF.cequal'}, 'DF.cword', [('CMPI', 'df_j', 0)])
    P('DF.cword').a(('ALUI', 'sub', 'df_j', 'df_j', 1), ('LDI', 'df_a', 0), ('LDI', 'df_d', 0)).branch({0: 'DF.ca'}, 'DF.cbd', [('CMP', 'df_j', 'df_n')])
    P('DF.ca').a(('LDX', 'df_a', 'df_j', A)).goto('DF.cbd')
    P('DF.cbd').branch({0: 'DF.cd'}, 'DF.cc', [('CMP', 'df_j', 'df_dn')])
    P('DF.cd').a(('LDX', 'df_d', 'df_j', D)).goto('DF.cc')
    P('DF.cc').branch({1: 'DF.cl', 0: 'DF.cless'}, 'DF.cgreater', [('C64U', 'df_a', 'df_d')])
    for name, val in (('equal', 1), ('less', 0), ('greater', 2)):
        P('DF.c'+name).a(('LDI', 'df_cmp', val)).ret()
    P('DF.sub').a(('LDI', 'df_j', 0), ('LDI', 'df_borrow', 0)).label('DF.sl').branch({0: 'DF.sw'}, 'RET', [('CMP', 'df_j', 'df_n')])
    P('DF.sw').a(('LDX', 'df_a', 'df_j', A), ('LDI', 'df_d', 0)).branch({0: 'DF.sd'}, 'DF.sminus', [('CMP', 'df_j', 'df_dn')])
    P('DF.sd').a(('LDX', 'df_d', 'df_j', D)).goto('DF.sminus')
    P('DF.sminus').a(('A64', 'sub', 'df_a', 'df_a', 'df_borrow'), ('A64', 'sub', 'df_a', 'df_a', 'df_d'), ('LDI', 'df_borrow', 0)).branch({0: 'DF.sborrow'}, 'DF.sstore', [('C64', 'df_a', 'df_z')])
    P('DF.sborrow').a(('LDI', 'df_borrow', 1), ('A64I', 'add', 'df_a', 'df_a', 4294967296)).goto('DF.sstore')
    P('DF.sstore').a(('STX', 'df_j', A, 'df_a'), ('ALUI', 'add', 'df_j', 'df_j', 1)).goto('DF.sl')
    P('DF.convert').branch({1: bad}, 'DF.value', [('CMPI', 'df_seen', 0)])
    P('DF.value').a(('LDI', 'tk', E.TK_FNUM), ('LDI', 'nx', 1)).call('DF.bitsA').branch({1: 'DF.zero'}, 'DF.expbound', [('CMPI', 'df_bits', 0)])
    # For negative e, M*10^e < 2^(bits(M)+3e). Only discard a value
    # proven below half the smallest subnormal; a long significand can
    # cancel a large negative exponent (e.g. 10^1000 * 10^-1000).
    P('DF.expbound').branch({1: 'HF.setup'}, 'DF.decimalbound', [('CMPI', 'df_hex', 1)])
    P('HF.setup').a(('COPYW', 'df_offset', 'df_exp'), ('LDI', 'df_exp', 0)).goto('DF.setup')
    P('DF.decimalbound').branch({0: 'DF.under'}, 'DF.expmax', [('CMPI', 'df_exp', 0)])
    P('DF.under').a(('ALUI', 'mul', 'df_upper', 'df_exp', 3), ('ALU', 'add', 'df_upper', 'df_upper', 'df_bits'),
        ('ALU', 'sub', 'df_floor', 'df_emin', 'df_mbits'), ('ALUI', 'sub', 'df_floor', 'df_floor', 1)).branch(
        {(0, 1): 'DF.zero'}, 'DF.expmax', [('CMP', 'df_upper', 'df_floor')])
    P('DF.expmax').branch({2: 'DF.clamp'}, 'DF.setup', [('CMPI', 'df_exp', 400)])
    P('DF.clamp').a(('LDI', 'df_exp', 400)).goto('DF.setup')
    P('DF.setup').a(('LDI', 'df_dn', 1), ('LDI', 'df_one', 1), ('STX', 'df_z', D, 'df_one'), ('COPYW', 'df_power', 'df_exp')).branch({0: 'DF.negexp'}, 'DF.powerA', [('CMPI', 'df_power', 0)])
    P('DF.negexp').a(('ALU', 'sub', 'df_power', 'df_z', 'df_power')).goto('DF.powerD')
    for tag in ('A', 'D'):
        P('DF.power'+tag).branch({1: 'DF.normal'}, 'DF.pow'+tag, [('CMPI', 'df_power', 0)])
        P('DF.pow'+tag).a(('LDI', 'df_mul', 10), ('LDI', 'df_carry', 0)).call('DF.mul'+tag).a(('ALUI', 'sub', 'df_power', 'df_power', 1)).goto('DF.power'+tag)
    P('DF.normal').call('DF.bitsA').a(('COPYW', 'df_abits', 'df_bits')).call('DF.bitsD').a(('ALU', 'sub', 'df_e', 'df_abits', 'df_bits'), ('COPYW', 'df_shift', 'df_e')).branch({0: 'DF.negshift'}, 'DF.shiftD', [('CMPI', 'df_e', 0)])
    P('DF.negshift').a(('ALU', 'sub', 'df_shift', 'df_z', 'df_shift')).goto('DF.shiftA')
    for tag in ('A', 'D'):
        P('DF.shift'+tag).branch({1: 'DF.adjust'}, 'DF.sh'+tag, [('CMPI', 'df_shift', 0)])
        P('DF.sh'+tag).a(('LDI', 'df_mul', 2), ('LDI', 'df_carry', 0)).call('DF.mul'+tag).a(('ALUI', 'sub', 'df_shift', 'df_shift', 1)).goto('DF.shift'+tag)
    P('DF.adjust').call('DF.cmp').branch({1: 'DF.adjust1'}, 'DF.precision', [('CMPI', 'df_cmp', 0)])
    P('DF.adjust1').a(('LDI', 'df_mul', 2), ('LDI', 'df_carry', 0)).call('DF.mulA').a(('ALUI', 'sub', 'df_e', 'df_e', 1)).goto('DF.precision')
    P('DF.precision').a(('ALU', 'add', 'df_e', 'df_e', 'df_offset'), ('ALUI', 'add', 'df_prec', 'df_mbits', 1), ('ALU', 'sub', 'df_delta', 'df_e', 'df_emin')).branch({0: 'DF.subnormal'}, 'DF.quotient', [('CMPI', 'df_delta', 0)])
    P('DF.subnormal').a(('ALU', 'add', 'df_prec', 'df_prec', 'df_delta')).branch({0: 'DF.zero', 1: 'DF.halfunit'}, 'DF.quotient', [('CMPI', 'df_prec', 0)])
    P('DF.halfunit').call('DF.cmp').branch({1: 'DF.zero'}, 'DF.one', [('CMPI', 'df_cmp', 1)])
    P('DF.one').a(('LDI', 'nv', 1)).ret()
    P('DF.zero').a(('LDI', 'nv', 0)).ret()
    P('DF.quotient').a(('LDI', 'df_m', 0), ('COPYW', 'df_left', 'df_prec')).label('DF.ql').call('DF.cmp').a(('A64I', 'shl', 'df_m', 'df_m', 1)).branch({1: 'DF.qshift'}, 'DF.qbit', [('CMPI', 'df_cmp', 0)])
    P('DF.qbit').call('DF.sub').a(('A64I', 'or', 'df_m', 'df_m', 1)).goto('DF.qshift')
    P('DF.qshift').a(('LDI', 'df_mul', 2), ('LDI', 'df_carry', 0)).call('DF.mulA').a(('ALUI', 'sub', 'df_left', 'df_left', 1)).branch({1: 'DF.round'}, 'DF.ql', [('CMPI', 'df_left', 0)])
    P('DF.round').call('DF.cmp').branch({0: 'DF.pack', 2: 'DF.up'}, 'DF.even', [('CMPI', 'df_cmp', 1)])
    P('DF.even').branch({1: 'DF.pack'}, 'DF.up', [('A64I', 'and', 'df_t', 'df_m', 1), ('CMPI', 'df_t', 0)])
    P('DF.up').a(('A64I', 'add', 'df_m', 'df_m', 1)).goto('DF.pack')
    P('DF.pack').branch({0: 'DF.subpack'}, 'DF.carry', [('CMP', 'df_e', 'df_emin')])
    P('DF.subpack').a(('COPYW', 'nv', 'df_m')).ret()
    P('DF.carry').a(('ALUI', 'add', 'df_t', 'df_mbits', 1), ('A64', 'shr', 'df_t', 'df_m', 'df_t')).branch({1: 'DF.exponentbits'}, 'DF.carry1', [('CMPI', 'df_t', 0)])
    P('DF.carry1').a(('A64I', 'shr', 'df_m', 'df_m', 1), ('ALUI', 'add', 'df_e', 'df_e', 1)).goto('DF.exponentbits')
    P('DF.exponentbits').a(('ALU', 'add', 'df_biased', 'df_e', 'df_bias'), ('ALUI', 'mul', 'df_max', 'df_bias', 2), ('ALUI', 'add', 'df_max', 'df_max', 1)).branch({(1, 2): 'DF.infinity'}, 'DF.finite', [('CMP', 'df_biased', 'df_max')])
    P('DF.infinity').a(('A64', 'shl', 'nv', 'df_max', 'df_mbits')).ret()
    P('DF.finite').a(('LDI', 'df_t', 1), ('A64', 'shl', 'df_t', 'df_t', 'df_mbits'), ('A64I', 'sub', 'df_t', 'df_t', 1), ('A64', 'and', 'df_m', 'df_m', 'df_t'), ('A64', 'shl', 'nv', 'df_biased', 'df_mbits'), ('A64', 'or', 'nv', 'nv', 'df_m')).ret()
