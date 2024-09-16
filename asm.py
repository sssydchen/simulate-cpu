#
# parser for hw1 assembler syntax
# split_at function takes previous character, this one, and returns
# whether to (a) split here and (b) save the character
#

# returns split,record

in_quote = False
def split_at(c1, c2):
    global in_quote

    if c1 == '':
        return False, True
    if in_quote:
        if c2 == '"':
            in_quote = False
            return False, False
        return False, True
    if c2 == '"':
        in_quote = True
        return False, False
    if c2.isspace():
        return not c1.isspace(), False
    if c2 in '^|&+,*:':                                   # singletons
        return not c1.isspace(), True
    if c2 == '-':
        if c1 == '<':
            return False, True
        return True, True
    if c1 == '-' and c2 != '>':
        return True, True
    if c1 in '+^|&,*:':
        return True, not c2.isspace()
    if c1 == '>':
        return c2 != '>', not c2.isspace()
    if c1 == '<':
        return c2 not in '<-', not c2.isspace()
    return False, True

def parse(line):
    words, word = [], ''
    c0 = ''

    # handle comments, leading / trailing spaces
    #
    i = line.find('#')
    if i > -1:
        line = line[:i]
    line = line.rstrip().lstrip() + ' '

    # split into words
    #
    for c1 in line:
        split,save = split_at(c0, c1)
        if split:
            if word:
                words.append(word)
            word = ''
        if save:
            word = word + c1
        c0 = c1
    return words

def is_num(s):
    if s[:2] == '0x':
        for c in s[2:]:
            if not c.upper() in '0123456789ABCDEF':
                return False
        return True
    return s.isnumeric()

symtab = dict()
current_addr = 0
max_addr = 0
memory = [0] * 65536 #

fixups = []

alu = ['ADD', 'SUB', 'AND', 'OR', 'XOR', 'LSHIFT', 'RSHIFT', 'CMP', 'TEST']
jmp = ['JMP', 'JMP_Z', 'JMP_NZ', 'JMP_LT', 'JMP_GT', 'JMP_LE','JMP_GE']
reg = ['R0', 'R1', 'R2', 'R3', 'R4', 'R5', 'R6', 'R7']
reg_sp = reg + ['SP']
reg = '<r>'
load = ['LOAD', 'LOAD.B']
store = ['STORE', 'STORE.B']

def regnum(r):
    if r == 'SP':
        return 8
    return int(r[1])

def value(n):
    if n.isidentifier():
        if n in symtab:
            return symtab[n]
        else:
            fixups.append([n, current_addr])
            return 0
    return int(n,base=0)

def store2(addr,val):
    memory[addr] = (val & 0xFF)
    memory[addr+1] = (val >> 8)

def store_insn(insn):
    global current_addr
    memory[current_addr] = (insn & 0xFF)
    memory[current_addr+1] = (insn >> 8)
    current_addr += 2
    
def do_set(line):                         # set rx = val
    store_insn(0x1000 | regnum(line[1]))
    store_insn(value(line[3]))

def do_load(line):                        # load rx <- addr
    op = line[0]
    byte = 0x0400 if (op[-2:] == '.B') else 0
    if '*' in line:
        line.remove('*')
    store_insn(0x2000 | byte | regnum(line[1]))
    store_insn(value(line[3]))

def do_load_rel(line):                    # load ra <- * rb
    op = line[0]
    byte = 0x0400 if (op[-2:] == '.B') else 0
    store_insn(0x2800 | byte | regnum(line[4]) << 3 | regnum(line[1]))

def do_store(line):                       # store rx -> addr
    op = line[0]
    byte = 0x0400 if (op[-2:] == '.B') else 0
    if '*' in line:
        line.remove('*')
    store_insn(0x3000 | byte | regnum(line[1]))
    store_insn(value(line[3]))

def do_store_rel(line):                   # store ra -> * rb
    op = line[0]
    byte = 0x0400 if (op[-2:] == '.B') else 0
    store_insn(0x3800 | byte | regnum(line[4]) << 3 | regnum(line[1]))

def do_move(line):                        # move rs -> rd
    store_insn(0x4000 | (regnum(line[3]) << 4) | regnum(line[1]))

alu_ops = {'ADD':0x0000, 'SUB':0x0200, 'AND':0x0400, 'OR':0x0600,
               'XOR':0x0800, 'RSHIFT':0x0A00}

def do_alu(line):                         # op ra + rb -> rc
    store_insn(0x5000 | alu_ops[line[0]] | (regnum(line[5]) << 6) |
                   (regnum(line[3]) << 3) | regnum(line[1]))

def do_cmp(line):                         # cmp ra - rb
    store_insn(0x5C00 | (regnum(line[3]) << 3) | regnum(line[1]))

def do_test(line):                        # test ra
    store_insn(0x5E00 | regnum(line[1]))

jmp_code = {'JMP':0x0000, 'JMP_Z':0x0200, 'JMP_NZ':0x0400, 'JMP_LT':0x0600,
                'JMP_GT':0x0800, 'JMP_LE':0x0A00, 'JMP_GE':0x0C00} #

def do_jmp(line):                         # jmp addr
    store_insn(0x6000 | jmp_code[line[0]])
    store_insn(value(line[1]))

def do_jmp_rel(line):                     # jmp * rx
    store_insn(0x7000 | jmp_code[line[0]] | regnum(line[2]))

