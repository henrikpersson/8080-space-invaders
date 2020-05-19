#include <criterion/criterion.h>
#include <criterion/assert.h>
#include <stdio.h>
#include <stdint.h>

#include "cpu.h"
#include "cpu_plugin.h"
#include "io.h"
#include "interrupts.h"

#define PC_BASE 0x0000

CPU* cpu = NULL;

void setup() {
    cpu = init(0);
}

void teardown() {
    free(cpu);
    cpu = NULL;
}

void load_program(uint8_t *program, size_t len) {
    load(cpu, 0, program, len);
}

void reset() {
    cpu->pc = PC_BASE;
}

void pc() {
    printf("PC ---> %04x\n", cpu->pc);
}

// Convert a val stored as two's complement to a signed int
int8_t dec(uint8_t val) {
    if ((val & 0x80) != 0) {
        // signed
        return -((~val) + 1); // Flip the bytes and add 1
    } else {
        // unsigned
        return val;
    }
}

void regs() {
    printf("A ---> 0x%x (decimal: %d)\n", cpu->A, dec(cpu->A));
    printf("HL ---> 0x%x (decimal: %d)\n", cpu->HL, dec(cpu->HL));
}

TestSuite(cpu, .init = setup, .fini = teardown);

Test(cpu, init) {
    cr_assert_not_null(cpu);
    cr_assert_eq(sizeof(cpu->mem), 0xffff);
    cr_assert_eq(cpu->pc, PC_BASE);
}

Test(cpu, union_) {
    cpu->B = 0xaa;
    cpu->C = 0xbb;
    cr_assert_eq(cpu->BC, 0xaabb);
    cpu->BC = cpu->BC & 0x00ff; 
    cr_assert_eq(cpu->B, 0x0);
    cr_assert_eq(cpu->C, 0xbb);
} 

Test(cpu, jmp) {
    load_program((uint8_t[]) { 0xc3, 0xad, 0xde }, 3);

    exec(cpu);

    cr_assert_eq(cpu->pc, 0xdead);
}

Test(cpu, lxi_sp) {
    load_program((uint8_t[]) { 0x31, 0xad, 0xde }, 3);

    exec(cpu);

    cr_assert_eq(cpu->sp, 0xdead);
}

// The contents of the accumulator replace the byte at the memory address 
// formed by concatenating HI ADO with LOW ADO.
Test(cpu, sta) {
    load_program((uint8_t[]) { 0x32, 0xad, 0xde }, 3);
    cpu->A = 0xbe;

    exec(cpu);

    cr_assert_eq(cpu->mem[0xdead], 0xbe);
}

// A = A + B
Test(cpu, add_b_and_flags) {
    load_program((uint8_t[]) { 0x80 }, 1);
    cpu->A = 0x03;
    cpu->B = 0x03;
    cpu->f.carry = 1;
    cpu->f.sign = 1;
    cpu->f.zero = 1;
    cpu->f.parity = 0;

    exec(cpu);

    cr_assert_eq(cpu->A, 0x06);
    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->f.sign, 0);
    cr_assert_eq(cpu->f.zero, 0);
    cr_assert_eq(cpu->f.parity, 1);

    reset();
    cpu->A = 0xae;
    cpu->B = 0xae;
    exec(cpu);

    cr_assert_eq(cpu->A, 0x5c);
    cr_assert_eq(cpu->f.carry, 1);
    cr_assert_eq(cpu->f.sign, 0);
    cr_assert_eq(cpu->f.zero, 0);
    cr_assert_eq(cpu->f.parity, 1);

    reset();
    cpu->A = 0x60;
    cpu->B = 0x60;
    exec(cpu);

    cr_assert_eq(cpu->A, 0xc0);
    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->f.sign, 1);
    cr_assert_eq(cpu->f.zero, 0);
    cr_assert_eq(cpu->f.parity, 1);

    reset();
    cpu->A = 0x00;
    cpu->B = 0x00;
    exec(cpu);

    cr_assert_eq(cpu->A, 0x00);
    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->f.sign, 0);
    cr_assert_eq(cpu->f.zero, 1);
    cr_assert_eq(cpu->f.parity, 1);

    reset();
    cpu->A = 0x80;
    cpu->B = 0x80;
    exec(cpu);

    cr_assert_eq(cpu->A, 0x0);
    cr_assert_eq(cpu->f.carry, 1);
    cr_assert_eq(cpu->f.sign, 0);
    cr_assert_eq(cpu->f.zero, 1);
    cr_assert_eq(cpu->f.parity, 1); // it's 0 but with a carry, i guess even?

    reset();
    cpu->A = 0x15;
    cpu->B = 0x15;
    exec(cpu);

    cr_assert_eq(cpu->A, 0x2A);
    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->f.sign, 0);
    cr_assert_eq(cpu->f.zero, 0);
    cr_assert_eq(cpu->f.parity, 0);

    reset();
    cpu->A = 0xFD; // -3
    cpu->B = 0x0A; // 10
    exec(cpu);

    cr_assert_eq(cpu->A, 0x07);
    cr_assert_eq(cpu->f.carry, 1);
    cr_assert_eq(cpu->f.sign, 0);
    cr_assert_eq(cpu->f.zero, 0);
    cr_assert_eq(cpu->f.parity, 0);
}

