#define LOGNAME "SISWI"

#include <hk.h>
#include <hk_fascade.h>

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <hk.h>

#include "segreto-di-fatima.h"
#include "sensor.h"

//

const char* NAME                   = "Furboporta";
const char* ACCESSORY_NAME         = "Furboporta";
const char* ACCESSORY_MANUFACTURER = "Furberie";
const char* ACCESSORY_MODEL        = "FRB-002";
const char* ACCESSORY_SERIAL_NR    = "0";
const char* ACCESSORY_REVISION     = "R1";

const int   MOTION_STOP_TIMEOUT    = 10000; // ms

sensor_t sensors[] = {
    { .name = "Button", .button_pin = 36,
        .debounce = { 
            .inverted = true
        },
    },
    { .name = "Sensor", .button_pin = 39,
        .debounce = { 
            .inverted = false
        },
    },
    { .name = "Bell",   .button_pin = 34,
        .debounce = { 
            .inverted = true
        },
    },
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
