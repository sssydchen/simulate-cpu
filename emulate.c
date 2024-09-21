/*
 * file:        emulate-soln.c
 * description: solution to Lab 1
 * random
 */

#include <stdio.h>
#include <stdlib.h>

#include "lab1.h"
#include <errno.h>

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
    int is_indirect = (insn & 0x0800) != 0; // I bit (check bit 11; 0000 1000 0000 0000)
    int is_byte = (insn & 0x0400) != 0;     // B bit (check bit 10; 0000 0100 0000 0000)

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

void store(struct cpu *cpu, uint16_t insn)
{
    int reg_a = insn & 0x7;
    int is_indirect = (insn & 0x0800) != 0;
    int is_byte = (insn & 0x0400) != 0;

    if (is_indirect)
    {
        int reg_b = (insn >> 3) & 0x7;
        uint16_t addr = cpu->R[reg_b];
        if (is_byte)
        {
            uint8_t value = cpu->R[reg_a] & 0xFF;
            cpu->memory[addr] = value;
        }
        else
        {
            uint16_t value = cpu->R[reg_a];
            store2(cpu, value, addr);
        }
        cpu->PC += 2;
    }
    else
    {
        uint16_t addr = load2(cpu, cpu->PC + 2);
        if (is_byte)
        {
            uint8_t value = cpu->R[reg_a] & 0xFF;
            cpu->memory[addr] = value;
        }
        else
        {

            uint16_t value = cpu->R[reg_a];
            store2(cpu, value, addr);
        }
        cpu->PC += 4;
    }
}

void move(struct cpu *cpu, uint16_t insn)
{
    int reg_d = (insn >> 4) & 0xF;
    int reg_s = insn & 0xF;

    if (reg_s > 8 || reg_d > 8)
    {
        printf("Invalid register\n");
        return;
    }

    if (reg_s == 8)
    {
        cpu->R[reg_d] = cpu->SP;
    }
    else if (reg_d == 8)
    {
        cpu->SP = cpu->R[reg_s];
    }
    else
    {
        cpu->R[reg_d] = cpu->R[reg_s];
    }

    cpu->PC += 2;
}

void alu(struct cpu *cpu, uint16_t insn)
{
    int op = (insn >> 9) & 0x7;    // Extract bits 9-11 for operation code (ooo)
    int reg_a = insn & 0x7;        // Extract bits 0-2 for Ra
    int reg_b = (insn >> 3) & 0x7; // Extract bits 3-5 for Rb
    int reg_c = (insn >> 6) & 0x7; // Extract bits 6-8 for Rc

    uint16_t result = 0;

    if ((insn & 0x0E00) == 0x0000) // 000: ADD Ra + Rb -> Rc
    {
        result = cpu->R[reg_a] + cpu->R[reg_b];
        cpu->R[reg_c] = result;
    }
    else if ((insn & 0x0E00) == 0x0200) // 001: SUB Ra - Rb -> Rc
    {
        result = cpu->R[reg_a] - cpu->R[reg_b];
        cpu->R[reg_c] = result;
    }
    else if ((insn & 0x0E00) == 0x0400) // 010: AND Ra & Rb -> Rc
    {
        result = cpu->R[reg_a] & cpu->R[reg_b];
        cpu->R[reg_c] = result;
    }
    else if ((insn & 0x0E00) == 0x0600) // 011: OR Ra | Rb -> Rb
    {
        result = cpu->R[reg_a] | cpu->R[reg_b];
        cpu->R[reg_b] = result;
    }
    else if ((insn & 0x0E00) == 0x0800) // 100: XOR Ra ^ Rb -> Rc
    {
        result = cpu->R[reg_a] ^ cpu->R[reg_b];
        cpu->R[reg_c] = result;
    }
    else if ((insn & 0x0E00) == 0x0A00) // 101: SHIFTR Ra >> Rb -> Rb
    {
        result = cpu->R[reg_a] >> cpu->R[reg_b];
        cpu->R[reg_b] = result;
    }
    else if ((insn & 0x0E00) == 0x0C00) // 110: CMP Ra – Rb (“compare”: compute Ra - Rb, discard the result but set N and Z
    {
        result = cpu->R[reg_a] - cpu->R[reg_b];
        cpu->Z = (result == 0);
        cpu->N = (result & 0x8000) != 0;
        return;
    }
    else if ((insn & 0x0E00) == 0x0E00) // 111: TEST Ra (set Z, N according to Ra)
    {
        result = cpu->R[reg_a];
        cpu->Z = (result == 0);
        cpu->N = (result & 0x8000) != 0;
        return;
    }

    // Set the flags for non-CMP/TEST operations
    cpu->Z = (result == 0);          // If result is zero, set Z to 1
    cpu->N = (result & 0x8000) != 0; // If result is positive, set N to 0; if negative, set N to 1
    cpu->PC += 2;                    // Reset program counter
}

