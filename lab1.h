/*
 * file:        lab1.h
 * description: definitions for Lab 1
 */

#include <stdint.h>
#include <string.h>

struct cpu {
    uint8_t *memory;
    uint16_t R[8];
    uint16_t PC;
    uint16_t SP;
    int Z;
    int N;
};

/* execute one instruction, return 0 to continue and 1 to halt
 */
extern int emulate(struct cpu *cpu);
