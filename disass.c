#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "disass.h"

// TODO: We are currently reading 1 byte at a time. This is good because we kinda
// doesn't have to care about endianness. But it would be interesting to figure out 
// how to read 2 bytes (16bits) at a time and at the same time keep endian portabilltiy.
// Boost as something for this in c++ i think

#define D8  "%02x"
#define D16 "%02x%02x"

#define p(s) sprintf(mnem, s)
#define pp(s, ...) sprintf(mnem, s, __VA_ARGS__); opsize = 2;
#define ppp(s, ...) sprintf(mnem, s, __VA_ARGS__); opsize = 3;

void disass(char *output, const uint8_t *mem, const int pc) {
    const uint8_t *opcode = &mem[pc];
    const uint8_t h = opcode[2];
    const uint8_t l = opcode[1];

    char mnem[DISASS_OP_SIZE];
    int opsize = 1;
    switch (*opcode) {
        case 0x00: p("NOP"); break;
        // LXI = Load register pair immediate
        case 0x01: ppp("LXI B, $#" D16, h, l); break;
        case 0x02: p("STAX B"); break;
        case 0x03: p("INX B"); break;
        case 0x04: p("INR B"); break;
        case 0x05: p("DCR B"); break;
        case 0x06: pp("MVI B, $#" D8, l); break;
        // Rotate accumulator left
        case 0x07: p("RLC"); break;
        case 0x09: p("DAD B"); break;
        case 0x0a: p("LDAX B"); break;
        case 0x0b: p("DCX B"); break;
        case 0x0c: p("INR C"); break;
        case 0x0d: p("DCR C"); break;
        case 0x0e: pp("MVI C, $#" D8, l); break;
        // Rotate accumulator right
        case 0x0f: p("RRC"); break;
        case 0x11: ppp("LXI D, $#" D16, h, l); break;
        // STAX = Store Accumulator indirect
        case 0x12: p("STAX D"); break;
        case 0x13: p("INX D"); break;
        case 0x14: p("INR D"); break;
        case 0x15: p("DCR D"); break;
        case 0x16: pp("MVI D, $#" D8, l); break;
        // Rotate left through carry
        case 0x17: p("RAL"); break;
        case 0x19: p("DAD D"); break;
        // LDAX = Load acc indirect
        case 0x1a: p("LDAX D"); break;
        case 0x1b: p("DCX D"); break;
        case 0x1c: p("INR E"); break;
        case 0x1d: p("DCR E"); break;
        case 0x1e: pp("MVI E, $#" D8, l); break;
        // Rotate right through carry
        case 0x1f: p("RAR"); break;
        case 0x21: ppp("LXI H, $#" D16, h, l); break;
        case 0x22: ppp("SHLD $" D16, h, l); break;
        case 0x23: p("INX H"); break;
        case 0x24: p("INR H"); break;
        case 0x25: p("DCR H"); break;
        case 0x26: pp("MVI H, $#" D8, l); break;
        case 0x27: p("DAA"); break;
        case 0x29: p("DAD H"); break;
        case 0x2a: ppp("LHLD $" D16, h, l); break;
        case 0x2b: p("DCX H"); break;
        case 0x2c: p("INR L"); break;
        case 0x2d: p("DCR L"); break;
        case 0x2e: pp("MVI L, $#" D8, l); break;
        case 0x2f: p("CMA"); break;
        case 0x31: ppp("LXI SP, $#" D16, h, l); break;
        // Store Accumulator Direct
        case 0x32: ppp("STA $" D16, h, l); break;
        case 0x33: p("INX SP"); break;
        case 0x34: p("INR M"); break;
        case 0x35: p("DCR M"); break;
        case 0x36: pp("MVI M, $#" D8, l); break;
        case 0x37: p("STC"); break;
        case 0x39: p("DAD SP"); break;
        // Load acc direct
        case 0x3a: ppp("LDA $" D16, h, l); break;
        case 0x3b: p("DCX SP"); break;
        case 0x3c: p("INR A"); break;
        case 0x3d: p("DCR A"); break;
        case 0x3e: pp("MVI A, $#" D8, l); break;
        case 0x3f: p("CMC"); break;
        case 0x40: p("MOV B,B"); break;
        case 0x41: p("MOV B,C"); break;
        case 0x42: p("MOV B,D"); break;
        case 0x43: p("MOV B,E"); break;
        case 0x44: p("MOV B,H"); break;
        case 0x45: p("MOV B,L"); break;
        case 0x46: p("MOV B,M"); break;
        case 0x47: p("MOV B,A"); break;
        case 0x48: p("MOV C,B"); break;
        case 0x49: p("MOV C,C"); break;
        case 0x4a: p("MOV C,D"); break;
        case 0x4b: p("MOV C,E"); break;
        case 0x4c: p("MOV C,H"); break;
        case 0x4d: p("MOV C,L"); break;
        case 0x4e: p("MOV C,M"); break;
        case 0x4f: p("MOV C,A"); break;
        case 0x50: p("MOV D,B"); break;
        case 0x51: p("MOV D,C"); break;
        case 0x52: p("MOV D,D"); break;
        case 0x53: p("MOV D,E"); break;
        case 0x54: p("MOV D,H"); break;
        case 0x55: p("MOV D,L"); break;
        case 0x56: p("MOV D,M"); break;
        case 0x57: p("MOV D,A"); break;
        case 0x58: p("MOV E,B"); break;
        case 0x59: p("MOV E,C"); break;
        case 0x5a: p("MOV E,D"); break;
        case 0x5b: p("MOV E,E"); break;
        case 0x5c: p("MOV E,H"); break;
        case 0x5d: p("MOV E,L"); break;
        case 0x5e: p("MOV E,M"); break;
        case 0x5f: p("MOV E,A"); break;
        case 0x60: p("MOV H,B"); break;
        case 0x61: p("MOV H,C"); break;
        case 0x62: p("MOV H,D"); break;
        case 0x63: p("MOV H,E"); break;
        case 0x64: p("MOV H,H"); break;
        case 0x65: p("MOV H,L"); break;
        case 0x66: p("MOV H,M"); break;
        case 0x67: p("MOV H,A"); break;
        case 0x68: p("MOV L,B"); break;
        case 0x69: p("MOV L,C"); break;
        case 0x6a: p("MOV L,D"); break;
        case 0x6b: p("MOV L,E"); break;
        case 0x6c: p("MOV L,H"); break;
        case 0x6d: p("MOV L,L"); break;
        case 0x6e: p("MOV L,M"); break;
        case 0x6f: p("MOV L,A"); break;
        case 0x70: p("MOV M,B"); break;
        case 0x71: p("MOV M,C"); break;
        case 0x72: p("MOV M,D"); break;
        case 0x73: p("MOV M,E"); break;
        case 0x74: p("MOV M,H"); break;
        case 0x75: p("MOV M,L"); break;
        case 0x76: p("HLT"); break;
        case 0x77: p("MOV M,A"); break;
        case 0x78: p("MOV A,B"); break;
        case 0x79: p("MOV A,C"); break;
        case 0x7a: p("MOV A,D"); break;
        case 0x7b: p("MOV A,E"); break;
        case 0x7c: p("MOV A,H"); break;
        case 0x7d: p("MOV A,L"); break;
        case 0x7e: p("MOV A,M"); break;
        case 0x7f: p("MOV A,A"); break;
        case 0x80: p("ADD B"); break;
        case 0x81: p("ADD C"); break;
        case 0x82: p("ADD D"); break;
        case 0x83: p("ADD E"); break;
        case 0x84: p("ADD H"); break;
        case 0x85: p("ADD L"); break;
        case 0x86: p("ADD M"); break;
        case 0x87: p("ADD A"); break;
        case 0x88: p("ADC B"); break;
        case 0x89: p("ADC C"); break;
        case 0x8a: p("ADC D"); break;
        case 0x8b: p("ADC E"); break;
        case 0x8c: p("ADC H"); break;
        case 0x8d: p("ADC L"); break;
        case 0x8e: p("ADC M"); break;
        case 0x8f: p("ADC A"); break;
        case 0x90: p("SUB B"); break;
        case 0x91: p("SUB C"); break;
        case 0x92: p("SUB D"); break;
        case 0x93: p("SUB E"); break;
        case 0x94: p("SUB H"); break;
        case 0x95: p("SUB L"); break;
        case 0x96: p("SUB M"); break;
        case 0x97: p("SUB A"); break;
        case 0x98: p("SBB B"); break;
        case 0x99: p("SBB C"); break;
        case 0x9a: p("SBB D"); break;
        case 0x9b: p("SBB E"); break;
        case 0x9c: p("SBB H"); break;
        case 0x9d: p("SBB L"); break;
        case 0x9e: p("SBB M"); break;
        case 0x9f: p("SBB A"); break;
        case 0xa0: p("ANA B"); break;
        case 0xa1: p("ANA C"); break;
        case 0xa2: p("ANA D"); break;
        case 0xa3: p("ANA E"); break;
        case 0xa4: p("ANA H"); break;
        case 0xa5: p("ANA L"); break;
        case 0xa6: p("ANA M"); break;
        case 0xa7: p("ANA A"); break;
        case 0xa8: p("XRA B"); break;
        case 0xa9: p("XRA C"); break;
        case 0xaa: p("XRA D"); break;
        case 0xab: p("XRA E"); break;
        case 0xac: p("XRA H"); break;
        case 0xad: p("XRA L"); break;
        case 0xae: p("XRA M"); break;
        case 0xaf: p("XRA A"); break;
        case 0xb0: p("ORA B"); break;
        case 0xb1: p("ORA C"); break;
        case 0xb2: p("ORA D"); break;
        case 0xb3: p("ORA E"); break;
        case 0xb4: p("ORA H"); break;
        case 0xb5: p("ORA L"); break;
        case 0xb6: p("ORA M"); break;
        case 0xb7: p("ORA A"); break;
        case 0xb8: p("CMP B"); break;
        case 0xb9: p("CMP C"); break;
        case 0xba: p("CMP D"); break;
        case 0xbb: p("CMP E"); break;
        case 0xbc: p("CMP H"); break;
        case 0xbd: p("CMP L"); break;
        case 0xbe: p("CMP M"); break;
        case 0xbf: p("CMP A"); break;
        case 0xc0: p("RNZ"); break;
        case 0xc1: p("POP B"); break;
        case 0xc2: ppp("JNZ $" D16, h, l); break;
        case 0xc3: ppp("JMP $" D16, h, l); break;
        // Call if not zero
        case 0xc4: ppp("CNZ $" D16, h, l); break;
        case 0xc5: p("PUSH B"); break;
        // ADD immediate
        case 0xc6: pp("ADI $#" D8, l); break;
        case 0xc7: p("RST 0"); break;
        case 0xc8: p("RZ"); break;
        case 0xc9: p("RET"); break;
        case 0xca: ppp("JZ $#" D16, h, l); break;
        case 0xcc: ppp("CZ $" D16, h, l); break;
        case 0xcd: ppp("CALL $" D16, h, l); break;
        // ADD immediate w carry
        case 0xce: pp("ACI $#" D8, l); break;
        case 0xcf: p("RST 1"); break;
        case 0xd0: p("RNC"); break;
        case 0xd1: p("POP D"); break;
        case 0xd2: ppp("JNC $#" D16, h, l); break;
        case 0xd3: pp("OUT %s", "D8"); break;
        case 0xd4: ppp("CNC $" D16, h, l); break;
        case 0xd5: p("PUSH D"); break;
        // SUB immediate
        case 0xd6: pp("SUI $#" D8, l); break;
        case 0xd7: p("RST 2"); break;
        case 0xd8: p("RC"); break;
        case 0xda: ppp("JC $#" D16, h, l); break;
        case 0xdb: pp("IN $" D8, l); break;
        // Call if carry
        case 0xdc: ppp("CC $#" D16, h, l); break;
        case 0xde: pp("SBI $#" D8, l); break;
        case 0xdf: p("RST 3"); break;
        case 0xe0: p("RPO"); break;
        case 0xe1: p("POP H"); break;
        case 0xe2: ppp("JPO $#" D16, h, l); break;
        // Exchange top of stack with HL
        case 0xe3: p("XTHL"); break;
        // Call if parity odd
        case 0xe4: ppp("CPO $" D16, h, l); break;
        case 0xe5: p("PUSH H"); break;
        // And immediate w acc
        case 0xe6: pp("ANI $#" D8, l); break;
        case 0xe7: p("RST 4"); break;
        case 0xe8: p("RPE"); break;
        case 0xe9: p("PCHL"); break;
        // Jump if parity even
        case 0xea: ppp("JPE $#" D16, h, l); break;
        case 0xeb: p("XCHG"); break;
        case 0xec: ppp("CPE $" D16, h, l); break;
        case 0xee: pp("XRI $#" D8, l); break;
        case 0xef: p("RST 5"); break;
        case 0xf0: p("RP"); break;
        case 0xf1: p("POP PSW"); break;
        case 0xf2: ppp("JP $#" D16, h, l); break;
        // Disable interrupts
        case 0xf3: p("DI"); break;
        case 0xf4: ppp("CP $#" D16, h, l); break;
        case 0xf5: p("PUSH PSW"); break;
        case 0xf6: pp("ORI $#" D8, l); break;
        case 0xf7: p("RST 6"); break;
        case 0xf8: p("RM"); break;
        case 0xf9: p("SPHL"); break;
        // Jump if minus
        case 0xfa: ppp("JM $#" D16, h, l); break;
        // Enable interrupts
        case 0xfb: p("EI"); break;
        // Call if minus
        case 0xfc: ppp("CM $#" D16, h, l); break;
        // Compare immediate
        case 0xfe: pp("CPI $#" D8, l); break;
        case 0xff: p("RST 7"); break;
        default: p("???"); break;
    }

    char raw_header[DISASS_OP_SIZE];
    switch(opsize) {
        case 1: sprintf(raw_header, "%04x %02x    \t", pc, opcode[0]); break;
        case 2: sprintf(raw_header, "%04x %02x %02x  \t", pc, opcode[0], opcode[1]); break;
        case 3: sprintf(raw_header, "%04x %02x %02x %02x\t", pc, opcode[0], opcode[1], opcode[2]); break;
    }

    strcat(raw_header, mnem);
    memcpy(output, raw_header, DISASS_OP_SIZE);
}
