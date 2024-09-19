/*
 * file:        emulate-soln.c
 * description: solution to Lab 1
 */

#include <stdio.h>
#include <stdlib.h>

#include "lab1.h"

void store2(struct cpu *cpu, uint16_t data, uint16_t addr)
{
    cpu->memory[addr] = data & 0xFF;
    cpu->memory[addr + 1] = (data >> 8) & 0xFF;
}

uint16_t load2(struct cpu *cpu, uint16_t addr)
{
    return (cpu->memory[addr] | (cpu->memory[addr + 1] << 8));
}

void set(struct cpu *cpu, uint16_t insn)
{
    int reg = insn & 0x7;                     // Perform bitwise AND to keep the last 3 bits of insn (which represents the register)
    uint16_t value = load2(cpu, cpu->PC + 2); // Read value from memory addresses PC+2 and PC+3
    cpu->R[reg] = value;                      // Put result into register
    cpu->PC += 4;                             // Increment PC by 4 bytes - after executing SET (4 bytes long), the program counter should be incremented by 4 to point to the next instruction
}

void load(struct cpu *cpu, uint16_t insn)
{
    int reg_a = insn & 0x7;
    int is_indirect = (insn & 0x0800) != 0; // I bit (bit 11)
    int is_byte = (insn & 0x0400) != 0;     // B bit (bit 10)

    if (is_indirect)
    {
        // Indirect: Load from the address in register Rb
        int reg_b = (insn >> 3) & 0x7; // Extract bits 5-3 for register Rb
        uint16_t addr = cpu->R[reg_b]; // Get the memory address from register Rb

        if (is_byte)
        {
            // Load an 8-bit byte from memory and store it in Ra (low 8 bits of Ra)
            uint8_t value = cpu->ram[addr];
            cpu->R[reg_a] = value; // Store the byte (zero-extended) in Ra
        }
        else
        {
            // Load a 16-bit word from memory and store it in Ra
            uint16_t value = load2(cpu, addr);
            cpu->R[reg_a] = value; // Store the word in Ra
        }

        // Indirect mode: Increment PC by 2 (instruction size is 2 bytes)
        cpu->PC += 2;
    }
    else
    {
        // Direct: Load from a constant address
        uint16_t addr = load2(cpu, cpu->PC + 2); // The address comes from the next 2 bytes

        if (is_byte)
        {
            // Load an 8-bit byte from memory and store it in Ra
            uint8_t value = cpu->ram[addr];
            cpu->R[reg_a] = value; // Store the byte (zero-extended) in Ra
        }
        else
        {
            // Load a 16-bit word from memory and store it in Ra
            uint16_t value = load2(cpu, addr);
            cpu->R[reg_a] = value; // Store the word in Ra
        }

        cpu->PC += 4;
    }
}

/* emulate(struct cpu*) - emulate a single instruction
 *     - returns 1 for halt, 0 for no halt
 */
int emulate(struct cpu *cpu)
{
    uint16_t insn = load2(cpu, cpu->PC);

    /* your code here */
    if ((insn & 0xF000) == 0x1000)
    {
        set(cpu, insn);
    }
    else if ((insn & 0xF000) == 0x2000)
    {
        load(cpu, insn);
    }
    else if ((insn & 0xF000) == 0x3000)
    {
        store(cpu, insn);
    }
    else if ((insn & 0xF000) == 0x4000)
    {
        move(cpu, insn);
    }
    else if ((insn & 0xF000) == 0x5000)
    {
        alu(cpu, insn);
    }
    else if ((insn & 0xF000) == 0x6000 || (insn & 0xF000) == 0x7000)
    {
        jmp(cpu, insn);
    }
    else if ((insn & 0xF000) == 0xF000)
    {
        return 1;
    }

    return 0; // Continue emulation
}
