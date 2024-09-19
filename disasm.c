/*
 * disassembler
 *
 * Assumes that the last instruction in an executable is HALT,
 * and that any bytes following that are data.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

char *rnames[] = {"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "SP"};
static char *reg_str(int r)
{
    if (r >= 0 && r < 9)
        return rnames[r];
    return "*BAD*";
}

static char *jmp_suff[] = {"","_Z", "_NZ", "_LT", "_GT",
                           "_LE", "_GE", "_*ILLEGAL*"};


// returns number of bytes consumed
int disasm(uint16_t insn, uint16_t next, char *buf)
{
    int ra = insn & 7;
    int rb = (insn & 070) >> 3;
    int rc = (insn & 0700) >> 6;
    
    int indir = insn & 0x0800;
    int byte = insn & 0x0400;
    char *tmp = "";

    int op = insn & 0x0E00;
    int ccc = (insn & 0x0E00) >> 9;
    
    switch (insn & 0xF000) {
    case 0x1000:            /* SET */
        sprintf(buf, "SET R%d = 0x%04x", ra, next);
        return 4;
        break;
    case 0x2000:            /* LOAD */
        if (byte)
            tmp = ".B";
        if (indir) 
            sprintf(buf, "LOAD%s R%d <- *R%d", tmp, ra, rb);
        else {
            sprintf(buf, "LOAD%s R%d <- *0x%04x", tmp, ra, next);
            return 4;
        }
        break;
    case 0x3000:            /* STORE */
        if (byte)
            tmp = ".B";
        if (indir) 
            sprintf(buf, "STORE%s R%d -> *R%d", tmp, ra, rb);
        else {
            sprintf(buf, "STORE%s R%d -> *0x%04x", tmp, ra, next);
            return 4;
        }
        break;
    case 0x4000:            /* MOVE */
        ra = insn & 0xF;
        rb = (insn & 0xF0) >> 4;
        sprintf(buf, "MOV %s -> %s", reg_str(ra), reg_str(rb));
        break;
    case 0x5000:
        switch (op) {
        case 0x0000:        /* ADD */
            sprintf(buf, "ADD R%d + R%d -> R%d", ra, rb, rc);
            break;
        case 0x0200:        /* SUB */
            sprintf(buf, "SUB R%d - R%d -> R%d", ra, rb, rc);
            break;
        case 0x0400:        /* AND */
            sprintf(buf, "AND R%d & R%d -> R%d", ra, rb, rc);
            break;
        case 0x0600:        /* OR */
            sprintf(buf, "OR R%d | R%d -> R%d", ra, rb, rc);
            break;
        case 0x0800:        /* XOR */
            sprintf(buf, "XOR R%d ^ R%d -> R%d", ra, rb, rc);
            break;
        case 0x0A00:        /* SHIFT RIGHT */
            sprintf(buf, "RSHIFT R%d >> R%d -> R%d", ra, rb, rc);
            break;
        case 0x0C00:        /* CMP */
            sprintf(buf, "CMP R%d - R%d", ra, rb);
            break;
        case 0x0E00:        /* TEST */
            sprintf(buf, "TEST R%d", ra);
            break;
        }
        break;
    case 0x6000:            /* JMP absolute */
        sprintf(buf, "JMP%s 0x%04x", jmp_suff[ccc], next);
        return 4;
        break;
    case 0x7000:            /* JMP register indirect */
        sprintf(buf, "JMP%s *R%d", jmp_suff[ccc], ra);
        break;
    case 0x8000:            /* CALL absolute */
        sprintf(buf, "CALL 0x%04x", next);
        return 4;
        break;
    case 0x9000:            /* CALL register indirect */
        sprintf(buf, "CALL *R%d", ra);
        break;
    case 0xA000:            /* RET */
        sprintf(buf, "RET");
        break;
    case 0xB000:            /* PUSH */
        sprintf(buf, "PUSH R%d", ra);
        break;
    case 0xC000:            /* POP */
        sprintf(buf, "POP R%d", ra);
        break;
    case 0xD000:            /* IN */
        sprintf(buf, "IN R%d", ra);
        break;
    case 0xE000:            /* OUT */
        sprintf(buf, "OUT R%d", ra);
        break;
    case 0xF000:            /* HALT */
        sprintf(buf, "HALT");
        break;
    }
    return 2;
}

static uint16_t load2(unsigned char *mem, uint16_t addr) {
    return mem[addr] | (mem[addr+1] << 8);
}

#ifdef STANDALONE
unsigned char _mem[64*1024];
int main(int argc, char **argv)
{
    FILE *fp = fopen(argv[1], "rb");
    if (!fp)
        perror("open"), exit(1);
    
    int len = fread(_mem, 1, sizeof(_mem), fp);

    int offset = 0;
    while (offset < len) {
        char buf[64], word2[16];
        uint16_t insn = load2(_mem, offset);
        uint16_t next = load2(_mem, offset+2);
        int _len = disasm(insn, next, buf);
        if (_len == 4) 
            sprintf(word2, "%04x", next);
        else
            sprintf(word2, "    ");
        printf("%04x : %04x %s : %s\n", offset, insn, word2, buf);
        offset += _len;
        if (insn == 0xF000)     /* HALT */
            break;
    }
    while (offset < len) {
        printf("%04x :", offset);
        int _offset = offset;
        for (int i = 0; i < 16 && len - offset; i++) {
            printf(" %02x", _mem[offset]);
            offset++;
        }
        printf("\t");
        for (int i = 0; i < offset - _offset; i++) {
            char b = _mem[_offset + i];
            if (isprint(b))
                printf(" %c", b);
            else
                printf(" ?");
        }
        printf("\n");
    }
}

#endif
