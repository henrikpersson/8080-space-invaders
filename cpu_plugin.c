#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "cpu_plugin.h"
#include "io.h"
#include "interrupts.h"

#define CPM_OUT 0

uint8_t shift0;
uint8_t shift1;
uint8_t shift_offset;

bool patch_cp_m_os_call(CPU* cpu, const uint8_t hi, const uint8_t lo) {
    if (((hi << 8) | lo) == 0x0005) { // BDOS
        size_t len = strlen(emu_cp_m_os_output);
        (void)len;
        if (cpu->C == 0x0009) { // MSG
            for (uint16_t i = cpu->DE; cpu->mem[i] != '$'; i++) {
                if (CPM_OUT) {
                    putchar(cpu->mem[i]);
                } else {
                    emu_cp_m_os_output[len++] = cpu->mem[i];
                }
            }
        }  else if (cpu->C == 0x0002) { // PCHAR
            if (CPM_OUT) {
                putchar((char)cpu->E);
            } else {
                emu_cp_m_os_output[len] = cpu->E;
            }
        }
        cpu->pc += 3;
        return true;
    }

    return false;
}

bool cpu_plugin_op(CPU* cpu, const uint8_t op, const uint8_t hi, const uint8_t lo) {
    switch (op) {
        // CALL (CP/M OS)
        case 0xcd: return patch_cp_m_os_call(cpu, hi, lo);
        // IN
        case 0xdb: {
            const uint8_t port = lo;
            uint8_t res = 0;
            switch(port) {
                case 0: res = 1; break;
                case 1: res = io_ports[1]; break;
                case 2: res = 0; break;
                case 3: {
                    uint16_t v = (shift1 << 8) | shift0;
                    res = ((v >> (8 - shift_offset)) & 0xff);
                    break;
                }
            }
            cpu->A = res;
            cpu->pc += 2;
            return true;
        }
        // OUT
        case 0xd3: {
            const uint8_t port = lo;
            switch(port) {
                case 2: shift_offset = cpu->A & 0x7; break;
                case 3: break; // sound related
                case 4: shift0 = shift1; shift1 = cpu->A; break;
                case 5: break; // sound related
                case 6: break; // strange 'debug' port?
                default: assert(0);
            }
            io_ports[port] = cpu->A;
            cpu->pc += 2;
            return true;
        }
        default: return false;
    }
}

void cpu_plugin_ret(uint16_t retaddr) {
    check_if_ret_from_interrupt(retaddr);
}