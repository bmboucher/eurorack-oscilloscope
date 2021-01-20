#include "tasks.h"
#include "graph.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define GRAPH_TASK_PRIO 2

static void displayTask(void* param) {
    draw_graph();
}

void createTasks() {
    xTaskCreate(displayTask, "display", 4096, NULL, GRAPH_TASK_PRIO, NULL);
}