// A = A - (HL)
Test(cpu, sub_m_and_flags) {
    load_program((uint8_t[]) { 0x96 }, 1);
    cpu->HL = 0xdead;
    cpu->mem[0xdead] = 0x03;
    cpu->A = 0x06;
    cpu->f.carry = 1;
    cpu->f.sign = 1;
    cpu->f.zero = 1;
    cpu->f.parity = 0;

    exec(cpu);

    cr_assert_eq(cpu->A, 0x03);
    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->f.sign, 0);
    cr_assert_eq(cpu->f.zero, 0);
    cr_assert_eq(cpu->f.parity, 1);

    reset();
    cpu->A = 0x03;
    cpu->mem[0xdead] = 0x06;
    exec(cpu);

    cr_assert_eq(cpu->A, 0xfd); // 0xfd = -3 in two's complement
    cr_assert_eq(cpu->f.carry, 1);
    cr_assert_eq(cpu->f.sign, 1);
    cr_assert_eq(cpu->f.zero, 0);
    cr_assert_eq(cpu->f.parity, 0);

    reset();
    cpu->A = 0xFF;
    cpu->mem[0xdead] = 0xFF;
    exec(cpu);

    cr_assert_eq(cpu->A, 0x00);
    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->f.sign, 0);
    cr_assert_eq(cpu->f.zero, 1);
    cr_assert_eq(cpu->f.parity, 1);
}

// A = A XOR A
Test(cpu, xra_a) {
    load_program((uint8_t[]) { 0xaf }, 1);
    cpu->A = 0xbe;
    cpu->f.carry = 1;

    exec(cpu);

    cr_assert_eq(cpu->A, 0xbe ^ 0xbe);
    cr_assert_eq(cpu->f.carry, 0);
}

// CALL $xxxx
Test(cpu, call) {
    load_program((uint8_t[]) { 0x00, 0x00, 0xcd, 0xe6, 0x01, 0x00, 0x00, 0x00 }, 8);
    cpu->sp = 0x7;

    exec(cpu);
    exec(cpu);
    exec(cpu);

    cr_assert_eq(cpu->pc, 0x01e6);
    cr_assert_eq(cpu->mem[cpu->sp], 0x0005);
}

// LXI D, $xxxx
Test(cpu, lxi_d) {
    load_program((uint8_t[]) { 0x11, 0xad, 0xde }, 3);

    exec(cpu);

    cr_assert_eq(cpu->DE, 0xdead);
    cr_assert_eq(cpu->D, 0xde);
    cr_assert_eq(cpu->E, 0xad);
}

// INX r
Test(cpu, inx_h) {
    load_program((uint8_t[]) { 0x23 }, 1);
    cpu->HL = 0x38ff;

    exec(cpu);

    cr_assert_eq(cpu->HL, 0x3900);
    cr_assert_eq(cpu->H, 0x39);
    cr_assert_eq(cpu->L, 0x00);
}

// INX sp
Test(cpu, inx_sp) {
    load_program((uint8_t[]) { 0x33 }, 1);
    cpu->sp = 0x38ff;

    exec(cpu);

    cr_assert_eq(cpu->sp, 0x3900);

    reset();
    cpu->sp = 0xffff;
    exec(cpu);

    cr_assert_eq(cpu->sp, 0x0000);
}

