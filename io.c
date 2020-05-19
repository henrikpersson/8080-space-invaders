#include "io.h"

/* 
Ports:    
    Read 1    
    BIT 0   coin (0 when active)    
        1   P2 start button    
        2   P1 start button    
        3   ?    
        4   P1 shoot button    
        5   P1 joystick left    
        6   P1 joystick right    
        7   ?

    Read 2    
    BIT 0,1 dipswitch number of lives (0:3,1:4,2:5,3:6)    
        2   tilt 'button'    
        3   dipswitch bonus life at 1:1000,0:1500    
        4   P2 shoot button    
        5   P2 joystick left    
        6   P2 joystick right    
        7   dipswitch coin info 1:off,0:on    

    Read 3      shift register result    

    Write 2     shift register result offset (bits 0,1,2)    
    Write 3     sound related    
    Write 4     fill shift register    
    Write 5     sound related    
    Write 6     strange 'debug' port? eg. it writes to this port when    
            it writes text to the screen (0=a,1=b,2=c, etc)    

    (write ports 3,5,6 can be left unemulated, read port 1=$01 and 2=$00    
    will make the game run, but but only in attract mode)    */

#define BIT_0 0x01
#define BIT_1 0x02
#define BIT_2 0x04
#define BIT_3 0x08
#define BIT_4 0x10
#define BIT_5 0x20
#define BIT_6 0x40
#define BIT_7 0x80

#define COIN        BIT_0
#define P1_START    BIT_2
#define P1_SHOOT    BIT_4
#define P1_LEFT     BIT_5
#define P1_RIGHT    BIT_6

#define TILT        BIT_2

uint8_t io_ports[8]; // TODO.. we only need 2x uints8's
#define PORT1 io_ports[1]
#define PORT2 io_ports[2]

SDL_Event event;
bool handle_user_input(CPU *cpu) {

    // printf("port 1 = 0x%02x\n", PORT1);

    bool user_exit = false;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            user_exit = true;
        }

        switch (event.type) {
            case SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                    case SDLK_ESCAPE: user_exit = true; break;
                    case SDLK_c: PORT1 |= COIN; break;
                    case SDLK_KP_ENTER: printf("START GAME\n"); PORT1 |= P1_START; break;
                    case SDLK_SPACE: PORT1 |= P1_SHOOT; break;
                    case SDLK_LEFT: PORT1 |= P1_LEFT; break;
                    case SDLK_RIGHT: PORT1 |= P1_RIGHT; break;
                    case SDLK_UP: PORT2 |= TILT; break;
                }
            } break;
            case SDL_KEYUP: {
                switch (event.key.keysym.sym) {
                    case SDLK_c: PORT1 ^= COIN; break;
                    case SDLK_KP_ENTER: PORT1 ^= P1_START; break;
                    case SDLK_SPACE: PORT1 ^= P1_SHOOT; break;
                    case SDLK_LEFT: PORT1 ^= P1_LEFT; break;
                    case SDLK_RIGHT: PORT1 ^= P1_RIGHT; break;
                    case SDLK_UP: PORT2 ^= TILT; break;
                }
            } break;
            case SDL_QUIT: user_exit = true; break;
        }
    }
    return user_exit;
}

