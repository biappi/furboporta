#include "sensor.h"

#include <driver/gpio.h>
#include <hk.h>

static void sensor_timer_callback(TimerHandle_t timer);

void sensor_init(sensor_t *sensor) {
    gpio_pad_select_gpio(sensor->button_pin);
    gpio_set_direction(sensor->button_pin, GPIO_MODE_INPUT);

    sensor->motion_stop_timer = xTimerCreate(
        "motion stop timeout",
        pdMS_TO_TICKS(MOTION_STOP_TIMEOUT),
        pdFALSE,
        sensor,
        sensor_timer_callback
    );

    assert(sensor->motion_stop_timer);
}

static void sensor_set_value(sensor_t *sensor, int value) {
    sensor->value = value;
    hk_notify(sensor->chr);
    printf("notifying new value: %s: %d\n", sensor->name, sensor->value);
}

void sensor_update(sensor_t *sensor) {
    int level = gpio_get_level(sensor->button_pin);
    debounce_update(&sensor->debounce, level);

    if (sensor->needs_to_clear) {
        sensor_set_value(sensor, 0);
        sensor->needs_to_clear = false;
    }

    if (debounce_down(&sensor->debounce)) {
        printf("%s down\n", sensor->name);
        sensor_set_value(sensor, 1);
        xTimerReset(sensor->motion_stop_timer, 10);
    }

    if (debounce_up(&sensor->debounce)) {
        printf("%s up\n", sensor->name);
    }
}

static void sensor_timer_callback(TimerHandle_t timer) {
    sensor_t *sensor = pvTimerGetTimerID(timer);
    sensor->needs_to_clear = true;
}

