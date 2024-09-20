/*
 * file:        emulate-soln.c
 * description: solution to Lab 1
 * random
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
            uint8_t value = cpu->memory[addr];
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
            uint8_t value = cpu->memory[addr];
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

void alu(struct cpu *cpu, uint16_t insn)
{
    int op = (insn >> 9) & 0x7;    // Extract bits 9-11 for operation code (ooo)
    int reg_a = insn & 0x7;        // Extract bits 0-2 for Ra
    int reg_b = (insn >> 3) & 0x7; // Extract bits 3-5 for Rb
    int reg_c = (insn >> 6) & 0x7; // Extract bits 6-8 for Rc

    uint16_t result = 0;

    switch (op)
    {
    case 0x0: // ADD
        // printf("ADD: R%d + R%d -> R%d\n", reg_a, reg_b, reg_c);
        // printf("Values before: R%d = %d, R%d = %d\n", reg_a, cpu->R[reg_a], reg_b, cpu->R[reg_b]);
        result = cpu->R[reg_a] + cpu->R[reg_b];
        cpu->R[reg_c] = result;
        // printf("Result: R%d = %d\n", reg_c, cpu->R[reg_c]);
        break;
    case 0x1: // SUB
        result = cpu->R[reg_a] - cpu->R[reg_b];
        cpu->R[reg_c] = result;
        break;
    case 0x2: // AND
        result = cpu->R[reg_a] & cpu->R[reg_b];
        cpu->R[reg_c] = result;
        break;
    case 0x3: // OR
        result = cpu->R[reg_a] | cpu->R[reg_b];
        cpu->R[reg_c] = result;
        break;
    case 0x4: // XOR
        result = cpu->R[reg_a] ^ cpu->R[reg_b];
        cpu->R[reg_c] = result;
        break;
    case 0x5: // SHIFTR
        result = cpu->R[reg_a] >> cpu->R[reg_b];
        cpu->R[reg_a] = result;
        break;
    case 0x6: // CMP
        result = cpu->R[reg_a] - cpu->R[reg_b];
        cpu->Z = (result == 0);          // Set Zero flag
        cpu->N = (result & 0x8000) != 0; // Set Negative flag
        return;
    case 0x7: // TEST
        result = cpu->R[reg_a];
        cpu->Z = (result == 0);          // Set Zero flag
        cpu->N = (result & 0x8000) != 0; // Set Negative flag
        return;
    }

    // Set the flags for non-CMP/TEST operations
    cpu->Z = (result == 0);          // Set Zero flag if result is zero
    cpu->N = (result & 0x8000) != 0; // Set Negative flag if the high bit is set
    cpu->PC += 2;
}

void call_register_indirect(struct cpu *cpu, uint16_t insn)
{
    uint8_t a = insn & 0x0007;         // get the register number from the last 3 bits.
    cpu->SP = cpu->SP - 2;             // decrement SP by 2.
    store2(cpu, cpu->PC + 2, cpu->SP); // store the next instruction in the address stored in SP.
    cpu->PC = cpu->R[a];
}

void ret(struct cpu *cpu)
{
    cpu->PC = load2(cpu, cpu->SP);
    cpu->SP = cpu->SP + 2;
}

void push(struct cpu *cpu, uint16_t insn)
{
    uint8_t a = insn & 0x7;
    cpu->SP = cpu->SP - 2;
    store2(cpu, cpu->R[a], cpu->SP);
}

void pop(struct cpu *cpu, uint16_t insn)
{
    uint8_t a = insn & 0x7;
    cpu->R[a] = load2(cpu, cpu->SP);
    cpu->SP = cpu->SP + 2;
}

void in(struct cpu *cpu, uint16_t insn)
{
    uint8_t a = insn & 0x7;
    cpu->R[a] = fgetc(stdin);
    cpu->PC = cpu->PC + 2;
}

void out(struct cpu *cpu, uint16_t insn)
{
    uint8_t a = insn & 0x7;
    fputc(cpu->R[a], stdout);
    cpu->PC = cpu->PC + 2;
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
        return 0;
    }
    else if ((insn & 0xF000) == 0x2000)
    {
        load(cpu, insn);
        return 0;
    }
    // else if ((insn & 0xF000) == 0x3000)
    // {
    //     store(cpu, insn);
    // }
    // else if ((insn & 0xF000) == 0x4000)
    // {
    //     move(cpu, insn);
    // }
    else if ((insn & 0xF000) == 0x5000)
    {
        alu(cpu, insn);
        return 0;
    }
    // else if ((insn & 0xF000) == 0x6000 || (insn & 0xF000) == 0x7000)
    // {
    //     jmp(cpu, insn);
    // }

    else if ((insn & 0xF000) == 0x9000)
    {
        /* CALL(Register Indirect) */
        call_register_indirect(cpu, insn);
        return 0;
    }
    else if ((insn & 0xF000) == 0xA000)
    {
        /* RET */
        ret(cpu);
        return 0;
    }
    else if ((insn & 0xF000) == 0xB000)
    {
        /* PUSH */
        push(cpu, insn);
        return 0;
    }
    else if ((insn & 0xF000) == 0xC000)
    {
        pop(cpu, insn);
        return 0;
    }
    else if ((insn & 0xF000) == 0xD000)
    {
        /* IN */
        in(cpu, insn);
        return 0;
    }
    else if ((insn & 0xF000) == 0xE000)
    {
        /* OUT */
        out(cpu, insn);
        return 0;
    }
    else
    {
        return 1; // HALT
    }
}