// void jmp(struct cpu *cpu, uint16_t insn)
// {
    
//     int condition = (insn >> 8) & 0x7;
//     int reg_a = insn & 0x7; 
//     int is_direct = (insn & 0x0800) == 0;

//     int jump = 0;
//     if (condition == 0x0) {        
//         jump = 1;
//     } else if (condition == 0x1) {  
//         jump = (cpu->Z == 1);
//     } else if (condition == 0x2) {  
//         jump = (cpu->Z == 0);
//     } else if (condition == 0x3) {   
//         jump = (cpu->N == 1);
//     } else if (condition == 0x4) {   
//         jump = (cpu->N == 0 && cpu->Z == 0);
//     } else if (condition == 0x5) {   
//         jump = (cpu->N == 1 || cpu->Z == 1);
//     } else if (condition == 0x6) {   
//         jump = (cpu->N == 0);
//     } else {                        
//         printf("Illegal JMP condition\n");
//         return;
//     }

//     if (jump) {
//         if (is_direct) {
//             uint16_t address = load2(cpu, cpu->PC + 2); // Jump to the constant address (specified in bytes 3 and 4)
//             cpu->PC = address; 
//         } else {
//             cpu->PC = cpu->R[reg_a]; // Indirect jump: Jump to the address in register Ra
//         }
//     } else { // If condition is false, just increment the PC
//         if (is_direct) {
//             cpu->PC += 4; 
//         } else {
//             cpu->PC += 2; 
//         }
//     }
// }

void jmp_to_explicit_addr(struct cpu *cpu, uint16_t insn)
{
    int cond = (insn >> 9) & 0x7;
    int is_true = 0;
    uint16_t target_address = load2(cpu, cpu->PC + 2);

    switch (cond)
    {
    case 0x0: // JMP(unconditional)
        cpu->PC = target_address;
        // printf("CPU->PC 0x%04X\n", cpu->PC);
        is_true = 1;
        break;
    case 0x1: // JMP_Z
        if (cpu->Z == 1)
        {
            cpu->PC = load2(cpu, cpu->PC + 2);
            is_true = 1;
        }
        break;
    case 0x2: // JMP_NZ
        if (cpu->Z == 0)
        {
            cpu->PC = load2(cpu, cpu->PC + 2);
            is_true = 1;
        }
        break;
    case 0x3: // JMP_LT
        if (cpu->N == 1)
        {
            cpu->PC = load2(cpu, cpu->PC + 2);
            is_true = 1;
        }
        break;
    case 0x4: // JMP_GT
        if (cpu->N == 0 && cpu->Z == 0)
        {
            cpu->PC = load2(cpu, cpu->PC + 2);
            is_true = 1;
        }
        break;
    case 0x5: // JMP_LE
        if (cpu->N == 1 || cpu->Z == 1)
        {
            cpu->PC = load2(cpu, cpu->PC + 2);
            is_true = 1;
        }
        break;
    case 0x6: // JMP_GE
        if (cpu->N == 0)
        {
            cpu->PC = load2(cpu, cpu->PC + 2);
            is_true = 1;
        }
        break;
    default:
        errno = EOPNOTSUPP; // operation is not supported.
        exit(EXIT_FAILURE);
        break;
    }
    if (is_true == 0)
    {
        cpu->PC = cpu->PC + 4;
    }
}

