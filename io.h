#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "cpu.h"

extern uint8_t io_ports[8];

bool handle_user_input(CPU *cpu);