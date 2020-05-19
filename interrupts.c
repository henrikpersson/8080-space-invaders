#include <stdio.h>
#include <sys/time.h>
#include "interrupts.h"

size_t last_mid_ts = 0;
size_t last_end_ts = 0;

size_t num_active_interrupts = 0;
uint16_t interrupt_rets[10];

void inject_interrupt(CPU* cpu, uint8_t interrupt) {
    // if(num_active_interrupts > 0) {
    //     printf("WARNING!!!! IGNORING INTERRUPT. ACTIVE INTERRUPTS: %zu\n", num_active_interrupts);
    //     // cpu->exit = true;
    //     return;
    // }

    // interrupt_rets[num_active_interrupts] = cpu->pc;
    // num_active_interrupts++;
    
    // push pc
    cpu->sp--;
    cpu->mem[cpu->sp] = cpu->pc >> 8;
    cpu->sp--;
    cpu->mem[cpu->sp] = cpu->pc & 0x00ff;

    // jmp interrupt
    cpu->pc = interrupt;
}

bool interrupt_flag;

void interrupt(CPU* cpu, double fps) {
    if (cpu->interrupts_disabled) {
        // printf("interrups disabled\n");
        return;
    }
    cpu->interrupts_disabled = true;

    // RST 8 when the beam is *near* the middle of the screen and RST 10 

    // TODO: DO WE NEED AN INTERRUPT QUEUE?????

    
    if (!interrupt_flag) {
        // RST 1
        // printf("MID FRAME INTERRUPT %zu\n", num_active_interrupts);
        inject_interrupt(cpu, 0x08);
    } else {
         // RST 2
        // printf("END FRAME INTERRUPT %zu\n", num_active_interrupts);
        inject_interrupt(cpu, 0x10);
    }

    interrupt_flag = !interrupt_flag;
}

void check_if_ret_from_interrupt(uint16_t retaddr) {
    // if (num_active_interrupts > 0 && retaddr == interrupt_rets[num_active_interrupts - 1]) {
    //     num_active_interrupts--;
    // }
}