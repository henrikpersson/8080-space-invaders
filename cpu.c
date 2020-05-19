#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "cpu.h"
#include "cpu_plugin.h"

CPU* init(const uint16_t base_addr) {
    CPU *cpu = calloc(sizeof(CPU), 1);
    cpu->pc = base_addr;
    // TODO: Not sure the stack should start.. 
    // 0x23ff if the top of RAM (stack grows downwards) for Space Invaders
    // so let's hope that works...
    cpu->sp = 0x23ff;
    // cpu->sp = 0x2fff;
    return cpu;
}

// Convert a val stored as two's complement to a signed int
int8_t cdec(uint8_t val) {
    if ((val & 0x80) != 0) {
        // signed
        return -((~val) + 1); // Flip the bytes and add 1
    } else {
        // unsigned
        return val;
    }
}

void load(CPU* cpu, const uint16_t base_addr, const uint8_t *program, size_t size) {
    memcpy(&cpu->mem[base_addr], program, size);
}

void todo(const char *op) {
    printf("^ TODO: IMPLEMENT: %s\n", op);
}

bool parity(const uint8_t res) {
    // TODO: there's probably a smarter way, look it up
    uint8_t on_bits = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if ((((res << i)) & 0x80) == 0x80) {
            on_bits++;
        }
    }
    return on_bits % 2 == 0;
}

void setflags(CPU* cpu, const uint16_t res) {
    // printf("res --> 0x%04x\n", result);
    cpu->f.sign = (res & 0x80) == 0x80;
    cpu->f.zero = (res & 0x00ff) == 0;
    cpu->f.parity = parity(res);
    // cpu->f.auxcarry = res > 0x8; // TODO: There's an error here. CPUTEST.COM catches it, but i dont care atm
}

void setflags_carry(CPU* cpu, uint16_t res) {
    cpu->f.carry = res > 0xff;
}

void reset_carries(CPU *cpu) {
    cpu->f.carry = 0;
    // cpu->f.auxcarry = 0;
}

typedef enum { ADD, SUB, XOR, AND, OR } arith_op;
char opch(const arith_op op) {
    switch (op) {
        case ADD: return '+';
        case SUB: return '-';
        case XOR: return '^';
        case AND: return '&';
        case OR: return '|';
    }
    return '?';
}

uint16_t arithmetixx(CPU* cpu, const arith_op op, const uint16_t lhs, const uint16_t rhs) {
    uint16_t result;
    switch (op) {
        case ADD: result = lhs + rhs; break;
        case SUB: result = lhs - rhs; break;
        case XOR: result = lhs ^ rhs; break;
        case AND: result = lhs & rhs; break;
        case OR: result = lhs | rhs; break;
        default: assert(0); 
    }
    return result;
}

uint16_t arithmetix_only_carry(CPU* cpu, const arith_op op, const uint16_t lhs, const uint16_t rhs) {
    const uint16_t res = arithmetixx(cpu, op, lhs, rhs);
    setflags_carry(cpu, res);
    return res;
}

uint8_t arithmetix(CPU* cpu, const arith_op op, const uint8_t val) {
    uint16_t res = arithmetixx(cpu, op, cpu->A, val);
    setflags(cpu, res);
    setflags_carry(cpu, res);
    // printf("%x %c %x = %x | c=%d, z=%d, p=%d, s=%d\n", cpu->A, opch(op), val, res, cpu->f.carry, cpu->f.zero, cpu->f.parity, cpu->f.sign);
    return res;
}

uint8_t rotr(const uint8_t val) {
    return (val >> 1) | (val << 7);
}

uint16_t rotl(const uint8_t val) {
    return (val << 1) | (val >> 7);
}

void push(CPU* cpu, const uint16_t val) {
    cpu->sp--;
    cpu->mem[cpu->sp] = val >> 8; // 1st byte
    cpu->sp--;
    cpu->mem[cpu->sp] = val & 0x00ff; // 2nd byte
    // printf("SP after push: 0x%x (%d)\n", cpu->sp, cpu->sp);
    // printf("top of stack: 0x%x 0x%x\n", cpu->mem[cpu->sp], cpu->mem[cpu->sp + 1]);
}

uint16_t pop(CPU* cpu) {
    uint16_t val = cpu->mem[cpu->sp];
    cpu->sp++;
    val = (cpu->mem[cpu->sp] << 8) | (val & 0x00ff);
    cpu->sp++;
    // printf("pop: %x\n", val);
    return val;
}

void call(CPU* cpu, const uint8_t high, const uint8_t low) {
    // printf("CALL TO 0x%x, RET IS 0x%x\n", (high << 8) | low, cpu->pc + 3);
    push(cpu, cpu->pc + 3); // +3 bc this op is 3 bytes and RET addr should be next op
    cpu->pc = (high << 8) | low;
}