// DCR r
Test(cpu, dcr_b) {
    load_program((uint8_t[]) { 0x05 }, 1);
    cpu->f.carry = 1;
    cpu->C = 0xff;
    cpu->B = 0xad;

    exec(cpu);

    cr_assert_eq(cpu->B, 0xac);
    cr_assert_eq(cpu->C, 0xff); // unchanged
    cr_assert_eq(cpu->f.carry, 1); // DCR should not affect carry
    cr_assert_eq(cpu->f.sign, 1);
    cr_assert_eq(cpu->f.zero, 0);
    cr_assert_eq(cpu->f.parity, 1);

    reset();
    cpu->B = 0x01;
    exec(cpu);

    cr_assert_eq(cpu->B, 0x00);
    cr_assert_eq(cpu->C, 0xff); // unchanged
    cr_assert_eq(cpu->f.carry, 1); // DCR should not affect carry
    cr_assert_eq(cpu->f.sign, 0);
    cr_assert_eq(cpu->f.zero, 1);
    cr_assert_eq(cpu->f.parity, 1);
}

// CPI $xx (compare immediate w/ acc)
Test(cpu, cpi) {
    load_program((uint8_t[]) { 0xfe, 0xde }, 2);
    cpu->A = 0xff;
    cpu->f.zero = 1;
    cpu->f.carry = 1;

    exec(cpu);

    cr_assert_eq(cpu->A, 0xff);
    cr_assert_eq(cpu->f.zero, 0);
    cr_assert_eq(cpu->f.carry, 0);

    reset();
    cpu->A = 0x10;
    exec(cpu);

    cr_assert_eq(cpu->A, 0x10);
    cr_assert_eq(cpu->f.zero, 0);
    cr_assert_eq(cpu->f.carry, 1);

    reset();
    cpu->A = 0xde;
    exec(cpu);

    cr_assert_eq(cpu->A, 0xde);
    cr_assert_eq(cpu->f.zero, 1);
    cr_assert_eq(cpu->f.carry, 0);
}

// RET
Test(cpu, ret) {
    load_program((uint8_t[]) { 0xc9, 0x00, 0xad, 0xde }, 4);
    cpu->sp = 0x02;

    exec(cpu);

    cr_assert_eq(cpu->sp, 0x04);
    cr_assert_eq(cpu->pc, 0xdead);
}

// PUSH
Test(cpu, push_d) {
    load_program((uint8_t[]) { 0xd5, 0x00, 0x00, 0x00 }, 4);
    cpu->sp = 0x03;
    cpu->DE = 0xdead;

    exec(cpu);

    cr_assert_eq(cpu->sp, 0x01);
    cr_assert_eq(cpu->mem[cpu->sp], 0xad);
    cr_assert_eq(cpu->mem[cpu->sp + 1], 0xde);
}

// DAD (Double add)
Test(cpu, DAD_B) {
    load_program((uint8_t[]) { 0x09 }, 1);
    cpu->BC = 0x0015;
    cpu->HL = 0x0010;
    cpu->f.carry = 1;
    cpu->f.sign = 1;
    cpu->f.zero = 1;
    cpu->f.parity = 1;

    exec(cpu);

    cr_assert_eq(cpu->HL, 0x0025);
    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->f.sign, 1);
    cr_assert_eq(cpu->f.zero, 1);
    cr_assert_eq(cpu->f.parity, 1);

    reset();
    cpu->BC = 0x00c0;
    cpu->HL = 0x0040;
    exec(cpu);

    cr_assert_eq(cpu->HL, 0x0100);
    cr_assert_eq(cpu->f.carry, 1);
    cr_assert_eq(cpu->f.sign, 1);
    cr_assert_eq(cpu->f.zero, 1);
    cr_assert_eq(cpu->f.parity, 1);

    reset();
    cpu->BC = 0x1234;
    cpu->HL = 0x0005;
    exec(cpu);

    cr_assert_eq(cpu->HL, 0x1239);
    cr_assert_eq(cpu->f.carry, 1);
    cr_assert_eq(cpu->f.sign, 1);
    cr_assert_eq(cpu->f.zero, 1);
    cr_assert_eq(cpu->f.parity, 1);
}

// XCHG, "EXCHANGE H AND L WITH D AND E"
Test(cpu, xchg) {
    load_program((uint8_t[]) { 0xeb }, 1);
    cpu->HL = 0xdead;
    cpu->DE = 0xbeef;

    exec(cpu);

    cr_assert_eq(cpu->HL, 0xbeef);
    cr_assert_eq(cpu->DE, 0xdead);
}

