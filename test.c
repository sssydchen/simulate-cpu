#include <stdio.h>
#include <assert.h>

#include "lab1.h"

extern void store2(struct cpu *cpu, uint16_t data, uint16_t addr);
extern uint16_t load2(struct cpu *cpu, uint16_t addr);

/* set all registers, flags, and memory to zero
 */
void zerocpu(struct cpu *cpu)
{
    cpu->Z = cpu->N = 0;
    cpu->PC = cpu->SP = 0;
    for (int i = 0; i < 8; i++)
        cpu->R[i] = 0;
    memset(cpu->memory, 0, 64 * 1024);
}

/* 0000 : 50f1      : ADD R3 + R6 -> R1
 */
void test1(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->R[3] = 5;
    cpu->R[6] = 10;
    store2(cpu, 0x5073, 0);

    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->R[1] == 15);
}

/*
0000 : 1001 0002 : SET R1 = 0x0002
0004 : 1004 0000 : SET R4 = 0x0000
*/
void test_set(struct cpu *cpu)
{
    zerocpu(cpu);

    store2(cpu, 0x1001, 0); // SET R1 = 2
    store2(cpu, 0x0002, 2); // Value 2
    store2(cpu, 0x1004, 4); // SET R4 = 0
    store2(cpu, 0x0000, 6); // Value 0

    int val1 = emulate(cpu);
    assert(val1 == 0);
    assert(cpu->R[1] == 2);

    int val2 = emulate(cpu);
    assert(val2 == 0);
    assert(cpu->R[4] == 0);
}

/**
 * 0000 : 2001 5678 : LOAD R1 <- *0x5678
 * 0004 : 2402 5678 : LOAD.B R2 <- *0x5678
 * 0008 : 282b      : LOAD R3 <- *R5
 * 000a : 2c2c      : LOAD.B R4 <- *R5
 */
void test_load(struct cpu *cpu)
{
    zerocpu(cpu);

    // Load a 16-bit word from address 0x5678 into R1
    store2(cpu, 0x2001, 0);
    store2(cpu, 0x5678, 2);
    store2(cpu, 0x1234, 0x5678);

    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->R[1] == 0x1234); // Check if R1 contains 0x1234

    // Load an 8-bit byte from address 0x5678 into R2
    store2(cpu, 0x2402, 4);
    store2(cpu, 0x5678, 6);
    cpu->memory[0x5678] = 0x12;
    val = emulate(cpu);
    assert(val == 0);
    assert(cpu->R[2] == 0x12);

    // Load a 16-bit word from the address stored in R5 into R3
    cpu->R[5] = 0x5678; // Set R5 to point to address 0x5678
    store2(cpu, 0x282B, 8);
    store2(cpu, 0x1234, 0x5678);

    val = emulate(cpu);
    assert(val == 0);
    assert(cpu->R[3] == 0x1234);

    // Load an 8-bit byte from the address stored in R5 into R4
    cpu->R[5] = 0x5678;
    store2(cpu, 0x2C2C, 10);
    cpu->memory[0x5678] = 0x34;

    val = emulate(cpu);
    assert(val == 0);
    assert(cpu->R[4] == 0x34);
}



void test_sub_negative(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->R[3] = 2;
    cpu->R[2] = 5;
    store2(cpu, 0x5253, 0);

    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->R[1] == 65533); // 65533 is two's compliment of -3
    assert(cpu->N == 1);
};

void test_sub_positive(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->R[3] = 6;
    cpu->R[2] = 4;
    store2(cpu, 0x5253, 0);

    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->R[1] == 2);
    assert(cpu->N == 0);
};

char memory[64 * 1024];
struct cpu cpu;

int main(int argc, char **argv)
{
    cpu.memory = memory;

    test1(&cpu);
    test_set(&cpu);
    test_load(&cpu);
    test_sub_negative(&cpu);
    test_sub_positive(&cpu);

    printf("all tests PASS\n");
}
