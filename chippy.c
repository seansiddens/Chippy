#include "chippy.h"

#include "stack.h"

#include <SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define FALSE 0
#define TRUE  1

const int PIXEL_SIZE = 16; // width and height of each individual "pixel"
// Screen dimension constants
const int SCREEN_WIDTH = PIXEL_SIZE * 64;
const int SCREEN_HEIGHT = PIXEL_SIZE * 32;

// Screen buffer
uint8_t SCREEN_BUFFER[32][64] = { 0 };

// Instruction execution rate
const int INSTR_PER_SEC = 700;
const uint32_t MS_PER_INSTR = (uint32_t)(1.0 / (float) (INSTR_PER_SEC) *1000.0);

// Size of loaded program in bytes
unsigned int PROGRAM_SIZE;

// Initialize memory
uint8_t MEM[4096] = { 0 }; // Byte addressable

// Registers
enum registers { V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, VA, VB, VC, VD, VE, VF };
uint8_t REGS[16] = { 0 };
uint8_t DELAY_TIMER, SOUND_TIMER;
uint16_t PC, I;

struct stack *stack; // Stack

// Callback function for updating timer registers every 60 times a second
uint32_t update_timers(uint32_t interval, void *param) {
    //printf("Updating timers...\n");
    // Decrement delay timer register
    if (DELAY_TIMER > 0) {
        DELAY_TIMER--;
    }
    // Decrement sound timer register
    if (SOUND_TIMER > 0) {
        SOUND_TIMER--;
        //printf("Sound timer register: %hhx\n", REGS.sound_timer);
    }

    return interval;
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
    enum font_indices {
        ZERO,
        ONE,
        TWO,
        THREE,
        FOUR,
        FIVE,
        SIX,
        SEVEN,
        EIGHT,
        NINE,
        A,
        B,
        C,
        D,
        E,
        F
    };

    uint8_t font[][5] = {
        { 0xf0, 0x90, 0x90, 0x90, 0xf0 }, // ZERO
        { 0x20, 0x60, 0x20, 0x20, 0x70 }, // ONE
        { 0xf0, 0x10, 0xf0, 0x80, 0xf0 }, // TWO
        { 0xf0, 0x10, 0xf0, 0x10, 0xf0 }, // THREE
        { 0x90, 0x90, 0xf0, 0x10, 0x10 }, // FOUR
        { 0xf0, 0x80, 0xf0, 0x10, 0xf0 }, // FIVE
        { 0xf0, 0x80, 0xf0, 0x90, 0xf0 }, // SIX
        { 0xf0, 0x10, 0x20, 0x40, 0x40 }, // SEVEN
        { 0xf0, 0x90, 0xf0, 0x90, 0xf0 }, // EIGHT
        { 0xf0, 0x90, 0xf0, 0x10, 0xf0 }, // NINE
        { 0xf0, 0x90, 0xf0, 0x90, 0x90 }, // A
        { 0xe0, 0x90, 0xe0, 0x90, 0xe0 }, // B
        { 0xf0, 0x80, 0x80, 0x80, 0xf0 }, // C
        { 0xe0, 0x90, 0x90, 0x90, 0xe0 }, // D
        { 0xf0, 0x80, 0xf0, 0x80, 0xf0 }, // E
        { 0xf0, 0x80, 0xf0, 0x80, 0x80 } // F
    };
    memcpy(&MEM[0x050], font, 16 * 5);
}

