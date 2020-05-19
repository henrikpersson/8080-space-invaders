#include <stdbool.h>
#include "cpu.h"

// for diag roms originally intended for CP/M OS.
// patches jmp calls to print routines etc...
// TODO: emu to whole CP/M OS???
bool emu_cp_m_os;
char emu_cp_m_os_output[1024];

bool cpu_plugin_op(CPU* cpu, const uint8_t op, const uint8_t hi, const uint8_t lo);
void cpu_plugin_ret(uint16_t retaddr);