#pragma once

// Function declarations;
void update_timers();
void load_program(char *file_name);
void load_font();
void step();
void update_screen(SDL_Renderer *window_renderer);
void clear_screen();
void draw_sprite(uint8_t x, uint8_t y, uint8_t N);
