#include "test_signal.h"
#include "graph.h"
#include "esp_timer.h"
#include "math.h"
#include <string.h>

float amp = 100.0;
float fx = 0.02;
float ft = 0.05;

float test_y(float x, float t) {
    float z = fx * x + ft * t;
    z = z - floor(z);
    z = (z <= 0.5) ? 2 * z : 2 * (1 - z);
    return 2 * amp * z;
}

bool first = true;

void update_test_signal() {
    if (!first) {
        trace_t tmp = traces[0][0];
        memcpy(&traces[0][0], &traces[0][1], sizeof(trace_t) * (DISPLAY_WIDTH-1));
        traces[0][DISPLAY_WIDTH - 1] = tmp;
        return;
    }
    //printf("Updating test signal");
    int64_t micros = esp_timer_get_time();
    float seconds = ((float)micros) * 1.0e-6;
    float prevy = test_y(-1.0, seconds);
    for (xcoord_t x = 0; x < DISPLAY_WIDTH; x++) {
        float y = test_y((float)x, seconds);
        ycoord_t miny = round((y < prevy) ? y : prevy);
        ycoord_t maxy = round((y > prevy) ? y : prevy);
        traces[0][x] = ((trace_t)maxy << 8) | (trace_t)miny;
        prevy = y;
    }
    first = false;
}