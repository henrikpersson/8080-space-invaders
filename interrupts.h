#include "cpu.h"

extern size_t num_active_interrupts;
extern uint16_t interrupt_rets[10];

void interrupt(CPU* cpu, double fps);
void check_if_ret_from_interrupt(uint16_t retaddr);