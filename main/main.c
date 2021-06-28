#define LOGNAME "SISWI"

#include <hk.h>
#include <hk_fascade.h>

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <driver/gpio.h>

#include "segreto-di-fatima.h"
#include "debounce.h"

const char* NAME                   = "Furboporta";
const char* ACCESSORY_NAME         = "Furboporta";
const char* ACCESSORY_MANUFACTURER = "Furberie";
const char* ACCESSORY_MODEL        = "FRB-002";
const char* ACCESSORY_SERIAL_NR    = "0";
const char* ACCESSORY_REVISION     = "R1";

const int   MOTION_STOP_TIMEOUT    = 10000; // ms

typedef struct {
    const char    *name;
    int           button_pin;
    int           value;
    debounce_t    debounce;
    void          *chr;
    TimerHandle_t motion_stop_timer;
    bool          needs_to_clear;
} sensor_t;

sensor_t sensors[] = {
    { .name = "Button", .button_pin = 36, },
    { .name = "Sensor", .button_pin = 39, },
    { .name = "Bell",   .button_pin = 34, },
};

void on_identify()
{
    ESP_LOGI(LOGNAME, "Identify");
}

esp_err_t on_write_sensor(hk_mem *request, sensor_t *sensor)
{
    sensor->value = *((bool *)request->ptr);
    ESP_LOGI(LOGNAME, "Setting value to %d", sensor->value);
    return ESP_OK;
}

esp_err_t on_read_sensor(hk_mem *response, sensor_t *sensor)
{
    printf("read sensor %s -- value %d\n", sensor->name, sensor->value);
    hk_mem_append_buffer(response, (char *)&sensor->value, sizeof(bool));
    return ESP_OK;
}

//

esp_err_t on_read_1(hk_mem *response) {
    return on_read_sensor(response, &sensors[0]);
}

esp_err_t on_read_2(hk_mem *response) {
    return on_read_sensor(response, &sensors[1]);
}

esp_err_t on_read_3(hk_mem *response) {
    return on_read_sensor(response, &sensors[2]);
}

esp_err_t name_read_1(hk_mem *response) {
    char *name = (char *)sensors[0].name;
    hk_mem_append_buffer(response, name, strlen(name));
    return ESP_OK;
}

esp_err_t name_read_2(hk_mem *response) {
    char *name = (char *)sensors[1].name;
    hk_mem_append_buffer(response, name, strlen(name));
    return ESP_OK;
}

esp_err_t name_read_3(hk_mem *response) {
    char *name = (char *)sensors[2].name;
    hk_mem_append_buffer(response, name, strlen(name));
    return ESP_OK;
}

//
void sensor_timer_callback(TimerHandle_t timer);

void sensor_init(sensor_t *sensor) {
    gpio_pad_select_gpio(sensor->button_pin);
    gpio_set_direction(sensor->button_pin, GPIO_MODE_INPUT);

    sensor->debounce.inverted = true;

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

void sensor_timer_callback(TimerHandle_t timer) {
    sensor_t *sensor = pvTimerGetTimerID(timer);
    sensor->needs_to_clear = true;
}

//

void app_main()
{
    ESP_LOGI(LOGNAME, "SDK version:%s\n", esp_get_idf_version());
    ESP_LOGI(LOGNAME, "Starting\n");

    sensor_init(&sensors[0]);
    sensor_init(&sensors[1]);
    sensor_init(&sensors[2]);

    hk_setup_start();

    hk_setup_add_accessory(
        ACCESSORY_NAME,
        ACCESSORY_MANUFACTURER,
        ACCESSORY_MODEL,
        ACCESSORY_SERIAL_NR,
        ACCESSORY_REVISION,
        on_identify
    );

    void *dummychr;

    hk_setup_add_srv(HK_SRV_DOORBELL, false, false);

    hk_setup_add_srv(HK_SRV_MOTION_SENSOR, false, false);
    hk_setup_add_chr(HK_CHR_MOTION_DETECTED, on_read_1, NULL, true, &sensors[0].chr);
    hk_setup_add_chr(HK_CHR_NAME, name_read_1, NULL, false, &dummychr);

    hk_setup_add_srv(HK_SRV_MOTION_SENSOR, false, false);
    hk_setup_add_chr(HK_CHR_MOTION_DETECTED, on_read_2, NULL, true, &sensors[1].chr);
    hk_setup_add_chr(HK_CHR_NAME, name_read_2, NULL, false, &dummychr);

    hk_setup_add_srv(HK_SRV_MOTION_SENSOR, false, false);
    hk_setup_add_chr(HK_CHR_MOTION_DETECTED, on_read_3, NULL, true, &sensors[2].chr);
    hk_setup_add_chr(HK_CHR_NAME, name_read_3, NULL, false, &dummychr);

    hk_setup_finish();

    // to restet the device. commented by default
    // hk_reset();

    // starting homekit
    hk_init(NAME, HK_CAT_DOOR, SEGRETO_DI_FATIMA);

    while (true) {
        sensor_update(&sensors[0]);
        sensor_update(&sensors[1]);
        sensor_update(&sensors[2]);

        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}
