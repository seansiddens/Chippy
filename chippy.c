#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <SDL.h>

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

uint16_t PC; // 16 bit program counter register
uint16_t I;  // 16 bit index register

uint8_t delay_timer;
uint8_t sound_timer;

// General purpose registers
uint8_t REGS[16] = {0};

struct stack *stack; // Stack

void update_timers() {
    //printf("Updating timers...\n");
    // Decrement delay timer register
    if (delay_timer > 0) {
        delay_timer--;
    }
    // Decrement sound timer register
    if (sound_timer > 0) {
        sound_timer--;
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
    uint16_t fetched_instr; 
    fetched_instr = (MEM[PC] << 8) | MEM[PC+1];
    PC += 2; // Increment PC by two to point to next instruction

    printf("Fetched instruction: %04hx\n", fetched_instr);

    
}

int main(void) {
    // Debug
    //printf("Refresh rate: %d\n", REFRESH_RATE);
    //printf("Seconds per frame: %f\n", SECONDS_PER_FRAME);
    //printf("ms per frame: %d\n", MS_PER_FRAME);




    // The window we'll be rendering to
    SDL_Window* window = NULL;

    // The surface contained by the window
    SDL_Surface* screen_surface = NULL;


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

            // Load font
            load_font();


            // Set program counter to beginning of program text
            PC = 0x200;

            step();

            // Initialize stack with max capacity of 16
            stack = new_stack(16);

            // Get window surface
            screen_surface = SDL_GetWindowSurface(window);

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

                // Update the surface
                SDL_UpdateWindowSurface(window);

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