void jmp_to_register_addr(struct cpu *cpu, uint16_t insn)
{
    int cond = (insn >> 9) & 0x7;
    int a = insn & 0x7;
    int is_true = 0;

    switch (cond)
    {
    case 0x0: // JMP(unconditional)
        cpu->PC = cpu->R[a];
        is_true = 1;
        break;
    case 0x1: // JMP_Z
        if (cpu->Z == 1)
        {
            cpu->PC = cpu->R[a];
            is_true = 1;
        }
        break;
    case 0x2: // JMP_NZ
        if (cpu->Z == 0)
        {
            cpu->PC = cpu->R[a];
            is_true = 1;
        }
        break;
    case 0x3: // JMP_LT
        if (cpu->N == 1)
        {
            cpu->PC = cpu->R[a];
            is_true = 1;
        }
        break;
    case 0x4: // JMP_GT
        if (cpu->N == 0 && cpu->Z == 0)
        {
            cpu->PC = cpu->R[a];
            is_true = 1;
        }
        break;
    case 0x5: // JMP_LE
        if (cpu->N == 1 || cpu->Z == 1)
        {
            cpu->PC = cpu->R[a];
            is_true = 1;
        }
        break;
    case 0x6: // JMP_GE
        if (cpu->N == 0)
        {
            cpu->PC = cpu->R[a];
            is_true = 1;
        }
        break;
    default:
        errno = EOPNOTSUPP; // operation is not supported.
        exit(EXIT_FAILURE);
        break;
    }
    if (is_true == 0)
    {
        cpu->PC = cpu->PC + 2;
    }
}

void call_specified_address(struct cpu *cpu, uint16_t insn)
{
    cpu->SP = cpu->SP - 2;
    store2(cpu, cpu->PC + 4, cpu->SP);
    cpu->PC = load2(cpu, cpu->PC + 2);
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
    cpu->PC = cpu->PC + 2;
}

void pop(struct cpu *cpu, uint16_t insn)
{
    uint8_t a = insn & 0x7;
    cpu->R[a] = load2(cpu, cpu->SP);
    cpu->SP = cpu->SP + 2;
    cpu->PC = cpu->PC + 2;
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

// void move(struct cpu *cpu, uint16_t insn)
// {
//     int s = insn & 0x0008;
//     int d = (insn >> 4) & 0x0008;
//     uint8_t *copyFrom = NULL;
//     uint8_t *copyInto = NULL;

//     if (s == 8)
//     {
//         copyFrom = &(cpu->SP);
//     }
//     else
//     {
//         copyFrom = &(cpu->R[s]);
//     }

//     if (d == 8)
//     {
//         copyInto = &(cpu->SP);
//     }
//     else
//     {
//         copyInto = &(cpu->R[d]);
//     }

//     store2(cpu, load2(cpu, copyFrom), copyInto);
// }

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
        return 0;
    }
    else if ((insn & 0xF000) == 0x6000)
    {
        jmp_to_explicit_addr(cpu, insn);
        return 0;
    }
    else if ((insn & 0xF000) == 0x7000)
    {
        jmp_to_register_addr(cpu, insn);
        return 0;
    }
    // else if ((insn & 0xF000) == 0x6000 || (insn & 0xF000) == 0x7000)
    // {
    //     jmp(cpu, insn);
    //     return 0;
    // }
    else if ((insn & 0xF000) == 0x8000)
    {
        /* CALL(Specified Address) */
        call_specified_address(cpu, insn);
        return 0;
    }
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
