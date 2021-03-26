#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <SDL.h>

#include "chippy.h"
#include "stack.h"

#define FALSE 0
#define TRUE 1

const int PIXEL_SIZE = 8; // width and height of each individual "pixel"
// Screen dimension constants
const int SCREEN_WIDTH = PIXEL_SIZE * 64; 
const int SCREEN_HEIGHT = PIXEL_SIZE * 32;

// Screen buffer
uint8_t SCREEN_BUFFER[32][64] = {0};

// Screen refresh rate
const int REFRESH_RATE = 60;
const float SECONDS_PER_FRAME = 1.0 / (float)REFRESH_RATE;
const int MS_PER_FRAME = (int)(SECONDS_PER_FRAME * 1000);

// Size of loaded program in bytes
unsigned int PROGRAM_SIZE;



// Initialize memory
uint8_t MEM[4096] = {0}; // Byte addressable

// Registers
union Registers {
    uint8_t V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, VA, VB, VC, VD, VE, VF;
    uint8_t delay_timer, sound_timer;
};
union Registers REGS;

uint16_t PC, I;

struct stack *stack; // Stack

void update_timers() {
    //printf("Updating timers...\n");
    // Decrement delay timer register
    if (REGS.delay_timer > 0) {
        REGS.delay_timer--;
    }
    // Decrement sound timer register
    if (REGS.sound_timer > 0) {
        REGS.sound_timer--;
        //printf("Sound timer register: %hhx\n", REGS.sound_timer);
    }
}

// Load program rom into memory beginning at address 0x200
void load_program(char *file_name) { 
    // Open file
    FILE *fp = fopen(file_name, "rb");
    if (fp == NULL) {
        printf("ERROR READING PROGRAM: %s\n", file_name);
    }
    printf("Program loaded: %s\n", file_name);

    // Get size of program
    fseek(fp, 0, SEEK_END); // Seek to end of file
    int file_size = ftell(fp);
    PROGRAM_SIZE = file_size;
    printf("Program size: %d bytes\n", file_size);
    fseek(fp, 0, SEEK_SET); // Seek back to beginning of file


    // Load CHIP-8 program into memory starting at address 0x200.
    fread(&MEM[0x200], sizeof(uint8_t), file_size, fp);



    // Test if program was correctly loaded into memory
    for (int i = 0; i < file_size; i++) {
        if (i % 8 == 0) {
            printf("\n");
        }
        printf("%02hhx ", MEM[0x200 + i]);
    }
    printf("\n");


    // Close program file
    fclose(fp);
}

// Load the built-in font sprites into memory from 0x050 to 0x09f
void load_font() {
    enum font_indices{ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, A, B, C, D, E, F};

    uint8_t font[][5] = {
                            {0xf0, 0x90, 0x90, 0x90, 0xf0}, // ZERO
                            {0x20, 0x60, 0x20, 0x20, 0x70}, // ONE
                            {0xf0, 0x10, 0xf0, 0x80, 0xf0}, // TWO
                            {0xf0, 0x10, 0xf0, 0x10, 0xf0}, // THREE
                            {0x90, 0x90, 0xf0, 0x10, 0x10}, // FOUR
                            {0xf0, 0x80, 0xf0, 0x10, 0xf0}, // FIVE
                            {0xf0, 0x80, 0xf0, 0x90, 0xf0}, // SIX
                            {0xf0, 0x10, 0x20, 0x40, 0x40}, // SEVEN
                            {0xf0, 0x90, 0xf0, 0x90, 0xf0}, // EIGHT
                            {0xf0, 0x90, 0xf0, 0x10, 0xf0}, // NINE
                            {0xf0, 0x90, 0xf0, 0x90, 0x90}, // A
                            {0xe0, 0x90, 0xe0, 0x90, 0xe0}, // B
                            {0xf0, 0x80, 0x80, 0x80, 0xf0}, // C
                            {0xe0, 0x90, 0x90, 0x90, 0xe0}, // D
                            {0xf0, 0x80, 0xf0, 0x80, 0xf0}, // E
                            {0xf0, 0x80, 0xf0, 0x80, 0x80}  // F
                        };
    memcpy(&MEM[0x050], font, 16 * 5);
}

