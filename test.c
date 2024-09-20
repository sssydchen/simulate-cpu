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

/*
    0000 : 5253      : SUB R3 - R2 -> R1
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

/*
    0000 : 5253      : SUB R3 - R2 -> R1
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

/*
    0000 : 1000 000a : SET R0 = 0x000a
    0004 : 1005 000c : SET R5 = 0x000c
    0008 : 9000      : CALL *R0
    000a : e005      : OUT R5
    000c : 0048      : OUT R5
*/
void test_cpu_address_indirect(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->SP = 1000;

    store2(cpu, 0x1000, 0);
    store2(cpu, 0x000A, 2);
    int val = emulate(cpu);
    store2(cpu, 0x1005, 4);
    store2(cpu, 0x000C, 6);
    emulate(cpu);
    store2(cpu, 0x9000, 8);
    val = emulate(cpu); // the CALL instruction is called here.

    assert(val == 0);
    assert(cpu->SP == 998);
    assert(cpu->PC == 0x000A);

    store2(cpu, 0xE005, 10);
    store2(cpu, 0x0048, 12);
    val = emulate(cpu);

    assert(val == 0);
}

/*
0000 : 1000 000c : SET R0 = 0x000c
0004 : 1005 000e : SET R5 = 0x000e
0008 : 9000      : CALL *R0
000a : e005      : OUT R5
000c : a000      : RET
000e : 0048      : RET
*/
void test_ret(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->SP = 500;

    store2(cpu, 0x1000, 0);
    store2(cpu, 0x000c, 2);
    int val = emulate(cpu);

    store2(cpu, 0x1005, 4);
    store2(cpu, 0x000e, 6);
    val = emulate(cpu);

    store2(cpu, 0x9000, 8);
    val = emulate(cpu);

    store2(cpu, 0xe005, 10);
    val = emulate(cpu);
    store2(cpu, 0xa000, 12);
    val = emulate(cpu); // the RET instruction is called here.
    assert(val == 0);
    assert(cpu->PC == 10);

    store2(cpu, 0x0048, 14);
    val = emulate(cpu);
    assert(val == 0);
}

/*
    0000 : 1001 04d2 : SET R1 = 0x04d2
    0004 : 1002 162e : SET R2 = 0x162e
    0008 : b001      : PUSH R1
    000a : b002      : PUSH R2
    000c : c001      : POP R1
    000e : c002      : POP R2
*/
void test_push_and_pop(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->SP = 1024;

    store2(cpu, 0x1001, 0);
    store2(cpu, 0x04d2, 2);
    int val = emulate(cpu);

    store2(cpu, 0x1002, 4);
    store2(cpu, 0x162e, 6);
    val = emulate(cpu);

    store2(cpu, 0xb001, 8); // PUSH instruction executed here.
    val = emulate(cpu);
    assert(val == 0);
    assert(cpu->SP == 1022);
    assert(load2(cpu, cpu->SP) == 1234);

    store2(cpu, 0xb002, 10); // PUSH instruction executed here.
    val = emulate(cpu);
    assert(val == 0);
    assert(cpu->SP == 1020);
    assert(load2(cpu, cpu->SP) == 5678);

    store2(cpu, 0xc003, 12); // POP instruction executed here.
    val = emulate(cpu);
    assert(val == 0);
    assert(cpu->SP == 1022);
    assert(cpu->R[3] == 5678);

    store2(cpu, 0xc004, 14); // POP instruction executed here.
    val = emulate(cpu);
    assert(val == 0);
    assert(cpu->SP == 1024);
    assert(cpu->R[4] == 1234);
}

/*
    0000 : d004      : IN R4
    0002 : d005      : IN R5
    0004 : e005      : OUT R5
    0006 : e004      : OUT R4
*/
void test_in_and_out(struct cpu *cpu)
{
    FILE *original_stdin = stdin; // redirecting stdin for testing purposes
    FILE *mock_stdin = tmpfile();
    fputs("AB", mock_stdin);
    rewind(mock_stdin);
    stdin = mock_stdin;

    FILE *original_stdout = stdout; // redirecting stdout for testing purposes
    FILE *mock_stdout = tmpfile();
    stdout = mock_stdout;
    char output_buffer[100];

    zerocpu(cpu);

    store2(cpu, 0xd004, 0);
    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->R[4] == 'A');

    store2(cpu, 0xd005, 2);
    val = emulate(cpu);
    assert(val == 0);
    assert(cpu->R[5] == 'B');

    store2(cpu, 0xe005, 4);
    val = emulate(cpu);
    assert(val == 0);

    store2(cpu, 0xe004, 6);
    val = emulate(cpu);

    rewind(mock_stdout);
    fgets(output_buffer, sizeof(output_buffer), mock_stdout);
    fclose(mock_stdout);
    assert(val == 0);
    assert(strcmp(output_buffer, "BA") == 0);

    // clearing the redirects.
    stdin = original_stdin;
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
    test_push_and_pop(&cpu);
    test_in_and_out(&cpu);
    test_halt(&cpu);

    printf("all tests PASS\n");
}