// Fetch/decode/execute loop
void step(SDL_Renderer *window_renderer) {
    //printf("PC: %04hx\n", PC);
    uint16_t fetched_instr;
    fetched_instr = (MEM[PC] << 8) | MEM[PC + 1];
    PC += 2; // Increment PC by two to point to next instruction after fetching instr

    uint8_t first_nibble = (fetched_instr >> 12) & 0x0f; // First nibble
    uint8_t X = (fetched_instr >> 8) & 0x0f; // Second nibble - used to look up registers
    uint8_t Y = (fetched_instr >> 4) & 0x0f; // Third nibble - used to look up registers
    uint8_t N = fetched_instr & 0x0f; // Fourth nibble - A 4-bit immediate number
    uint8_t NN
        = (Y << 4) | N; // Second byte (third and fourth nibbles) - An 8-bit immediate number.
    uint16_t NNN
        = (X << 8) | NN; // Second, third, and fourth nibbles - A 12-bit immediate memory address
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
        case 0x0ee:
            // Return from a subroutine - pop PC off stack
            PC = pop(stack);
            break;
        default: printf("UNKNOWN INSTRUCTION: %04hx\n", fetched_instr); break;
        };
        break;
    case 0x1:
        // Jump to address NNN
        PC = NNN;
        break;
    case 0x2:
        // Execute subroutine starting at address NNN
        // Push current PC to stack before jumping to subroutine
        push(stack, PC);
        PC = NNN;
        break;
    case 0x3:
        // Skip the following instruction if the value of register VX equals NN
        if (REGS[X] == NN)
            PC += 2;
        break;
    case 0x4:
        // Skip the following instruction if the value of register VX is not equal to NN
        if (REGS[X] != NN)
            PC += 2;
        break;
    case 0x5:
        // Skip the following instruction if the value of register VX is equal to value of VY
        if (REGS[X] == REGS[Y])
            PC += 2;
        break;
    case 0x6:
        // Store number NN in register VX
        REGS[X] = NN;
        break;
    case 0x7:
        // Add the value NN to register VX
        REGS[X] += NN;
        break;
    case 0x8:
        // Switch on final half-byte of instruction
        switch (N) {
        case 0:
            // Store the value of VY in VX
            REGS[X] = REGS[Y];
            break;
        case 1:
            // Set VX to VX OR VY
            REGS[X] |= REGS[Y];
            break;
        case 2:
            // Set VX to VX AND VY
            REGS[X] &= REGS[Y];
            break;
        case 3:
            // Set VX to VX XOR VY
            REGS[X] ^= REGS[Y];
            break;
        case 4:
            // Add the value of register VY to register VX
            // Set VF to 01 if a carry occurs
            // Set VF to 00 if a carry does not occur
            if (REGS[X] + REGS[Y] > 255)
                REGS[VF] = 1;
            else
                REGS[VF] = 0;

            REGS[X] += REGS[Y];
            break;
        case 5:
            // Subtract the value of register VY from register VX
            // Set VF to 0 if a borrow occurs
            // Set VF to 1 if a borrow does not occur
            if (REGS[Y] > REGS[X])
                REGS[VF] = 0;
            else
                REGS[VF] = 1;
            REGS[X] -= REGS[Y];
            break;
        case 6:
            // Set register VF to the least significant bit prior to the shift
            // VY is unchanged
            //REGS[X] = REGS[Y];
            REGS[VF] = REGS[X] & 1;
            REGS[X] >>= 1;
            break;
        case 7:
            // Set register VX to the value of VY minus VX
            if (REGS[X] > REGS[Y])
                REGS[VF] = 0;
            else
                REGS[VF] = 1;
            REGS[X] = REGS[Y] - REGS[X];
            break;
        case 0xe:
            // Store MSB of VX in VF and then shifts VX to left by 1
            REGS[VF] = REGS[X] & 0x80;
            REGS[X] <<= 1;
            break;
        default: printf("UNKNOWN INSTRUCTION: %04hx\n", fetched_instr); break;
        }
        break;
    case 0x9:
        // Skip the following instruction if the value of VX is not equal to value of VY
        if (REGS[X] != REGS[Y])
            PC += 2;
        break;
    case 0xa:
        // Store memory address NNN in I
        I = NNN;
        break;
    case 0xc:
        // Generate a random number between 0 and 255, AND it with NN, and store in VX
        REGS[X] = (rand() % 256) & NN;
        break;
    case 0xd:
        // Draw a sprite at position VX, VY with N bytes of sprite data starting at the
        // address stored in I. Set VF to 01 if any set pixels are changed to unset,
        // and 00 otherwise.
        //printf("Display instr: %04hx\n", fetched_instr);
        draw_sprite(REGS[X], REGS[Y], N);

        // Only update screen when a draw instruction is executed
        update_screen(window_renderer);
        break;
    case 0xf:
        // Switch on final byte
        switch (NN) {
        case 0x07:
            // Store the current value of the delay timer in register VX
            REGS[X] = DELAY_TIMER;
            break;
        case 0x1e:
            // Add the value stored in register VX to register I
            I += REGS[X];
            break;
        case 0x55:
            // Store the values of registers V0 to VX inclusive in memory starting at
            // address I. I is set to I + X + 1 after operation
            for (int i = 0; i <= X; i++) {
                MEM[I + i] = REGS[i];
            }
            I += X + 1;
            break;
        case 0x65:
            // Fill registers V0 to VX inclusive with the values stored in memory
            // starting at address I. I is set to I + X + 1 after operation
            for (int i = 0; i <= X; i++) {
                REGS[i] = MEM[I + i];
            }
            I += X + 1;
            break;
        default: printf("UNKNOWN INSTRUCTION: %04hx\n", fetched_instr); break;
        }
        break;
    default: printf("UNKNOWN INSTRUCTION: %04hx\n", fetched_instr); break;
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
            } else {
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

// Draw an N pixels tall sprite from memory location I at the X and Y coordinate in
// screen buffer.
void draw_sprite(uint8_t x, uint8_t y, uint8_t N) {
    // Wrap initial coordinates
    if (x >= 64) {
        x %= 64;
    }
    if (y >= 32) {
        y %= 32;
    }

    //printf("Displaying at location: (%hhd, %hhd) with height: %hhd\n", x, y, N);

    // Set VF flag to 0
    REGS[VF] = 0;

    uint8_t sprite_row;
    // Iterate through each row of the sprite data
    for (int yline = 0; yline < N; yline++) {

        // Fetch individual row of sprite data
        sprite_row = MEM[I + yline];

        // Iterate through each bit in the sprite_row byte
        for (int xline = 0; xline < 8; xline++) {
            // Mask to select for individual bit in sprite_row byte
            if ((sprite_row & (0x80 >> xline)) != 0) {

                // If screen_buffer pixel is also ON, set VF flag to 1
                if (SCREEN_BUFFER[y + yline][x + xline] == 1)
                    REGS[VF] = 1;

                // XOR sprite bit with screen value to draw to screen
                SCREEN_BUFFER[y + yline][x + xline] ^= 1;
            }
        }
    }
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
    } else {
        // Create window
        window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (window == NULL) {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        } else {
            // Get window renderer
            window_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

            // Clear screen
            //SDL_SetRenderDrawColor(window_renderer, 255, 255, 255, 255);
            SDL_RenderClear(window_renderer);

            // Initialize random seed
            time_t t;
            srand((unsigned) time(&t));

            // Setup timer for updating timer registers
            uint32_t delay
                = (uint32_t)(1.0 / 60.0 * 1000.0); // Update 60 times a second, or every 16 ms
            SDL_TimerID timer_id = SDL_AddTimer(delay, update_timers, NULL);

            // Initialize stack with max capacity of 16
            stack = new_stack(16);

            // Load ROM
            load_program("roms/pong.ch8");
            // Load font sprites
            load_font();

            // Set program counter to 0x200
            PC = 0x200;

            // Main loop flag
            unsigned char quit = FALSE;
            // Event handler
            SDL_Event e;
            // Main loop
            while (!quit) {
                // Handle events on queue
                while (SDL_PollEvent(&e) != 0) {
                    switch (e.type) {
                    case SDL_QUIT: quit = TRUE; break;
                    case SDL_KEYDOWN:
                        switch (e.key.keysym.sym) {
                        case SDLK_ESCAPE: quit = TRUE; break;
                        }
                        break;
                    }
                }

                // Carry out fetch/decode/execute loop
                if (PC - 0x200 < PROGRAM_SIZE) {
                    //printf("PC: %04hx\n", PC);
                    step(window_renderer);
                }

                // Refresh delay
                SDL_Delay(MS_PER_INSTR);
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
