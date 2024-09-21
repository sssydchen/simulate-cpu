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
void test_or(struct cpu *cpu)
{
    zerocpu(cpu);

    cpu->R[2] = 0x1712; // 0001 0111 0001 0010
    cpu->R[3] = 0xF4D6; // 1111 0100 1101 0110
    store2(cpu, 0x56DA, 0);
    int val1 = emulate(cpu);
    assert(val1 == 0);
    assert(cpu->R[3] == 0xF7D6); // 1111 0111 1101 0110

    cpu->R[5] = 0x28AF;     // 0010 1000 1010 1111
    cpu->R[7] = 0x5036;     // 0101 0000 0011 0110
    store2(cpu, 0x57FD, 2); // Instruction is 2 byte long
    int val2 = emulate(cpu);
    assert(val2 == 0);
    assert(cpu->R[7] == 0x78BF); // 0111 1000 1011 1111
}

/**
 * 0000 : 58d1      : XOR R1 ^ R2 -> R3
 * 0002 : 592c      : XOR R4 ^ R5 -> R4
 */
void test_xor(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->R[1] = 0x7612; // 0111 0110 0001 0010
    cpu->R[2] = 0x2189; // 0010 0001 1000 1001
    store2(cpu, 0x58d1, 0);

    int val1 = emulate(cpu);
    assert(val1 == 0);
    assert(cpu->R[3] == 0x579B); // 0101 0111 1001 1011

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
void test_rshift(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->R[2] = 0x0103; // 0000 0001 0000 0011
    cpu->R[4] = 0x2;
    store2(cpu, 0x5B22, 0);

    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->R[4] == 0x40); // 0000 0000 0100 0000
}

/**
 * 0000 : 5c11      : CMP R1 - R2
 */
void test_cmp_positive(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->R[1] = 4;
    cpu->R[2] = 1;
    store2(cpu, 0x5c11, 0);

    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->Z == 0);
    assert(cpu->N == 0);
}

void test_cmp_zero(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->R[1] = 4;
    cpu->R[2] = 1;
    store2(cpu, 0x5c11, 0);

    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->Z == 0);
    assert(cpu->N == 0);
}

