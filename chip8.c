#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "SDL.h"

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
} sdl_t;

typedef struct
{
    uint32_t window_width;  // SDL window width
    uint32_t window_height; // SDL window height
    uint32_t fg_color;      // Foreground color RRGGBBAA
    uint32_t bg_color;      // Background color RRGGBBAA
    uint32_t scale_factor;  // Amount to scale a CHIP8 pixel by e.g. 20x will be a 20x larger window
} config_t;

typedef enum
{
    QUIT,
    RUNNING,
    PAUSED,
} emu_state_t;

typedef struct
{
    emu_state_t state;
} chip8_t;

bool init_sdl(sdl_t *sdl, const config_t config)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0)
    {
        SDL_Log("Unable to initialize SDL: %s\n", SDL_GetError());
        return false;
    }
    
    sdl->window = SDL_CreateWindow("CHIP8 Emulator", SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED, 
                                    config.window_width * config.scale_factor,
                                    config.window_height * config.scale_factor,
                                    0);
    
    if (!sdl->window)
    {
        SDL_Log("Unable to create SDL window: %s\n", SDL_GetError());
        return false;
    }
    
    sdl->renderer = SDL_CreateRenderer(sdl->window, -1, SDL_RENDERER_ACCELERATED);
    
    if (!sdl->renderer)
    {
        SDL_Log("Unable to create SDL renderer: %s", SDL_GetError());
        return false;
    }
    
    return true;
}

bool set_config_from_args(config_t *config, const int argc, char *argv[])
{
    *config = (config_t)
    {
        .window_width  = 64,    // CHIP8 original X resolution
        .window_height = 32,    // CHIP8 original Y resolution
        .fg_color = 0xFFFFFFFF, // WHITE
        .bg_color = 0xFFFF00FF, // YELLOW
        .scale_factor = 20,     // Default resolution will be 1280x640
    };
    
    for (int i = 1; i < argc; i++)
    {
        (void)argv[i]; //Prevent compiler error from unused variables argc/argv
        // ...
    }
    
    return true;
}

bool init_chip8(chip8_t *chip8)
{
    chip8->state = RUNNING;
    return true;
}

void final_cleanup(const sdl_t sdl)
{
    SDL_DestroyRenderer(sdl.renderer);
    SDL_DestroyWindow(sdl.window);
    SDL_Quit();
}

void clear_screen(const sdl_t sdl, const config_t config)
{
    const uint8_t r = (config.bg_color >> 24) & 0xFF;
    const uint8_t g = (config.bg_color >> 16) & 0xFF;
    const uint8_t b = (config.bg_color >>  8) & 0xFF;
    const uint8_t a = (config.bg_color >>  0) & 0xFF;
    
    SDL_SetRenderDrawColor(sdl.renderer, r, g, b, a);
    SDL_RenderClear(sdl.renderer);
}

void update_screen(const sdl_t sdl)
{
    SDL_RenderPresent(sdl.renderer);
}

void handle_input(chip8_t *chip8)
{
    SDL_Event event;
    
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                chip8->state = QUIT;
                return;
            
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        chip8->state = QUIT;
                        return;
                    
                    default:
                        break;
                }
                break;
            
            case SDL_KEYUP:
                break;
            
            default:
                break;
        }
    }
}

int main(int argc, char *argv[])
{
    config_t config = {0};
    if (!set_config_from_args(&config, argc, argv))
        exit(EXIT_FAILURE);
    
    sdl_t sdl = {0};
    if (!init_sdl(&sdl, config))
        exit(EXIT_FAILURE);
    
    chip8_t chip8 = {0};
    if (!init_chip8(&chip8))
        exit(EXIT_FAILURE);
    
    clear_screen(sdl, config); // Initial screen clear to background color
    
    // Main emulator loop
    while (chip8.state != QUIT)
    {
        handle_input(&chip8);
        
        // Get_time();
        // Emulate CHIP8 Instructions
        // Get_time() elapsed since last get_time();
        
        //SDL_Delay(16 - actual time elasped);
        SDL_Delay(16); // Approximately 60hz/60fps (16.67ms)
        
        update_screen(sdl);
    }
    
    final_cleanup(sdl);
    
    exit(EXIT_SUCCESS);
}