// TODO: would be nicer to do like this instead
// cpu->pc = cond_ret(cpu, cpu->f.sign == 0)
// if cond is met, it returns the addr from pop,
// otherwise it returns pc + 3
void ret(CPU* cpu) {
    cpu->pc = pop(cpu);
    cpu_plugin_ret(cpu->pc);
    // printf("RET 0x%x\n", cpu->pc);
}

void mov(CPU* cpu, const uint8_t opcode) {
    switch (opcode) {
        case 0x40: cpu->B = cpu->B; break;
        case 0x41: cpu->B = cpu->C; break;
        case 0x42: cpu->B = cpu->D; break;
        case 0x43: cpu->B = cpu->E; break;
        case 0x44: cpu->B = cpu->H; break;
        case 0x45: cpu->B = cpu->L; break;
        case 0x46: cpu->B = cpu->mem[cpu->HL]; break;
        case 0x47: cpu->B = cpu->A; break;
        case 0x48: cpu->C = cpu->B; break;
        case 0x49: cpu->C = cpu->C; break;
        case 0x4a: cpu->C = cpu->D; break;
        case 0x4b: cpu->C = cpu->E; break;
        case 0x4c: cpu->C = cpu->H; break;
        case 0x4d: cpu->C = cpu->L; break;
        case 0x4e: cpu->C = cpu->mem[cpu->HL]; break;
        case 0x4f: cpu->C = cpu->A; break;
        case 0x50: cpu->D = cpu->B; break;
        case 0x51: cpu->D = cpu->C; break;
        case 0x52: cpu->D = cpu->D; break;
        case 0x53: cpu->D = cpu->E; break;
        case 0x54: cpu->D = cpu->H; break;
        case 0x55: cpu->D = cpu->L; break;
        case 0x56: cpu->D = cpu->mem[cpu->HL]; break;
        case 0x57: cpu->D = cpu->A; break;
        case 0x58: cpu->E = cpu->B; break;
        case 0x59: cpu->E = cpu->C; break;
        case 0x5a: cpu->E = cpu->D; break;
        case 0x5b: cpu->E = cpu->E; break;
        case 0x5c: cpu->E = cpu->H; break;
        case 0x5d: cpu->E = cpu->L; break;
        case 0x5e: cpu->E = cpu->mem[cpu->HL]; break;
        case 0x5f: cpu->E = cpu->A; break;
        case 0x60: cpu->H = cpu->B; break;
        case 0x61: cpu->H = cpu->C; break;
        case 0x62: cpu->H = cpu->D; break;
        case 0x63: cpu->H = cpu->E; break;
        case 0x64: cpu->H = cpu->H; break;
        case 0x65: cpu->H = cpu->L; break;
        case 0x66: cpu->H = cpu->mem[cpu->HL]; break;
        case 0x67: cpu->H = cpu->A; break;
        case 0x68: cpu->L = cpu->B; break;
        case 0x69: cpu->L = cpu->C; break;
        case 0x6a: cpu->L = cpu->D; break;
        case 0x6b: cpu->L = cpu->E; break;
        case 0x6c: cpu->L = cpu->H; break;
        case 0x6d: cpu->L = cpu->L; break;
        case 0x6e: cpu->L = cpu->mem[cpu->HL]; break;
        case 0x6f: cpu->L = cpu->A; break;
        case 0x70: cpu->mem[cpu->HL] = cpu->B; break;
        case 0x71: cpu->mem[cpu->HL] = cpu->C; break;
        case 0x72: cpu->mem[cpu->HL] = cpu->D; break;
        case 0x73: cpu->mem[cpu->HL] = cpu->E; break;
        case 0x74: cpu->mem[cpu->HL] = cpu->H; break;
        case 0x75: cpu->mem[cpu->HL] = cpu->L; break;
        case 0x77: cpu->mem[cpu->HL] = cpu->A; break;
        case 0x78: cpu->A = cpu->B; break;
        case 0x79: cpu->A = cpu->C; break;
        case 0x7a: cpu->A = cpu->D; break;
        case 0x7b: cpu->A = cpu->E; break;
        case 0x7c: cpu->A = cpu->H; break;
        case 0x7d: cpu->A = cpu->L; break;
        case 0x7e: cpu->A = cpu->mem[cpu->HL]; break;
        case 0x7f: cpu->A = cpu->A; break;
        default: assert(0);
    }

    cpu->pc += 1;
}

