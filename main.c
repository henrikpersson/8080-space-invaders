#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "cpu.h"
#include "cpu_plugin.h"
#include "interrupts.h"
#include "gfx.h"
#include "io.h"

#include "disass.h"

#define DEBUG 0
#define TRACE 0

#define ENABLE_INTERRUPTS

#define TICKS_PER_FRAME 1500 // how many?? todo: get rid of this one and do as many as we can in a frame!
#define FRAMES_PER_SECOND 60 // 0.1 = 10 sec per frame

#define ONE_SECOND_IN_MICRO 1000000
#define FRAMES_PER_MICROSECOND ONE_SECOND_IN_MICRO / FRAMES_PER_SECOND

uint64_t gettimestamp_micro() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t) (1000000 * tv.tv_sec) + tv.tv_usec;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("usage: %s [rom] [$base_addr] [emu_cpm_os:1|0]", argv[0]);
        exit(1);
    }

    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) {
        printf("fopen");
        exit(1);
    }

    uint16_t base_addr = strtol(argv[2], NULL, 16);
    emu_cp_m_os = atoi(argv[3]);
    printf("rom: %s, base_addr: 0x%04x (%dd), emu_cp_m_os: %d\n", argv[1], base_addr, base_addr, emu_cp_m_os);

    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    CPU *cpu = init(base_addr);

    uint8_t program[fsize];
    fread(program, fsize, 1, f);
    load(cpu, base_addr, program, fsize);
    fclose(f);

    init_sdl(argv[1]);

    uint64_t totalticks = 0;
    bool user_exit = false;
    while(!cpu->exit && !user_exit) { 
        uint64_t frame_start_ts = gettimestamp_micro();

        user_exit = handle_user_input(cpu);

        #ifdef ENABLE_INTERRUPTS
            interrupt(cpu, FRAMES_PER_SECOND); // TODO: is this the place to interrupt???? 
        #endif

        char curr_op_dissasd[128];
        for (int tick = 0; tick < TICKS_PER_FRAME; tick++) {
            if (cpu->exit || user_exit) break;

            if (DEBUG) {
                // print interrupt indicator
                for (size_t i = 0; i < num_active_interrupts; i++) {
                    printf("-");
                }
                if (num_active_interrupts > 0) {
                    printf(">");
                }

                disass(curr_op_dissasd, cpu->mem, cpu->pc);
                printf("%s\n", curr_op_dissasd);
            }

            if (TRACE) {
                printf("0x%04x -> ", cpu->pc);
            }

            // tick
            exec(cpu);
            totalticks++;

            if (TRACE) {
                // const uint8_t *flags = (uint8_t*) &cpu->f;
                printf("0x%02x\n", cpu->A);
                printf("F = c:%x, p:%x, ac:%x, z:%x, s:%x\n", cpu->f.carry, cpu->f.parity, cpu->f.auxcarry, cpu->f.zero, cpu->f.sign);
                // printf("stack: ");
                // uint8_t *s = &cpu->mem[cpu->sp];
                // while (*s != '\0') printf("0x%02x ", *s++); // TODO: wrong? what if stack contains a 0x00??? ITS VALID!
                // printf("\n");
            }
        }

        #ifdef ENABLE_INTERRUPTS
            interrupt(cpu, FRAMES_PER_SECOND); // TODO: is this the place to interrupt???? 
        #endif

        render_sdl(&cpu->mem[0x2400]);

        uint64_t frame_end_ts = gettimestamp_micro();
        uint64_t elapsed = frame_end_ts - frame_start_ts;
        // printf("frame end: %llu\n", frame_end_ts);
        // printf("frame took: %llu\n", elapsed);
        if (elapsed < FRAMES_PER_MICROSECOND) {
            SDL_Delay((FRAMES_PER_MICROSECOND - elapsed) / 1000);
        }
    }

    if (emu_cp_m_os) {
        printf("CP/M OUT: %s\n", emu_cp_m_os_output);
    }

    // free(cpu->memory);
    printf("cleaning up! total ticks: %llu\n", totalticks);
    destroy_sdl();
    free(cpu);

    return 0;
}
