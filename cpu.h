#ifndef cpu_h
#define cpu_h

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// Register Pairs:
// B = B and C (0 and 1)
// D = D and E (2 and 3)
// H = H and L (4 and 5)
// PSW (program status word) =  A (7) and "a special byte witch flags"

//  For almost all 8080 instructions, the H and L registers must be used. 
// The H register contains the most significant 8 bits of the referenced address, 
// and the L register contains the least significant 8 bits.

// 8080 and all it's successors were little-endian machines, 
// concerning the byte-order of 16-bit words in memory.

typedef struct {
    uint8_t carry:1; // CY
    uint8_t pad1:1;
    uint8_t parity:1;
    uint8_t pad3:1;
    uint8_t auxcarry:1; // AC  
    uint8_t pad5:1;
    uint8_t zero:1;
    uint8_t sign:1;
    // uint8_t pad:3; // to make this struct 8bit
} flags;

typedef struct {
    uint8_t mem[0xffff]; // 35536 bytes
    flags f;
    uint8_t A; // accumulator
    
    union {
        struct {
            uint8_t C;
            uint8_t B;
        };
        uint16_t BC;
    };

    union {
        struct {
            uint8_t E;
            uint8_t D;
        };
        uint16_t DE;
    };

    union {
        struct {
            uint8_t L;
            uint8_t H;
        };
        uint16_t HL; // AKA M (memory)
    };
    

    uint16_t sp;
    uint16_t pc;
    bool interrupts_disabled;
    bool exit;
} CPU;


CPU* init(const uint16_t base_addr);
void load(CPU* cpu, const uint16_t base_addr, const uint8_t *program, size_t size);
void exec(CPU* cpu);
void handle_interrupt(CPU* cpu, uint8_t interrupt);

#endif