void exec(CPU* cpu) {
    const uint8_t *opcode = &cpu->mem[cpu->pc];
    const uint8_t high = opcode[2];
    const uint8_t low = opcode[1];

    if (cpu_plugin_op(cpu, *opcode, high, low)) {
        return;
    }

    switch(*opcode) {
        // NOP
        case 0x00: cpu->pc += 1; break;
        // LXI B, $xxxx
        case 0x01: cpu->BC = (high << 8) | low; cpu->pc += 3; break;
        // STAX B
        case 0x02: cpu->mem[cpu->BC] = cpu->A; cpu->pc += 1; break;
        // INX B
        case 0x03: cpu->BC += 1; cpu->pc += 1; break;
        // INR B
        case 0x04: cpu->B++; setflags(cpu, cpu->B); cpu->pc += 1; break;
        // DCR B
        case 0x05: cpu->B -= 1; setflags(cpu, cpu->B); cpu->pc += 1; break;
        // MVI B, $xx
        case 0x06: cpu->B = low; cpu->pc += 2; break;
        // RLC
        case 0x07: cpu->f.carry = (cpu->A & 0x80) != 0; cpu->A = rotl(cpu->A); cpu->pc += 1; break;
        // DAD B
        case 0x09: cpu->HL = arithmetix_only_carry(cpu, ADD, cpu->HL, cpu->BC); cpu->pc += 1; break;
        // LDAX B
        case 0x0a: cpu->A = cpu->mem[cpu->BC]; cpu->pc += 1; break;
        // DCX B
        case 0x0b: cpu->BC--; cpu->pc += 1; break;
        // INR C
        case 0x0c: cpu->C++; setflags(cpu, cpu->C); cpu->pc += 1; break;
        // DCR C
        case 0x0d: cpu->C -= 1; setflags(cpu, cpu->C); cpu->pc += 1; break;
        // MVI C, $xx
        case 0x0e: cpu->C = low; cpu->pc += 2; break;
        // RRC
        case 0x0f: cpu->f.carry = (cpu->A & 1) == 1; cpu->A = rotr(cpu->A); cpu->pc += 1; break;
        // LXI D, $xxxx
        case 0x11: cpu->DE = (high << 8) | low; cpu->pc += 3; break;
        // STAX D
        case 0x12: cpu->mem[cpu->DE] = cpu->A; cpu->pc += 1; break;
        // INX D
        case 0x13: cpu->DE += 1; cpu->pc += 1; break;
        // INR D
        case 0x14: cpu->D++; setflags(cpu, cpu->D); cpu->pc += 1; break;
        // DCR D
        case 0x15: cpu->D -= 1; setflags(cpu, cpu->D); cpu->pc += 1; break;
        // MVI D, $xx
        case 0x16: cpu->D = low; cpu->pc += 2; break;
        // RAL
        case 0x17: {
            uint16_t res = (cpu->A << 1) + cpu->f.carry;
            setflags_carry(cpu, res);
            cpu->A = res;
            cpu->pc += 1; 
            break;
        }
        // DAD D
        case 0x19: cpu->HL = arithmetix_only_carry(cpu, ADD, cpu->HL, cpu->DE); cpu->pc += 1; break;
        // LDAX D
        case 0x1a: cpu->A = cpu->mem[cpu->DE]; cpu->pc += 1; break;
        // DCX D
        case 0x1b: cpu->DE--; cpu->pc += 1; break;
        // INR E
        case 0x1c: cpu->E++; setflags(cpu, cpu->E); cpu->pc += 1; break;
        // DCR E
        case 0x1d: cpu->E -= 1; setflags(cpu, cpu->E); cpu->pc += 1; break;
        // MVI E, $xx
        case 0x1e: cpu->E = low; cpu->pc += 2; break;
        // RAR
        case 0x1f: {
            uint8_t res = cpu->A >> 1;
            if(cpu->f.carry == 1) {
                res |= 0x80; 
            }
            cpu->f.carry = (cpu->A & 1) == 1;
            cpu->A = res;
            cpu->pc += 1; 
            break;
        }
        // LXI H, $xxxx
        case 0x21: cpu->HL = (high << 8) | low; cpu->pc += 3; break;
        // SHLD $xxx
        case 0x22: {
            const uint16_t addr = (high << 8) | low;
            cpu->mem[addr] = cpu->L;
            cpu->mem[addr + 1] = cpu->H;
            cpu->pc += 3; 
            break;
        }
        // INX H
        case 0x23: cpu->HL += 1; cpu->pc += 1; break;
        // INR H
        case 0x24: cpu->H++; setflags(cpu, cpu->H); cpu->pc += 1; break;
        // DCR H
        case 0x25: cpu->H -= 1; setflags(cpu, cpu->H); cpu->pc += 1; break;
        // MVI H, $xx
        case 0x26: cpu->H = low; cpu->pc += 2; break;
        // DAA, Decimal adjust, only op that uses aux carry
        case 0x27: {
            // assert(0);
            /*
            1. If the least significant four bits of the accumulator have a value greater 
            than nine, or if the auxiliary carry flag is ON, DAA adds six to the accumulator.
            2. If the most significant four bits of the accumulator have 
            a value greater than nine, or if the carry flag is ON, DAA adds 
            six to the most significant four bits of the accumulator.
            */
            // uint8_t add = 0;
            // if (cpu->f.auxcarry == 1 || (cpu->A & 0x0f) > 9) {
            //     add = 0x06;
            // }
            // if (cpu->f.carry == 1 || (cpu->A >> 4) > 9 || ((cpu->A >> 4) >= 9 && (cpu->A & 0x0f) > 9)) {
            //     add |= 0x60;
            // }
            // cpu->A = arithmetix(cpu, ADD, add);


            if((cpu->A & 0xf) > 9){
	        	cpu->A += 6;
	        }

	        if((cpu->A & 0xf) > 0x90){
		        uint16_t result = (uint16_t) cpu->A + 0x60;
		        cpu->A = result & 0xff;
		        setflags(cpu, result);
            }

            cpu->pc += 1;
            break;
        }
        // DAD H
        case 0x29: cpu->HL = arithmetix_only_carry(cpu, ADD, cpu->HL, cpu->HL); cpu->pc += 1; break;
        // LHLD $xxx
        case 0x2a: {
            const uint16_t addr = (high << 8) | low;
            cpu->L = cpu->mem[addr]; 
            cpu->H = cpu->mem[addr + 1]; 
            cpu->pc += 3; 
            break;
        }
        // DCX H
        case 0x2b: cpu->HL--; cpu->pc += 1; break;
        // INR L
        case 0x2c: cpu->L++; setflags(cpu, cpu->L); cpu->pc += 1; break;
        // DCR L
        case 0x2d: cpu->L -= 1; setflags(cpu, cpu->L); cpu->pc += 1; break;
        // MVI L, $xx
        case 0x2e: cpu->L = low; cpu->pc += 2; break;
        // CMA
        case 0x2f: cpu->A = ~cpu->A; cpu->pc += 1; break;
        // LXI SP, $xxxx
        case 0x31: cpu->sp = (high << 8) | low; cpu->pc += 3; break;
        // STA $xxxx
        case 0x32: {
            cpu->mem[(high << 8) | low] = cpu->A;
            cpu->pc += 3;
            break;
        }
        // INX SP
        case 0x33: cpu->sp += 1; cpu->pc += 1; break;
        // INR M
        case 0x34: cpu->mem[cpu->HL]++; setflags(cpu, cpu->mem[cpu->HL]); cpu->pc += 1; break;
        // DCR M
        case 0x35: cpu->mem[cpu->HL] -= 1; setflags(cpu, cpu->mem[cpu->HL]); cpu->pc += 1; break;
        // MVI M, $xx
        case 0x36: cpu->mem[cpu->HL] = low; cpu->pc += 2; break;
        // STC
        case 0x37: cpu->f.carry = 1; cpu->pc += 1; break;
        // DAD SP
        case 0x39: cpu->HL = arithmetix_only_carry(cpu, ADD, cpu->HL, cpu->sp); cpu->pc += 1; break;
        // LDA $xxxx
        case 0x3a: cpu->A = cpu->mem[(high << 8) | low]; cpu->pc += 3; break;
        // DCX SP
        case 0x3b: cpu->sp--; cpu->pc += 1; break;
        // INR A
        case 0x3c: cpu->A++; setflags(cpu, cpu->A); cpu->pc += 1; break;
        // DCR A
        case 0x3d: cpu->A -= 1; setflags(cpu, cpu->A); cpu->pc += 1; break;
        // MVI A, $xx
        case 0x3e: cpu->A = low; cpu->pc += 2; break;
        // CMC
        case 0x3f: cpu->f.carry = ~cpu->f.carry; cpu->pc += 1; break;
        // MOV x, x
        case 0x40 ... 0x75: mov(cpu, *opcode); break;
        // HLT
        case 0x76: todo("HLT"); cpu->pc += 1; break;
        // MOV x, x
        case 0x77 ... 0x7f: mov(cpu, *opcode); break;
        // ADD B
        case 0x80: cpu->A = arithmetix(cpu, ADD, cpu->B); cpu->pc += 1; break;
        // ADD C
        case 0x81: cpu->A = arithmetix(cpu, ADD, cpu->C); cpu->pc += 1; break;
        // ADD D
        case 0x82: cpu->A = arithmetix(cpu, ADD, cpu->D); cpu->pc += 1; break;
        // ADD E
        case 0x83: cpu->A = arithmetix(cpu, ADD, cpu->E); cpu->pc += 1; break;
        // ADD H
        case 0x84: cpu->A = arithmetix(cpu, ADD, cpu->H); cpu->pc += 1; break;
        // ADD L
        case 0x85: cpu->A = arithmetix(cpu, ADD, cpu->L); cpu->pc += 1; break;
        // ADD M
        case 0x86: cpu->A = arithmetix(cpu, ADD, cpu->mem[cpu->HL]); cpu->pc += 1; break;
        // ADD A
        case 0x87: cpu->A = arithmetix(cpu, ADD, cpu->A); cpu->pc += 1; break;
        // ADC B
        case 0x88: cpu->A = arithmetix(cpu, ADD, cpu->B + cpu->f.carry); cpu->pc += 1; break;
        // ADC C
        case 0x89: cpu->A = arithmetix(cpu, ADD, cpu->C + cpu->f.carry); cpu->pc += 1; break;
        // ADC D
        case 0x8a: cpu->A = arithmetix(cpu, ADD, cpu->D + cpu->f.carry); cpu->pc += 1; break;
        // ADC E
        case 0x8b: cpu->A = arithmetix(cpu, ADD, cpu->E + cpu->f.carry); cpu->pc += 1; break;
        // ADC H
        case 0x8c: cpu->A = arithmetix(cpu, ADD, cpu->H + cpu->f.carry); cpu->pc += 1; break;
        // ADC L
        case 0x8d: cpu->A = arithmetix(cpu, ADD, cpu->L + cpu->f.carry); cpu->pc += 1; break;
        // ADC M
        case 0x8e: cpu->A = arithmetix(cpu, ADD, cpu->mem[cpu->HL] + cpu->f.carry); cpu->pc += 1; break;
        // ADC A
        case 0x8f: cpu->A = arithmetix(cpu, ADD, cpu->A + cpu->f.carry); cpu->pc += 1; break;
        // SUB B
        case 0x90: cpu->A = arithmetix(cpu, SUB, cpu->B); cpu->pc += 1; break;
        // SUB C
        case 0x91: cpu->A = arithmetix(cpu, SUB, cpu->C); cpu->pc += 1; break;
        // SUB D
        case 0x92: cpu->A = arithmetix(cpu, SUB, cpu->D); cpu->pc += 1; break;
        // SUB E
        case 0x93: cpu->A = arithmetix(cpu, SUB, cpu->E); cpu->pc += 1; break;
        // SUB H
        case 0x94: cpu->A = arithmetix(cpu, SUB, cpu->H); cpu->pc += 1; break;
        // SUB L
        case 0x95: cpu->A = arithmetix(cpu, SUB, cpu->L); cpu->pc += 1; break;
        // SUB M
        case 0x96: cpu->A = arithmetix(cpu, SUB, cpu->mem[cpu->HL]); cpu->pc += 1; break;
        // SUB A
        case 0x97: cpu->A = arithmetix(cpu, SUB, cpu->A); cpu->pc += 1; break;
        // SBB B TODO: not sure about all the cases for
        case 0x98: cpu->A = arithmetix(cpu, SUB, cpu->B + cpu->f.carry); cpu->pc += 1; break;
        // SBB C
        case 0x99: cpu->A = arithmetix(cpu, SUB, cpu->C + cpu->f.carry); cpu->pc += 1; break;
        // SBB D
        case 0x9a: cpu->A = arithmetix(cpu, SUB, cpu->D + cpu->f.carry); cpu->pc += 1; break;
        // SBB E
        case 0x9b: cpu->A = arithmetix(cpu, SUB, cpu->E + cpu->f.carry); cpu->pc += 1; break;
        // SBB H
        case 0x9c: cpu->A = arithmetix(cpu, SUB, cpu->H + cpu->f.carry); cpu->pc += 1; break;
        // SBB L
        case 0x9d: cpu->A = arithmetix(cpu, SUB, cpu->L + cpu->f.carry); cpu->pc += 1; break;
        // SBB M
        case 0x9e: cpu->A = arithmetix(cpu, SUB, cpu->mem[cpu->HL] + cpu->f.carry); cpu->pc += 1; break;
        // SBB A
        case 0x9f: cpu->A = arithmetix(cpu, SUB, cpu->A + cpu->f.carry); cpu->pc += 1; break;
        // ANA B
        case 0xa0: cpu->A = arithmetix(cpu, AND, cpu->B); cpu->f.carry = 0; cpu->pc += 1; break;
        // ANA C
        case 0xa1: cpu->A = arithmetix(cpu, AND, cpu->C); cpu->f.carry = 0; cpu->pc += 1; break;
        // ANA D
        case 0xa2: cpu->A = arithmetix(cpu, AND, cpu->D); cpu->f.carry = 0; cpu->pc += 1; break;
        // ANA E
        case 0xa3: cpu->A = arithmetix(cpu, AND, cpu->E); cpu->f.carry = 0; cpu->pc += 1; break;
        // ANA H
        case 0xa4: cpu->A = arithmetix(cpu, AND, cpu->H); cpu->f.carry = 0; cpu->pc += 1; break;
        // ANA L
        case 0xa5: cpu->A = arithmetix(cpu, AND, cpu->L); cpu->f.carry = 0; cpu->pc += 1; break;
        // ANA M
        case 0xa6: cpu->A = arithmetix(cpu, AND, cpu->mem[cpu->HL]); cpu->f.carry = 0; cpu->pc += 1; break;
        // ANA A
        case 0xa7: cpu->A = arithmetix(cpu, AND, cpu->A); cpu->f.carry = 0; cpu->pc += 1; break;
        // XRA B
        case 0xa8: cpu->A = arithmetix(cpu, XOR, cpu->B); reset_carries(cpu); cpu->pc += 1; break;
        // XRA C
        case 0xa9: cpu->A = arithmetix(cpu, XOR, cpu->C); reset_carries(cpu); cpu->pc += 1; break;
        // XRA D
        case 0xaa: cpu->A = arithmetix(cpu, XOR, cpu->D); reset_carries(cpu); cpu->pc += 1; break;
        // XRA E
        case 0xab: cpu->A = arithmetix(cpu, XOR, cpu->E); reset_carries(cpu); cpu->pc += 1; break;
        // XRA H
        case 0xac: cpu->A = arithmetix(cpu, XOR, cpu->H); reset_carries(cpu); cpu->pc += 1; break;
        // XRA L
        case 0xad: cpu->A = arithmetix(cpu, XOR, cpu->L); reset_carries(cpu); cpu->pc += 1; break;
        // XRA M
        case 0xae: cpu->A = arithmetix(cpu, XOR, cpu->mem[cpu->HL]); reset_carries(cpu); cpu->pc += 1; break;
        // XRA A
        case 0xaf: cpu->A = arithmetix(cpu, XOR, cpu->A); reset_carries(cpu); cpu->pc += 1; break;
        // ORA B
        case 0xb0: cpu->A = arithmetix(cpu, OR, cpu->B); reset_carries(cpu); cpu->pc += 1; break;
        // ORA C
        case 0xb1: cpu->A = arithmetix(cpu, OR, cpu->C); reset_carries(cpu); cpu->pc += 1; break;
        // ORA D
        case 0xb2: cpu->A = arithmetix(cpu, OR, cpu->D); reset_carries(cpu); cpu->pc += 1; break;
        // ORA E
        case 0xb3: cpu->A = arithmetix(cpu, OR, cpu->E); reset_carries(cpu); cpu->pc += 1; break;
        // ORA H
        case 0xb4: cpu->A = arithmetix(cpu, OR, cpu->H); reset_carries(cpu); cpu->pc += 1; break;
        // ORA L
        case 0xb5: cpu->A = arithmetix(cpu, OR, cpu->L); reset_carries(cpu); cpu->pc += 1; break;
        // ORA M
        case 0xb6: cpu->A = arithmetix(cpu, OR, cpu->mem[cpu->HL]); reset_carries(cpu); cpu->pc += 1; break;
        // ORA A
        case 0xb7: cpu->A = arithmetix(cpu, OR, cpu->A); reset_carries(cpu); cpu->pc += 1; break;
        // CMP B
        case 0xb8: arithmetix(cpu, SUB, cpu->B); cpu->pc += 1; break;
        // CMP C
        case 0xb9: arithmetix(cpu, SUB, cpu->C); cpu->pc += 1; break;
        // CMP D
        case 0xba: arithmetix(cpu, SUB, cpu->D); cpu->pc += 1; break;
        // CMP E
        case 0xbb: arithmetix(cpu, SUB, cpu->E); cpu->pc += 1; break;
        // CMP H
        case 0xbc: arithmetix(cpu, SUB, cpu->H); cpu->pc += 1; break;
        // CMP L
        case 0xbd: arithmetix(cpu, SUB, cpu->L); cpu->pc += 1; break;
        // CMP M
        case 0xbe: arithmetix(cpu, SUB, cpu->mem[cpu->HL]); cpu->pc += 1; break;
        // CMP A
        case 0xbf: arithmetix(cpu, SUB, cpu->A); cpu->pc += 1; break;
         // RNZ
        case 0xc0: {
            if (cpu->f.zero == 0) {
                ret(cpu);
            } else {
                cpu->pc += 1;
            }
            break;
        }
        // POP B
        case 0xc1: cpu->BC = pop(cpu); cpu->pc += 1; break;
        // JNZ $xxxx
        case 0xc2: {
            cpu->pc = cpu->f.zero == 0 ? (high << 8 | low) : cpu->pc + 3;
            break;
        }
        // JMP $xxxx
        case 0xc3: {
            cpu->pc = (high << 8) | low;
            if (cpu->pc == 0x0) {
                printf("WARN: exiting bc jmp 0x00..\n");
                cpu->exit = true;
            }
            break;
        }
        // CNZ $xxxx
        case 0xc4: {
            if (cpu->f.zero == 0) {
                call(cpu, high, low);
            } else {
                cpu->pc += 3;
            }
            break;
        }
        // PUSH B
        case 0xc5: push(cpu, cpu->BC); cpu->pc += 1; break;
        // ADI $xx
        case 0xc6: cpu->A = arithmetix(cpu, ADD, low); cpu->pc += 2; break;
        // RST 0
        case 0xc7: call(cpu, 0x00, 0x00); break;
        // RZ
        case 0xc8: {
            if (cpu->f.zero == 1) {
                ret(cpu);
            } else {
                cpu->pc += 1;
            }
            break;
        }
        // RET
        case 0xc9: {
            ret(cpu);
            break;
        }
        // JZ $xxxx
        case 0xca: cpu->pc = cpu->f.zero == 1 ? (high << 8) | low : cpu->pc + 3; break;
        // CZ $xxxx
        case 0xcc: {
            if (cpu->f.zero == 1) {
                call(cpu, high, low);
            } else {
                cpu->pc += 3;
            }
            break;
        }
        // CALL $xxxx
        case 0xcd: {
            call(cpu, high, low);
            break;
        }
        // ACI $xx
        case 0xce: cpu->A = arithmetix(cpu, ADD, low + cpu->f.carry); cpu->pc += 2; break;
        // RST 1
        case 0xcf: call(cpu, 0x00, 0x08); break;
        // RNC
        case 0xd0: {
            if (cpu->f.carry == 0) {
                ret(cpu);
            } else {
                cpu->pc += 1;
            }
            break;
        }
        // POP D
        case 0xd1: cpu->DE = pop(cpu); cpu->pc += 1; break;
        // JNC $xxxx
        case 0xd2: { 
            cpu->pc = cpu->f.carry == 0 ? (high << 8) | low : cpu->pc + 3;
            break; 
        }
        // OUT $xx
        case 0xd3: {
            assert(0);
            // todo("OUT");
            // cpu->pc += 2;
            break;
        }
        // CNC $xxxx
        case 0xd4: {
            if (cpu->f.carry == 0) {
                call(cpu, high, low);
            } else {
                cpu->pc += 3;
            }
            break;
        }
        // PUSH D
        case 0xd5: push(cpu, cpu->DE); cpu->pc += 1; break;
        // SUI $xx
        case 0xd6: cpu->A = arithmetix(cpu, SUB, low); cpu->pc += 2; break;
        // RST 2
        case 0xd7: call(cpu, 0x00, 0x10); break;
        // RC
        case 0xd8: {
            if (cpu->f.carry == 1) {
                ret(cpu);
            } else {
                cpu->pc += 1;
            }
            break;
        }
        // JC $xxxx
        case 0xda: { 
            cpu->pc = cpu->f.carry == 1 ? (high << 8) | low : cpu->pc + 3;
            break; 
        }
        // IN $xx
        case 0xdb: {
            assert(0);
            break;
        }
        // CC $xxxc (call if carry)
        case 0xdc: {
            if (cpu->f.carry == 1) {
                call(cpu, high, low);
            } else {
                cpu->pc += 3;
            }
            break;
        }
        // SBI $xx
        case 0xde: cpu->A = arithmetix(cpu, SUB, low + cpu->f.carry); cpu->pc += 2; break;
        // RST 3
        case 0xdf: call(cpu, 0x00, 0x18); break;
        // RPO
        case 0xe0: {
            if (cpu->f.parity == 0) {
                ret(cpu);
            } else {
                cpu->pc += 1;
            }
            break;
        }
        // POP H
        case 0xe1: cpu->HL = pop(cpu); cpu->pc += 1; break;
        // JPO $xxxx
        case 0xe2: cpu->pc = cpu->f.parity == 0 ? (high << 8) | low : cpu->pc + 3; break;
        // XTHL
        case 0xe3: { // TODO: unsure af
            const uint16_t tmp = pop(cpu);
            push(cpu, cpu->HL);
            cpu->HL = tmp;
            cpu->pc += 1;
            break;
        }
        // CPO $xxxx
        case 0xe4: {
            if (cpu->f.parity == 0) {
                call(cpu, high, low);
            } else {
                cpu->pc += 3;
            }
            break;
        }
        // PUSH H
        case 0xe5: push(cpu, cpu->HL); cpu->pc += 1; break;
        // ANI $xx
        case 0xe6: cpu->A = arithmetix(cpu, AND, low); cpu->f.carry = 0; cpu->pc += 2; break;
        // RPE
        case 0xe8: {
            if (cpu->f.parity == 1) {
                ret(cpu);
            } else {
                cpu->pc += 1;
            }
            break;
        }
        // RST 4
        case 0xe7: call(cpu, 0x00, 0x20); break;
        // PCHL
        case 0xe9: cpu->pc = (cpu->H << 8) | cpu->L; break;
        // JPE $xxx
        case 0xea: cpu->pc = cpu->f.parity == 1 ? (high << 8) | low : cpu->pc + 3; break;
        // XCHG
        case 0xeb: {
            const uint16_t tmp = cpu->HL;
            cpu->HL = cpu->DE;
            cpu->DE = tmp;
            cpu->pc += 1;
            break;
        }
        // CPE $xxxx
        case 0xec: {
             if (cpu->f.parity == 1) {
                call(cpu, high, low);
            } else {
                cpu->pc += 3;
            }
            break;
        }
        // XRI $xx
        case 0xee: cpu->A = arithmetix(cpu, XOR, low); reset_carries(cpu); cpu->pc += 2; break;
        // RST 5
        case 0xef: call(cpu, 0x00, 0x28); break;
        // RP
        case 0xf0: {
            if (cpu->f.sign == 0) {
                ret(cpu);
            } else {
                cpu->pc += 1;
            }
            break;
        }
        // POP PSW
        case 0xf1: {
            const uint16_t psw = pop(cpu);
            cpu->A = psw >> 8;
            memset(&cpu->f, psw & 0x00ff, sizeof(flags));
            cpu->pc += 1;
            break;
        }
        // JP $xxxx
        case 0xf2: cpu->pc = cpu->f.sign == 0 ? (high << 8) | low : cpu->pc + 3; break;
        // DI
        case 0xf3: cpu->interrupts_disabled = true; cpu->pc += 1; break;
        // CP $xxxx
        case 0xf4: {
             if (cpu->f.sign == 0) {
                call(cpu, high, low);
            } else {
                cpu->pc += 3;
            }
            break;
        }
        // PUSH PSW
        case 0xf5: {
            const uint8_t *flags = (uint8_t*) &cpu->f;
            const uint16_t psw = (cpu->A << 8) | *flags;
            push(cpu, psw);
            cpu->pc += 1;
            break;
        }
        // ORI $xx
        case 0xf6: cpu->A = arithmetix(cpu, OR, low); reset_carries(cpu); cpu->pc += 2; break;
        // RM
        case 0xf8: {
            if (cpu->f.sign == 1) {
                ret(cpu);
            } else {
                cpu->pc += 1;
            }
            break;
        }
        // RST 6
        case 0xf7: call(cpu, 0x00, 0x30); break;
        // SPHL
        case 0xf9: cpu->sp = (cpu->H << 8) | cpu->L; cpu->pc += 1; break;
        // JM $xxxx
        case 0xfa: cpu->pc = cpu->f.sign == 1 ? (high << 8) | low : cpu->pc + 3; break;
        // EI, enable interrupts
        case 0xfb: cpu->interrupts_disabled = false; cpu->pc += 1; break;
        // CM $xxxx
        case 0xfc: {
            if (cpu->f.sign == 1) {
                call(cpu, high, low);
            } else {
                cpu->pc += 3;
            }
            break;
        }
        // CPI $xx
        case 0xfe: {
            // "The comparison is performed by internally subtract- ing the data from the accumulator using two's complement arithmetic, leaving the accumulator unchanged but setting the condition bits by the result."
            arithmetix(cpu, SUB, low);
            // printf("%x - %x = %x\n", cpu->A, low, res);
            cpu->pc += 2;
            break;
        }
        // RST 7
        case 0xff: call(cpu, 0x00, 0x38); break;
        default: cpu->exit = true; // TODO: remove
    }
}