void test_cmp_negative(struct cpu *cpu)
{
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
void test_test_positive(struct cpu *cpu)
{
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
void test_test_zero(struct cpu *cpu)
{
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
void test_test_negative(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->R[4] = -3;
    store2(cpu, 0x5e04, 0);

    int val = emulate(cpu);
    assert(val == 0);
    assert(cpu->Z == 0);
    assert(cpu->N == 1);
}

/**
 * 0000 : 3001 5678 : STORE R1 -> *0x5678
 */
void test_store_word_direct(struct cpu *cpu)
{
    zerocpu(cpu);

    cpu->R[1] = 0xABCD;

    store2(cpu, 0x3001, 0);
    store2(cpu, 0x5678, 2);

    int val = emulate(cpu);
    assert(cpu->memory[0x5678] == 0xCD);
    assert(cpu->memory[0x5678 + 1] == 0xAB);
}

/**
 *  0004 : 3402 5678 : STORE.B R2 -> *0x5678
 */
void test_store_byte_direct(struct cpu *cpu)
{
    zerocpu(cpu);

    cpu->R[2] = 0x1234;

    store2(cpu, 0x3402, 0);
    store2(cpu, 0x5678, 2);

    int val = emulate(cpu);
    assert(cpu->memory[0x5678] == 0x34);
}

/**
 * 0008 : 382b      : STORE R3 -> *R5
 */
void test_store_word_indirect(struct cpu *cpu)
{
    zerocpu(cpu);

    cpu->R[3] = 0x5678;
    cpu->R[5] = 0x2000;

    store2(cpu, 0x382B, 0);

    int val = emulate(cpu);
    assert(cpu->memory[0x2000] == 0x78);
    assert(cpu->memory[0x2000 + 1] == 0x56);
}

/**
 * 000a : 3c2c      : STORE.B R4 -> *R5
 */
void test_store_byte_indirect(struct cpu *cpu)
{
    zerocpu(cpu);

    cpu->R[4] = 0x5678;
    cpu->R[5] = 0x2000;

    store2(cpu, 0x3C2C, 0);
    int val = emulate(cpu);
    assert(cpu->memory[0x2000] == 0x78);
}

/**
 *  0000 : 4021      : MOV R1 -> R2
 *  0002 : 4038      : MOV SP -> R3
 *  0004 : 4082      : MOV R2 -> SP
 */
void test_move_reg_to_reg(struct cpu *cpu)
{
    zerocpu(cpu);

    cpu->R[1] = 0x1111;
    store2(cpu, 0x4021, 0);
    int val = emulate(cpu);
    assert(cpu->R[2] == 0x1111);
}

void test_move_sp_to_reg(struct cpu *cpu)
{
    zerocpu(cpu);

    cpu->SP = 0x2222;
    store2(cpu, 0x4038, 0);
    int val = emulate(cpu);
    assert(cpu->R[3] == 0x2222);
}

void test_move_reg_to_sp(struct cpu *cpu)
{
    zerocpu(cpu);

    cpu->R[2] = 0x3333;
    store2(cpu, 0x4082, 0);
    int val = emulate(cpu);
    assert(cpu->SP == 0x3333);
}

/**
 * 0000 : 6000 1234 : JMP 0x1234
 * Direct unconditional jump to the address 0x1234
 */
void test_jmp_direct(struct cpu *cpu) {
    zerocpu(cpu);

    // Store the JMP instruction in memory (address 0x0000)
    store2(cpu, 0x6000, 0);  // JMP to address 0x1234
    store2(cpu, 0x6787, 2);  // Target address (0x1234)

    int val = emulate(cpu);

    // After the jump, PC should be 0x1234
    assert(val == 0);
    assert(cpu->PC == 0x6787);
}

/*
    0000 : 1005 000c : SET R5 = 0x000c
    0004 : 8000 000a : CALL 0x000a
    0008 : f000      : HALT
    000a : 05 e0 48 00       ? ? H ?
*/
void test_cpu_specified_address(struct cpu *cpu)
{
    zerocpu(cpu);
    cpu->SP = 1024;

    store2(cpu, 0x1005, 0);
    store2(cpu, 0x000c, 2);
    int val = emulate(cpu);

    store2(cpu, 0x8000, 4); // CALL instruction occurs here.
    store2(cpu, 0x000a, 6);
    val = emulate(cpu);

    assert(val == 0);
    assert(cpu->SP == 1022);
    assert(load2(cpu, cpu->SP) == 0x0008); // checking if the instruction at PC + 4 is stored at address in SP
    assert(cpu->PC == 0x000a);

    store2(cpu, 0xF000, 8);
    store2(cpu, 0xe005, 10);
    val = emulate(cpu);

    store2(cpu, 0x0048, 12);
    val = emulate(cpu);
}

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
    0000 : 1001 054a : SET R1 = 0x054a
    0004 : 1002 0591 : SET R2 = 0x0591
    0008 : f000      : HALT
*/
void test_halt(struct cpu *cpu)
{
    zerocpu(cpu);
    store2(cpu, 0x1001, 0);
    emulate(cpu);
    store2(cpu, 0x054a, 2);
    emulate(cpu);
    store2(cpu, 0x1002, 4);
    emulate(cpu);
    store2(cpu, 0x0591, 6);
    emulate(cpu);
    store2(cpu, 0xf000, 4);

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

    test_or(&cpu);
    test_xor(&cpu);
    test_rshift(&cpu);
    test_cmp_positive(&cpu);
    test_cmp_zero(&cpu);
    test_cmp_negative(&cpu);
    test_test_positive(&cpu);
    test_test_zero(&cpu);
    test_test_negative(&cpu);

    test_store_word_direct(&cpu);
    test_store_byte_direct(&cpu);
    test_store_word_indirect(&cpu);
    test_store_byte_indirect(&cpu);

    test_move_reg_to_reg(&cpu);
    test_move_reg_to_sp(&cpu);
    test_move_sp_to_reg(&cpu);

    test_jmp_direct(&cpu);

    test_cpu_specified_address(&cpu);
    test_cpu_address_indirect(&cpu);
    test_ret(&cpu);
    test_push_and_pop(&cpu);
    test_in_and_out(&cpu);
    test_halt(&cpu);

    printf("all tests PASS\n");
}
