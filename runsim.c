/*
 * file:        runsim.c
 * description: simulator scaffolding for CS 5600 Lab 1
 *
 * Peter Desnoyers, Fall 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "lab1.h"


uint8_t _memory[64*1024];
uint16_t R[8];
uint16_t PC = 0, SP = 0xFFFE;
int Z, N;

struct cpu _cpu = {
    .memory = _memory,
    .R = {0},
    .PC = 0,
    .SP = 0xFFFE,
    .Z = 0,
    .N = 0
};

extern int disasm(uint16_t insn, uint16_t next, char *buf);
static uint16_t _load2(struct cpu *cpu, uint16_t addr) {
    return (cpu->memory[addr] | (cpu->memory[addr+1] << 8));
}

int main(int argc, char **argv)
{
    struct cpu *cpu = &_cpu;
    int verbose = 0;
    int show_regs = 0;

    int arg = 1;
    while (argv[arg][0] == '-') {
        if (!strcmp(argv[arg], "-v")) {
            verbose = 1;
            arg++;
        }
        if (!strcmp(argv[arg], "-r")) {
            show_regs = 1;
            arg++;
        }
    }
    //verbose = 1;
    
    FILE *fp = fopen(argv[arg], "rb");
    fread(cpu->memory, 1<<16, 1, fp);
    fclose(fp);

    
    while (1) {
        uint16_t old_pc = _load2(cpu, cpu->PC);
        
        if (verbose) {
            char buf[64], word2[16];
            uint16_t insn = _load2(cpu, cpu->PC);
            uint16_t next = _load2(cpu, cpu->PC+2);
            int _len = disasm(insn, next, buf);
            if (_len == 4) 
                sprintf(word2, "%04x", next);
            else
                sprintf(word2, "    ");
            printf("%04x : %04x %s : %s\n", cpu->PC, insn, word2, buf);
        }

        if (emulate(cpu))
            break;
        
        if (show_regs) {
            printf("  SP=%04x  N=%d Z=%d\n", cpu->SP, cpu->N, cpu->Z);
            printf("  R0=%04x R1=%04x R2=%04x R3=%04x\n", cpu->R[0], cpu->R[1], cpu->R[2], cpu->R[3]);
            printf("  R4=%04x R5=%04x R6=%04x R7=%04x\n", cpu->R[4], cpu->R[5], cpu->R[6], cpu->R[7]);
        }

        if (old_pc == cpu->PC) {
            printf("ERROR: PC did not change: %04x\n", cpu->PC);
            exit(1);
        }
    }
    printf("halted at PC=%04x\n", cpu->PC);
    exit(0);
}
