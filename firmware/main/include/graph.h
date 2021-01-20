#pragma once

#include "ST7789.h"
#include <stdlib.h>
#include <stdbool.h>

#define NUM_TRACES 6

typedef uint8_t grid_t;
typedef uint16_t trace_t; // Two bytes for low/hi on y axis

extern trace_t* traces[NUM_TRACES];

typedef struct GraphWindow {
    grid_t gridx;
    grid_t gridy;
    xcoord_t left;
    xcoord_t right;
    xcoord_t midx;
    ycoord_t midy;
} GraphWindow;

void set_trace_enable(size_t trace_idx, bool enable);
void set_graph_window(GraphWindow window);

void init_graph();
void draw_graph();