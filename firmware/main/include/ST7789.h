#pragma once

#include <stdint.h>

#define DISPLAY_HEIGHT 240
#define DISPLAY_WIDTH 320
#define MAX_LINES 2
#define MAX_PIXEL_TRANSACTION (MAX_LINES*DISPLAY_WIDTH)

typedef uint16_t xcoord_t; // Width = 320 -> two bytes
typedef uint8_t ycoord_t;  // Height = 240 -> one byte
typedef uint16_t color_t;  // Color = R5G6B5 encoded

void initialize_display();
void send_pixels(xcoord_t xpos, ycoord_t ypos, 
                 xcoord_t width, ycoord_t height, 
                 color_t *data);
void blank_screen();

// Some ready-made 16-bit ('565') color settings:
#define ST77XX_BLACK 0x0000
#define ST77XX_WHITE 0xFFFF
#define ST77XX_RED 0xF800
#define ST77XX_GREEN 0x07E0
#define ST77XX_BLUE 0x001F
#define ST77XX_CYAN 0x07FF
#define ST77XX_MAGENTA 0xF81F
#define ST77XX_YELLOW 0xFFE0
#define ST77XX_ORANGE 0xFC00