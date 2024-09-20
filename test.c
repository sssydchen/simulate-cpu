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

    store2(cpu, 0x1001, 0); 
    store2(cpu, 0x0002, 2); 
    store2(cpu, 0x1004, 4); 
    store2(cpu, 0x0000, 6); 

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

/**
 * 0000 : 5253      : SUB R3 - R2 -> R1
 */
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

/**
 * 0000 : 5253      : SUB R3 - R2 -> R1
 */
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

/**
 * 0000 : 56da      : OR R2 | R3 -> R3
 * 0002 : 57fd      : OR R5 | R7 -> R7
 */
void test_or(struct cpu *cpu) {
    zerocpu(cpu);

    cpu->R[2] = 0x1712; // 0001 0111 0001 0010
    cpu->R[3] = 0xF4D6; // 1111 0100 1101 0110
    store2(cpu, 0x56DA, 0);  
    int val1 = emulate(cpu);
    assert(val1 == 0);
    assert(cpu->R[3] == 0xF7D6); // 1111 0111 1101 0110

    cpu->R[5] = 0x28AF; // 0010 1000 1010 1111
    cpu->R[7] = 0x5036; // 0101 0000 0011 0110 
    store2(cpu, 0x57FD, 2); // Instruction is 2 byte long
    int val2 = emulate(cpu);
    assert(val2 == 0);
    assert(cpu->R[7] == 0x78BF); // 0111 1000 1011 1111
}

/**
 * 0000 : 58d1      : XOR R1 ^ R2 -> R3
 * 0002 : 592c      : XOR R4 ^ R5 -> R4
 */
void test_xor(struct cpu *cpu) {
    zerocpu(cpu);
    cpu->R[1] = 0x7612; // 0111 0110 0001 0010 
    cpu->R[2] = 0x2189; // 0010 0001 1000 1001 
    store2(cpu, 0x58d1, 0);  

    int val1 = emulate(cpu);
    assert(val1 == 0);
    assert(cpu->R[3] == 0x579B);  // 0101 0111 1001 1011

    cpu->R[4] = 0x7612; 
    cpu->R[5] = 0x2189; 
    store2(cpu, 0x592c, 2);  

    int val2 = emulate(cpu);
    assert(val2 == 0);
    assert(cpu->R[4] == 0x579B);  
}

/**
 * 0000 : 5b22      : RSHIFT R2 >> R4 -> R4
 */
void test_rshift(struct cpu *cpu) {
    zerocpu(cpu);
    cpu->R[2] = 0x0103; // 0000 0001 0000 0011 
    cpu->R[4] = 0x2; 
    store2(cpu, 0x5B22, 0); 

    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->R[4] == 0x40);  // 0000 0000 0100 0000 
}

/**
 * 0000 : 5c11      : CMP R1 - R2
 */
void test_cmp_positive(struct cpu *cpu) {
    zerocpu(cpu);
    cpu->R[1] = 4;
    cpu->R[2] = 1; 
    store2(cpu, 0x5c11, 0); 

    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->Z == 0); 
    assert(cpu->N == 0); 
}

void test_cmp_zero(struct cpu *cpu) {
    zerocpu(cpu);
    cpu->R[1] = 4;
    cpu->R[2] = 1; 
    store2(cpu, 0x5c11, 0); 

    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->Z == 0); 
    assert(cpu->N == 0);  
}

void test_cmp_negative(struct cpu *cpu) {
    zerocpu(cpu);
    cpu->R[1] = 1; 
    cpu->R[2] = 4; 
    store2(cpu, 0x5c11, 0); 

    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->Z == 0);  
    assert(cpu->N == 1);
}

/**
 * 0000 : 5e04      : TEST R4
 */
void test_test_positive(struct cpu *cpu) {
    zerocpu(cpu);
    cpu->R[4] = 3; 
    store2(cpu, 0x5e04, 0); 

    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->Z == 0); 
    assert(cpu->N == 0); 
}

/**
 * 0000 : 5e04      : TEST R4
 */
void test_test_zero(struct cpu *cpu) {
    zerocpu(cpu);
    cpu->R[4] = 0; 
    store2(cpu, 0x5e04, 0); 

    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->Z == 1); 
    assert(cpu->N == 0); 
}

/**
 * 0000 : 5e04      : TEST R4
 */
void test_test_negative(struct cpu *cpu) {
    zerocpu(cpu);
    cpu->R[4] = -3; 
    store2(cpu, 0x5e04, 0); 

    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->Z == 0);  
    assert(cpu->N == 1); 
}

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
    test_or(&cpu);
    test_xor(&cpu);
    test_rshift(&cpu);
    test_cmp_positive(&cpu);
    test_cmp_zero(&cpu);
    test_cmp_negative(&cpu);
    test_test_positive(&cpu);
    test_test_zero(&cpu);
    test_test_negative(&cpu);

    printf("all tests PASS\n");
}
