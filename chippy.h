#pragma once

// Function declarations;
uint32_t update_timers(uint32_t interval, void *param);
void load_program(char *file_name);
void load_font();
void step(SDL_Renderer *window_renderer);
void update_screen(SDL_Renderer *window_renderer);
void clear_screen();
void draw_sprite(uint8_t x, uint8_t y, uint8_t N);
