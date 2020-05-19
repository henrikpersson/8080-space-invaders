#include <SDL2/SDL.h>
#include "gfx.h"

#define SCREEN_WIDTH 224
#define SCREEN_HEIGHT 256
#define SCREEN_PIXELS SCREEN_HEIGHT * SCREEN_WIDTH

SDL_Window* window;
SDL_Surface *window_surface;
SDL_Renderer* renderer;
SDL_Texture* texture;

#define SDL_BYTES_PER_PIXEL 4 // ARGB8888, todo: can be 3 if using mode RGB24?
int pixels[SCREEN_WIDTH * SCREEN_HEIGHT * SDL_BYTES_PER_PIXEL];

void init_sdl(char *title) {
    SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    window_surface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
}

void render_sdl(uint8_t *buffer) {
    // TODO: is it faster to just stack alloc pixels on the stack here?
    memset(pixels, 0, SCREEN_PIXELS * SDL_BYTES_PER_PIXEL);

    int vram_index = 0;
   for(int columns = 0; columns < SCREEN_WIDTH; columns++){
        for(int row = SCREEN_HEIGHT; row > 0; row -=8){
            for(int j = 0; j < 8; j++){
                
                int idx = (row - j) * SCREEN_WIDTH + columns;
                int res = buffer[vram_index] & 1 << j; // TODO unsure
                
                if(res){
                    pixels[idx] = 0xFFFFFF;             
                } else {
                    pixels[idx] = 0x000000;
                }
            }
            vram_index++;
        }
    }

    SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(Uint32));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_UpdateWindowSurface(window);
}

void destroy_sdl() {
    SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
