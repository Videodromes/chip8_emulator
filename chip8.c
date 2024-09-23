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
    uint8_t memory[4096];
    bool display[64 * 32];  // Emulate original CHIP8 resolution
    uint16_t stack[12];     // Subroutine stack
    uint8_t registers[16];  // Data registers V0-VF
    uint16_t index;         // Address register
    uint16_t pc;            // Program Counter
    uint8_t delay_timer;    // Decrements at 60hz when > 0
    uint8_t sound_timer;    // Decrements at 60hx and plays tone when > 0
    bool keypad[16];        // Hexadecimal keypad 0x0 - 0xF
    const char *rom_name;   // Currently loaded ROM
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
        .bg_color = 0x000000FF, // BLACK
        .scale_factor = 20,     // Default resolution will be 1280x640
    };
    
    for (int i = 1; i < argc; i++)
    {
        (void)argv[i];  //Prevent compiler error from unused variables argc/argv
        // ...
    }
    
    return true;
}

bool init_chip8(chip8_t *chip8, const char rom_name[])
{
    const uint32_t entry_point = 0x200; // CHIP8 roms will be loaded to 0x200
    const uint8_t font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0,   // 0
        0x20, 0x60, 0x20, 0x20, 0x70,   // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0,   // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0,   // 3
        0x90, 0x90, 0xF0, 0x10, 0x10,   // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0,   // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0,   // 6
        0xF0, 0x10, 0x20, 0x40, 0x40,   // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0,   // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0,   // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90,   // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0,   // B
        0xF0, 0x80, 0x80, 0x80, 0xF0,   // C
        0xE0, 0x90, 0x90, 0x90, 0xE0,   // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0,   // E
        0xF0, 0x80, 0xF0, 0x80, 0x80    // F
    };
    // Load font
    memcpy(&chip8->memory[0], font, sizeof(font));
    
    // Open rom file
    FILE *rom = fopen(rom_name, "rb");
    if (!rom)
    {
        SDL_Log("Rom file %s is invalid or does not exist\n", rom_name);
        return false;
    }
    
    // Get/check rom size
    fseek(rom, 0, SEEK_END);
    const size_t rom_size = ftell(rom);
    const size_t max_size = sizeof chip8->memory - entry_point;
    rewind(rom);
    
    if (rom_size > max_size)
    {
        SDL_Log("Rom file %s is too big!", rom_name);
        return false;
    }
    
    // Load rom
    if (fread(&chip8->memory[entry_point], rom_size, 1, rom) != 1)
    {
        SDL_Log("Could not read rom file %s into CHIP8 memory\n", rom_name);
        return false;
    }
    
    fclose(rom);
    
    // Set chip8 machine defaults
    chip8->state = RUNNING;
    chip8->pc = entry_point;    // Start program counter at rom entry point
    chip8->rom_name = rom_name;
    
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
                    
                    case SDLK_SPACE:
                        if (chip8->state == RUNNING)
                        {
                            chip8->state = PAUSED;
                            puts("==== PAUSED ====");
                        }
                        else
                            chip8->state = RUNNING;
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

/* Emulate 1 CHIP8 instruction
void emulate_instructions(chip8_t *chip8)
{
    
} */

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <rom_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    config_t config = {0};
    if (!set_config_from_args(&config, argc, argv))
        exit(EXIT_FAILURE);
    
    sdl_t sdl = {0};
    if (!init_sdl(&sdl, config))
        exit(EXIT_FAILURE);
    
    chip8_t chip8 = {0};
    const char *rom_name = argv[1];
    if (!init_chip8(&chip8, rom_name))
        exit(EXIT_FAILURE);
    
    clear_screen(sdl, config); // Initial screen clear to background color
    
    // Main emulator loop
    while (chip8.state != QUIT)
    {
        handle_input(&chip8);
        
        if (chip8.state == PAUSED)
            continue;
        
        // Get_time();
        
        // Emulate CHIP8 Instructions
        //emulate_instructions(&chip8);
        
        // Get_time() elapsed since last get_time();
        
        //SDL_Delay(16 - actual time elasped);
        SDL_Delay(16); // Approximately 60hz/60fps (16.67ms)
        
        update_screen(sdl);
    }
    
    final_cleanup(sdl);
    
    exit(EXIT_SUCCESS);
}