// Fetch/decode/execute loop
void step() {
    //printf("PC: %04hx\n", PC);
    uint16_t fetched_instr; 
    fetched_instr = (MEM[PC] << 8) | MEM[PC+1];
    PC += 2; // Increment PC by two to point to next instruction

    uint8_t first_nibble = (fetched_instr >> 12) & 0x0f; // First nibble
    uint8_t X = (fetched_instr >> 8) & 0x0f; // Second nibble - used to look up registers
    uint8_t Y = (fetched_instr >> 4) & 0x0f; // Third nibble - used to look up registers
    uint8_t N = fetched_instr & 0x0f; // Fourth nibble - A 4-bit immediate number
    uint8_t NN = (Y << 4) | N; // Second byte (third and fourth nibbles) - An 8-bit immediate number.
    uint16_t NNN = (X << 8) | NN; // Second, third, and fourth nibbles - A 12-bit immediate memory address
    //printf("Fetched instruction: %04hx\n", fetched_instr);
    //printf("First nibble: %hhx\n", first_nibble);
    //printf("X: %hhx\n", X);
    //printf("Y: %hhx\n", Y);
    //printf("N: %hhx\n", N);
    //printf("NN: %hhx\n", NN);
    //printf("NNN: %03hx\n", NNN);

    switch (first_nibble) {
        case 0x0:
            switch (NNN) {
                case 0x0e0:
                    // Clear the screen
                    clear_screen();
                    break;
                default:
                    printf("UNKNOWN INSTRUCTION: %04hx\n", fetched_instr);
                    break;

            };
            break;
        case 0x1:
            // Jump to address NNN
            PC = NNN;
            break;
        case 0x6:
            // Store number NN in register VX
            break;
        default:
            printf("UNKNOWN INSTRUCTION: %04hx\n", fetched_instr);
            break;
    }


}

// Update screen 
void update_screen(SDL_Renderer *window_renderer) {
    // Draw "pixel" rects from screen buffer
    SDL_Rect rect;
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            rect.x = x * PIXEL_SIZE;
            rect.y = y * PIXEL_SIZE;
            rect.w = PIXEL_SIZE;
            rect.h = PIXEL_SIZE;
            if (SCREEN_BUFFER[y][x]) {

                SDL_SetRenderDrawColor(window_renderer, 255, 255, 255, 255);
            }
            else {
                SDL_SetRenderDrawColor(window_renderer, 0, 0, 0, 255);
            }
            SDL_RenderFillRect(window_renderer, &rect);
        }
    }

    // Refresh screen
    SDL_RenderPresent(window_renderer);
}

// Clear screen - set every pixel in screen buffer to 0
void clear_screen() {
    memset(SCREEN_BUFFER, 0, 64 * 32);
}

int main(void) {
    // Debug
    //printf("Refresh rate: %d\n", REFRESH_RATE);
    //printf("Seconds per frame: %f\n", SECONDS_PER_FRAME);
    //printf("ms per frame: %d\n", MS_PER_FRAME);

    // The window we'll be rendering to
    SDL_Window *window = NULL;

    // Window renderer
    SDL_Renderer *window_renderer = NULL;





    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    }
    else {
        // Create window
        window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, 
                                                  SDL_WINDOWPOS_UNDEFINED,
                                                  SCREEN_WIDTH,
                                                  SCREEN_HEIGHT,
                                                  SDL_WINDOW_SHOWN);
        if (window == NULL) {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        }
        else {
            // Load program
            load_program("programs/IBM_logo.ch8");

            // Load font sprites
            load_font();

            // Set program counter to 0x200
            PC = 0x200;


            // Initialize stack with max capacity of 16
            stack = new_stack(16);

            // Get window renderer
            window_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

            // Get texture
            SDL_Surface *surface = SDL_GetWindowSurface(window);
            SDL_Texture *texture = SDL_CreateTextureFromSurface(window_renderer, surface);
            if (texture == NULL) {
                printf("Failed to convert surface into a texture: %s\n", SDL_GetError());
            }
            SDL_FreeSurface(surface);

            // Clear screen
            //SDL_SetRenderDrawColor(window_renderer, 255, 255, 255, 255);
            SDL_RenderClear(window_renderer);



            // Main loop flag
            unsigned char quit = FALSE;

            // Event handler
            SDL_Event e;

            // Main loop
            while (!quit) {
                update_timers();

                // Handle events on queue
                while (SDL_PollEvent(&e) != 0) {
                    switch (e.type) {
                        case SDL_QUIT:
                            quit = TRUE;
                            break;
                        case SDL_KEYDOWN:
                            switch (e.key.keysym.sym) {
                                case SDLK_ESCAPE:
                                    quit = TRUE;
                                    break; 
                            }
                            break;
                    }
                }

                // Carry out fetch/decode/execute loop
                if (PC - 0x200 < PROGRAM_SIZE) {
                    step();
                }


                // Update screen
                update_screen(window_renderer);


                // Refresh delay
                SDL_Delay(MS_PER_FRAME);
            } // End main loop

        }
    }

    // Destroy window
    SDL_DestroyWindow(window);

    // Quit SDL subsystems
    SDL_Quit();

    // Deallocate stack
    delete_stack(stack);
    return 0;
}
