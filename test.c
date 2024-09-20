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

/*
    CALL *R3
    SUB R3 - R2 -> R1
*/
void test_cpu_address_indirect(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->SP = 30;
    cpu->R[3] = 5;

    store2(cpu, 0x9003, 0); // adding the CALL instruction on R3.
    store2(cpu, 0x5253, 2); // dummy instruction for setting it to PC + 2.
    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->memory[28] == 2);
    assert(cpu->SP == 28);
    assert(cpu->PC == 5);
}

/*
        RET
*/
void test_ret(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->R[3] = 2;
    cpu->R[2] = 1;
    cpu->SP = 10;
    cpu->PC = 100;
    cpu->memory[10] = 50;

    store2(cpu, 0xA000, 100);
    int val = emulate(cpu);

    assert(val == 0);
    assert(cpu->SP == 12);
    assert(cpu->PC == 50);
}

/*
    CALL R6
*/
void test_push(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->R[6] = 25;
    cpu->SP = 10;

    store2(cpu, 0xB006, 0);
    int val = emulate(cpu);

    assert(val == 0);
    assert(cpu->SP == 8);
    assert(cpu->memory[8] == 25);
}

/*
    POP R7
*/
void test_pop(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->SP = 10;
    cpu->memory[10] = 100;

    store2(cpu, 0xC007, 0);
    int val = emulate(cpu);

    assert(val == 0);
    assert(cpu->SP == 12);
    assert(cpu->R[7] == 100);
}

/*
    IN R5;
*/
void test_in(struct cpu *cpu)
{
    FILE *original_stdin = stdin;
    FILE *mock_stdin = tmpfile();
    fputs("A", mock_stdin);
    rewind(mock_stdin);
    stdin = mock_stdin;
    zerocpu(cpu);

    store2(cpu, 0xD005, 0);
    int val = emulate(cpu);

    assert(val == 0);
    assert(cpu->PC == 2);
    assert(cpu->R[5] == 'A');
    stdin = original_stdin;
}

/*
    OUT R5;
*/
void test_out(struct cpu *cpu)
{
    FILE *original_stdout = stdout;
    FILE *mock_stdout = tmpfile();
    stdout = mock_stdout;
    char output_buffer[100];
    zerocpu(cpu);
    cpu->R[5] = 'A';

    store2(cpu, 0xE005, 0);
    int val = emulate(cpu);
    rewind(mock_stdout);
    fgets(output_buffer, sizeof(output_buffer), mock_stdout);
    fclose(mock_stdout);

    assert(val == 0);
    assert(cpu->PC == 2);
    assert(strcmp(output_buffer, "A") == 0);

    stdout = original_stdout;
}

/*
    HALT
*/
void test_halt(struct cpu *cpu)
{
    zerocpu(cpu);
    store2(cpu, 0xF000, 0);

    int val = emulate(cpu);
    assert(val == 1);
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
    test_cpu_address_indirect(&cpu);
    test_ret(&cpu);
    test_push(&cpu);
    test_pop(&cpu);
    test_in(&cpu);
    test_out(&cpu);
    test_halt(&cpu);

    printf("all tests PASS\n");
}
