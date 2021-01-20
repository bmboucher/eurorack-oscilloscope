/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "ST7789.h"
#include "graph.h"
#include "test_signal.h"
#include "esp_task_wdt.h"

/* Can use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 2

void app_main(void)
{
    initialize_display();
    printf("Display initialized");
    
    init_graph();
    printf("Graph initialized");

    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    set_trace_enable(0, true);

    while(1) {
        esp_task_wdt_reset();
        printf("TEST\n");
        update_test_signal();
        draw_graph();
    }
}