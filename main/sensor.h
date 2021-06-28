#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#include "debounce.h"

typedef struct {
    const char    *name;
    int           button_pin;
    int           value;
    debounce_t    debounce;
    void          *chr;
    TimerHandle_t motion_stop_timer;
    bool          needs_to_clear;
} sensor_t;

extern const int   MOTION_STOP_TIMEOUT; // ms

void sensor_init(sensor_t *sensor);
void sensor_update(sensor_t *sensor);