// XTHL, "Exchange Top of Stack with H and L"
Test(cpu, xthl) {
    load_program((uint8_t[]) { 0xe3, 0xbe, 0xba }, 3);
    cpu->sp = 0x1;
    cpu->HL = 0xdead;

    exec(cpu);

    cr_assert_eq(cpu->HL, 0xbabe);
    cr_assert_eq(cpu->mem[cpu->sp], 0xad);
    cr_assert_eq(cpu->mem[cpu->sp + 1], 0xde);
}

// POP
Test(cpu, pop_h) {
    load_program((uint8_t[]) { 0xe1, 0x00, 0x00, 0xad, 0xde, 0x00 }, 5);
    cpu->sp = 0x03;

    exec(cpu);

    cr_assert_eq(cpu->sp, 0x05);
    cr_assert_eq(cpu->HL, 0xdead);
}

// PUSH PSW, POP PSW
Test(cpu, push_pop_psw) {
    load_program((uint8_t[]) { 0xf5, 0xf1, 0x00, 0x00, 0x00 }, 5);
    flags flags_before_push = (flags) { .zero = 1, .sign = 0, .parity = 1, .carry = 1, .auxcarry = 0 };
    cpu->sp = 0x04;
    cpu->A = 0xde;
    cpu->f = flags_before_push;

    // 0xf5 = PUSH PSW
    exec(cpu);

    // flags *stack_flags = (flags*) (size_t) cpu->mem[cpu->sp + 1];
    uint8_t *flags_before_push_byte = (uint8_t*) (size_t) &flags_before_push;
    // printf("flags_b: %02x\n", *flags_b);
    cr_assert_eq(cpu->sp, 0x02);
    cr_assert_eq(cpu->mem[cpu->sp], *flags_before_push_byte);
    cr_assert_eq(cpu->mem[cpu->sp + 1], 0xde);

    // fiddle with the state (inside a subroutine)
    cpu->A = 0xbb;
    cpu->f.zero = 0;
    cpu->f.parity = 0;
    cpu->f.auxcarry = 1;

    // 0xf1 = POP PSW
    exec(cpu);

    uint8_t *flags_after_pop = (uint8_t*) (size_t) &cpu->f;
    // printf("flags_before_push: %x\n", *flags_before_push_byte);
    // printf("flags_after_pop: %x\n", *flags_after_pop);
    cr_assert_eq(cpu->sp, 0x04);
    cr_assert_eq(cpu->A, 0xde);
    cr_assert_eq(*flags_after_pop, *flags_before_push_byte);
}

// RRC, rotate acc right
Test(cpu, rrc) {
    load_program((uint8_t[]) { 0x0f }, 1);
    cpu->f.carry = 1;
    cpu->A = 0xba;

    exec(cpu);

    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->A, 0x5d);

    reset();
    exec(cpu); // on 0x5d

    cr_assert_eq(cpu->f.carry, 1);
    cr_assert_eq(cpu->A, 0xae);
}

// RLC, rotate acc left
Test(cpu, rlc) {
    load_program((uint8_t[]) { 0x07 }, 1);
    cpu->f.carry = 1;
    cpu->A = 0x5b;

    exec(cpu);

    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->A, 0xb6);

    reset();
    exec(cpu); // on 0xb6

    cr_assert_eq(cpu->f.carry, 1);
    cr_assert_eq(cpu->A, 0x6d);
}

// ADC B, Add w carry
Test(cpu, adb_b) {
    load_program((uint8_t[]) { 0x88 }, 1);
    cpu->f.carry = 0;
    cpu->A = 0x42;
    cpu->B = 0x3d;

    exec(cpu);

    cr_assert_eq(cpu->A, 0x7f);
    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->f.sign, 0);
    cr_assert_eq(cpu->f.zero, 0);
    cr_assert_eq(cpu->f.parity, 0);
    // cr_assert_eq(cpu->f.auxcarry, 0);

    reset();
    cpu->f.carry = 1;
    cpu->A = 0x42;
    cpu->B = 0x3d;
    exec(cpu);

    cr_assert_eq(cpu->A, 0x80);
    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->f.sign, 1);
    cr_assert_eq(cpu->f.zero, 0);
    cr_assert_eq(cpu->f.parity, 0);
    // cr_assert_eq(cpu->f.auxcarry, 1);
}

