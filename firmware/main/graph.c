#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "graph.h"
#include "ST7789.h"

// Trace data
trace_t* traces[NUM_TRACES];
static trace_t* trace_window;
static trace_t* dirty_window;
static bool trace_en[NUM_TRACES];
static color_t TRACE_COLORS[6] = {
    ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE,
    ST77XX_CYAN, ST77XX_ORANGE, ST77XX_MAGENTA
};

static color_t* paint_buffer;

GraphWindow activeWindow;
bool drawFull;

void set_trace_enable(size_t trace_idx, bool enable) {
    trace_en[trace_idx] = enable;
}
void init_graph() {
    paint_buffer = malloc(sizeof(uint16_t) * MAX_PIXEL_TRANSACTION);
    trace_window = malloc(sizeof(uint16_t) * DISPLAY_WIDTH);
    dirty_window = malloc(sizeof(uint16_t) * DISPLAY_WIDTH);
    for (size_t trace_idx = 0; trace_idx < NUM_TRACES; trace_idx++) {
        traces[trace_idx] = malloc(sizeof(uint16_t) * DISPLAY_WIDTH);
        trace_en[trace_idx] = false;
    }
    activeWindow.gridx = 50;
    activeWindow.gridy = 50;
    activeWindow.left = 0;
    activeWindow.right = DISPLAY_WIDTH - 1;
    activeWindow.midx = DISPLAY_WIDTH / 2;
    activeWindow.midy = DISPLAY_HEIGHT / 2;
    drawFull = true;
}

void set_graph_window(GraphWindow window) {
    memcpy(&activeWindow, &window, sizeof(GraphWindow));
    drawFull = true;
}

static void paint_graph_area(int xpos, int ypos, int w, int h) {
    const size_t n = w * h;
    memset(paint_buffer, 0, sizeof(uint16_t) * n);
    int xend = xpos + w - 1;
    int yend = ypos + h - 1;

    int gx = activeWindow.midx;
    while (gx > xpos && gx >= activeWindow.gridx) { gx -= activeWindow.gridx; }
    while (gx <= xend) {
        if (gx >= xpos) {
            color_t color = (gx == activeWindow.midx) ? ST77XX_WHITE : ST77XX_YELLOW;
            for (ycoord_t y = 0; y < h; y++) {
                paint_buffer[y * w + gx - xpos] = color;
            }
        }
        gx += activeWindow.gridx;
    }

    int gy = activeWindow.midy;
    while (gy > ypos && gy >= activeWindow.gridy) { gy -= activeWindow.gridy; }
    while (gy <= yend) {
        if (gy >= ypos) {
            color_t color = (gy == activeWindow.midy) ? ST77XX_WHITE : ST77XX_YELLOW;
            for (xcoord_t x = 0; x < w; x++) {
                paint_buffer[(gy - ypos) * w + x] = color;
            }
        }
        gy += activeWindow.gridy;
    }

    for (size_t t_idx = 0; t_idx < NUM_TRACES; t_idx++) {
        if (!trace_en[t_idx]) continue;
        for (int x = 0; x < w; x++) {
            trace_t yt = traces[t_idx][x + xpos];
            int ylo = (yt & 0xFF) - ypos;
            int yhi = (yt >> 8) - ypos;
            for (int y = ylo; y <= yhi; y++) {
                if (y >= h) continue;
                //printf("Coloring element %d at (%d, %d) with %x\n",
                //        y * w + x, x, y, TRACE_COLORS[t_idx]);
                paint_buffer[y * w + x] = TRACE_COLORS[t_idx];
            }
        }
    }

    send_pixels(xpos, ypos, w, h, paint_buffer);
}

static trace_t widen(trace_t old, trace_t trace) {
    ycoord_t old_lo = (old & 0xFF);
    ycoord_t trace_lo = (trace & 0xFF);
    ycoord_t new_lo = (trace_lo < old_lo) ? trace_lo : old_lo;
    ycoord_t old_hi = (old >> 8);
    ycoord_t trace_hi = (trace >> 8);
    ycoord_t new_hi = (trace_hi > old_hi) ? trace_hi : old_hi;
    return (new_hi << 8) | new_lo;
}

static void update_trace_window(uint16_t* window) {
    memcpy(window, traces[0], sizeof(trace_t) * DISPLAY_WIDTH);
    for (xcoord_t xpos = activeWindow.left; xpos <= activeWindow.right; xpos++) {
        for (size_t t_idx = 1; t_idx < NUM_TRACES; t_idx++) {
            window[xpos] = widen(window[xpos], traces[t_idx][xpos]);
        }
    }
}

static void draw_graph_full() {
    const xcoord_t MAX_COL = MAX_PIXEL_TRANSACTION / DISPLAY_HEIGHT;
    for (xcoord_t xpos = activeWindow.left; xpos <= activeWindow.right; xpos += MAX_COL) {
        xcoord_t n_col = (xpos + MAX_COL > activeWindow.right) ? (activeWindow.right - xpos + 1) : MAX_COL;
        paint_graph_area(xpos, 0, n_col, DISPLAY_HEIGHT);
    }
    update_trace_window(dirty_window);
    drawFull = false;
}

static ycoord_t theight(trace_t t) {
    ycoord_t tlo = t & 0xFF;
    ycoord_t thi = t >> 8;
    return thi - tlo + 1;
}

static void draw_graph_partial() {
    update_trace_window(trace_window);
    xcoord_t xpos = activeWindow.left;
    while (xpos <= activeWindow.right) {
        ycoord_t ypos = 0;
        ycoord_t h = 0;
        xcoord_t w = 1;
        trace_t update = dirty_window[xpos];
        update = widen(update, trace_window[xpos]);
        while (w * theight(update) < MAX_PIXEL_TRANSACTION) {
            h = theight(update);
            ypos = update & 0xFF;
            if (xpos + (w++) > activeWindow.right) break;
            update = widen(update, dirty_window[xpos + w - 1]);
            update = widen(update, trace_window[xpos + w - 1]);
        }
        w--;
        paint_graph_area(xpos, ypos, w, h);
        xpos += w;
    }
    memcpy(dirty_window, trace_window, sizeof(trace_t) * DISPLAY_WIDTH);
}

void draw_graph() {
    if (drawFull) {
        printf("Drawing full\n");
        draw_graph_full();
    } else {
        printf("Drawing partial\n");
        draw_graph_full();
    }
}