def do_call(line):                        # call addr
    store_insn(0x8000)
    store_insn(value(line[1]))

def do_call_rel(line):                    # call * rx
    store_insn(0x9000 | regnum(line[2]))

misc_opcode = {'PUSH':0xB000, 'POP':0xC000, 'IN':0xD000, 'OUT':0xE000,
                   'RET':0xA000, 'HALT':0xF000}

def do_misc(line):
    if line[0] in ['RET', 'HALT']:
        store_insn(misc_opcode[line[0]])
    else:                                 # push/pop/in/out rx
        store_insn(misc_opcode[line[0]] | regnum(line[1]))

def do_sym(line):
    symtab[line[1]] = current_addr

def do_bytes(line):
    global current_addr
    for s in line[1:]:
        n = int(s,base=0) & 0xFF
        memory[current_addr] = n
        current_addr += 1

def do_words(line):
    global current_addr
    for s in line[1:]:
        n = int(s,base=0) & 0xFFFF
        memory[current_addr] = (n & 0xFF)
        memory[current_addr+1] = ((n >> 8) & 0xFF)
        current_addr += 2

def do_string(line):
    global current_addr
    s = line[1]
    s = s.replace('\\n','\n')
    for c in s:
        memory[current_addr] = ord(c)
        current_addr += 1
    memory[current_addr] = 0              # null terminator
    current_addr += 1

def do_align(line):
    global current_addr
    n = int(line[1],base=0)
    while (current_addr % n) != 0:
        current_addr += 1

def do_addr(line):
    global current_addr
    current_addr = int(line[1],base=0)

patterns = [
    [['SET', reg, '=', '<#>'], do_set],
    [['SET', reg, '=', '<label>'], do_set],
    [[load, reg, '<-', '<#>'], do_load],
    [[load, reg, '<-', '*', '<#>'], do_load],
    [[load, reg, '<-', '<label>'], do_load],
    [[load, reg, '<-', '*', reg], do_load_rel],
    [[load, reg, '<-', '*', '<label>'], do_load],   # must be after reg
    [[store, reg, '->', '<#>'], do_store],
    [[store, reg, '->', '*', '<#>'], do_store],
    [[store, reg, '->', '<label>'], do_store],
    [[store, reg, '->', '*', reg], do_store_rel],
    [[store, reg, '->', '*', '<label>'], do_store],   # must be after reg
    [['MOVE', reg_sp, '->', reg_sp], do_move],
    [['ADD', reg, '+', reg, '->', reg], do_alu],
    [['SUB', reg, '-', reg, '->', reg], do_alu],
    [['AND', reg, '&', reg, '->', reg], do_alu],
    [['OR', reg, '|', reg, '->', reg], do_alu],
    [['XOR', reg, '^', reg, '->', reg], do_alu],
    [['RSHIFT', reg, '>>', reg, '->', reg], do_alu],
    [['CMP', reg, '-', reg], do_cmp],
    [['TEST', reg], do_test],
    [[jmp, '<#>'], do_jmp],
    [[jmp, '<label>'], do_jmp],
    [[jmp, '*', reg], do_jmp_rel],
    [['CALL', '<#>'], do_call],
    [['CALL', '<label>'], do_call],
    [['CALL', '*', reg], do_call_rel],
    [['RET'], do_misc],
    [['HALT'], do_misc],
    [['PUSH', reg], do_misc],
    [['POP', reg], do_misc],
    [['IN', reg], do_misc],
    [['OUT', reg], do_misc],
    [['SYMBOL', '<label>'], do_sym],
    [['BYTES', '<nums>'], do_bytes],
    [['WORDS', '<nums>'], do_words],
    [['STRING', '<*>'], do_string],
    [['ALIGN', '<#>'], do_align],
    [['ADDR', '<#>'], do_addr]
    ]

def match(token, pattern):
    if type(pattern) == list:
        return token in pattern
    if pattern == '<#>':
        return is_num(token)
    if pattern == '<nums>':
        _words = token.replace(',', '')
        for t in _words.split():
            if not is_num(t):
                return False
        return True
    if pattern == '<*>':                # Any
        return True
    if pattern == '<label>':
        return token.isidentifier()
    if pattern == '<r>':
        return token[0] == 'R' and token[1] in '01234567'
    return token == pattern


def match_line(line, pattern):
    if len(line) != len(pattern):
        return False
    for l,p in zip(line,pattern):
        if not match(l,p):
            return False
    return True
        
def do_line(l):
    if len(l) == 0:
        return
    for p in patterns:
        if match_line(l,p[0]):
            p[1](l)
            break

if __name__ == "__main__":
    import sys
    infile = sys.argv[1]
    outfile = infile.replace('.asm', '.bin')
    
    fp = open(sys.argv[1], 'r')
    for line in fp:
        l = parse(line)
        if len(l) > 1 and l[1] == ':':
            do_line(['SYMBOL', l[0]])
            do_line(l[2:])
        else:
            do_line(l)
        max_addr = max(current_addr, max_addr)
    for sym,loc in fixups:
        store2(loc, symtab[sym])
    
    print('bytes:', current_addr)
    data = bytearray(max_addr)
    for i in range(max_addr):
        data[i] = memory[i]
    fp = open(outfile, 'wb')
    fp.write(data)
    fp.close()

    for i in range(0, max_addr, 2):
        print('%02X%02X' % (memory[i+1], memory[i]))
    
    