// SBB B, Sub w borrow
Test(cpu, sbb_b) {
    load_program((uint8_t[]) { 0x98 }, 1);
    cpu->f.carry = 1;
    cpu->A = 0x04;
    cpu->B = 0x02;

    exec(cpu);

    cr_assert_eq(cpu->A, 0x01);
    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->f.sign, 0);
    cr_assert_eq(cpu->f.zero, 0);
    cr_assert_eq(cpu->f.parity, 0);
}

// CMP B
Test(cpu, cmp_b) {
    load_program((uint8_t[]) { 0xb8 }, 1);
    cpu->f.carry = 1;
    cpu->f.zero = 1;
    cpu->A = 0x0a;
    cpu->B = 0x05;

    exec(cpu);

    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->f.zero, 0);

    reset();
    cpu->f.carry = 1;
    cpu->f.zero = 1;
    cpu->A = 0xe5;
    cpu->B = 0x05;
    exec(cpu);

    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->f.zero, 0);

    reset();
    cpu->f.zero = 0;
    cpu->A = 0xe5;
    cpu->B = 0xe5;
    exec(cpu);

    cr_assert_eq(cpu->f.zero, 1);

    reset();
    cpu->f.zero = 1;
    cpu->f.carry = 0;
    cpu->A = 0x05;
    cpu->B = 0xe5;
    exec(cpu);

    cr_assert_eq(cpu->f.zero, 0);
    cr_assert_eq(cpu->f.carry, 1);
}

// LHLD $xxxx
// LHLD loads the L register with a copy of the byte stored itt 
 //the memory location specified in bytes two and three of the LHLD instruction. 
 //LHLD then loads the H register with a copy of the byte stored at the next higher memory location.
Test(cpu, lhld) {
    // LHLD $0x0004
    load_program((uint8_t[]) { 0x2a, 0x04, 0x00, 0x00, 0xad, 0xde }, 6);

    exec(cpu);

    cr_assert_eq(cpu->HL, 0xdead);
}

// SHLD $xxxx
Test(cpu, shld) {
    // SHLD $0x0004
    load_program((uint8_t[]) { 0x22, 0x04, 0x00, 0x00, 0x00, 0x00 }, 6);
    cpu->HL = 0xdead;

    exec(cpu);

    cr_assert_eq(cpu->mem[0x4], 0xad);
    cr_assert_eq(cpu->mem[0x5], 0xde);
}


// CMC
Test(cpu, cmc) {
    load_program((uint8_t[]) { 0x3f }, 1);
    cpu->f.carry = 1;

    exec(cpu);

    cr_assert_eq(cpu->f.carry, 0);

    reset();
    cpu->f.carry = 0;
    exec(cpu);

    cr_assert_eq(cpu->f.carry, 1);
}

// RAL, rotate left through carry
Test(cpu, ral) {
    load_program((uint8_t[]) { 0x17 }, 1);
    cpu->f.carry = 0;
    cpu->A = 0xaa;
    
    exec(cpu);

    cr_assert_eq(cpu->f.carry, 1);
    cr_assert_eq(cpu->A, 0x54);

    reset();
    cpu->f.carry = 1;
    cpu->A = 0xaa;
    exec(cpu);

    cr_assert_eq(cpu->f.carry, 1);
    cr_assert_eq(cpu->A, 0x55);

    reset();
    cpu->f.carry = 1;
    cpu->A = 0x10;
    exec(cpu);

    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->A, 0x21);
}

// RAL, rotate right through carry
Test(cpu, rar) {
    load_program((uint8_t[]) { 0x1f }, 1);
    cpu->f.carry = 0;
    cpu->A = 0xaa;
    
    exec(cpu);

    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->A, 0x55);

    reset();
    cpu->f.carry = 1;
    cpu->A = 0xaa;
    exec(cpu);

    cr_assert_eq(cpu->f.carry, 0);
    cr_assert_eq(cpu->A, 0xd5);

    reset();
    cpu->f.carry = 0;
    cpu->A = 0xd5;
    exec(cpu);

    cr_assert_eq(cpu->f.carry, 1);
    cr_assert_eq(cpu->A, 0x6a);
}